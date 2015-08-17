/**
 * @file ace.c
 * @brief ACE automatic color enhancement
 * @author Pascal Getreuer <getreuer@gmail.com>
 * 
 * Copyright (c) 2012, Pascal Getreuer
 * All rights reserved.
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under, at your option, the terms of the GNU General Public License as 
 * published by the Free Software Foundation, either version 3 of the 
 * License, or (at your option) any later version, or the terms of the 
 * simplified BSD license.
 *
 * You should have received a copy of these licenses along with this program.
 * If not, see <http://www.gnu.org/licenses/> and
 * <http://www.opensource.org/licenses/bsd-license.html>.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>
#ifdef _OPENMP
#include <omp.h>
#endif

int ace_enhance_image_interp(float *u, const float *f, 
    int width, int height, float alpha, const char *omega_string, 
    int num_levels);
int ace_enhance_image_poly(float *u, const float *f, 
    int width, int height, float alpha, const char *omega_string, int degree);
static int compute_omega_trans(float *omega_trans, 
    float *omega, int width, int height, const char *omega_string);
static void get_min_max(float *min_ptr, float *max_ptr, 
    const float *data, size_t num_samples);
static void int_pow(float *dest, const float *src, size_t num_samples, int m);
static void stretch(float *image, long num_pixels);
static void convolve(float *blurred_trans, const float *omega_trans, 
    long num_pixels, fftwf_plan forward_plan, fftwf_plan inverse_plan);
static int alloc_buffers_and_plans(float ***buffers_ptr, 
    fftwf_plan (**plans_ptr)[2], int width, int height, int num_buffers);
static void free_buffers_and_plans(float **buffers, 
    fftwf_plan (*plans)[2], int num_buffers);
static double binom_coeff(int m, int n);
static double factorial(int n);


/**
 * @brief ACE automatic color enhancement using level interpolation
 * @param u the enhanced output image
 * @param f the input image in planar row-major order
 * @param width, height image dimensions
 * @param alpha the slope parameter (>=1), larger implies stronger enhancement
 * @param omega_string string specifying the spatial weighting function
 * @param num_levels number of interpolation levels (>=2)
 * @return 1 on success, 0 on failure
 * 
 * This routine perform ACE enhancement by computing 
 * 
 * \f[ \omega(x) * s_\alpha(L - I(x)) \f]
 * 
 * for fixed levels L.  The \f$ R_L \f$ are then piecewise linearly 
 * interpolated to approximate R.  The parameter \c num_levels determines 
 * the number of L values used.  Increasing \c num_levels improves quality but
 * increases computation time.
 * 
 * If non-null, parameter \c omega_string specifies the spatial weighting 
 * function \f$ \omega(x,y) \f$.  Valid choices are
 * 
 *   - "1/r"  default ACE, \f$ \omega(x,y) = 1/\sqrt{x^2 + y^2} \f$
 *   - "1"    constant, \f$ \omega(x,y) = 1 \f$
 *   - "G:#"  Gaussian where # is a number specifying \f$ \sigma \f$,
 *            \f$ \omega(x,y) = \exp(-\tfrac{x^2 + y^2}{2\sigma^2}) \f$
 * 
 * If OpenMP is enabled, the main computation loop is parallelized.
 */
int ace_enhance_image_interp(float *u, const float *f, 
    int width, int height, float alpha, const char *omega_string, 
    int num_levels)
{
    const long num_pixels = ((long)width) * ((long)height);
    const long pad_num_pixels = ((long)width + 1) * ((long)height + 1);
#ifdef _OPENMP
    const int num_threads = omp_get_max_threads();
#else
    const int num_threads = 1;
#endif
    const int num_buffers = num_threads + 1;
    fftwf_plan (*plans)[2] = NULL;
    float **buffers = NULL, *omega_trans = NULL;
    int *buffer_levels = NULL, *buffer_channels = NULL;    
    float min[3], max[3], level_step[3];    
    long i;
    int thread, num_conv, index, steps, channel, success = 0;
    
    /* Allocate memory */
    if(!(omega_trans = (float *)fftwf_malloc(sizeof(float)*pad_num_pixels))
        || !compute_omega_trans(omega_trans, u, width, height, omega_string)
        || !alloc_buffers_and_plans(&buffers, &plans, 
            width, height, num_buffers)
        || !(buffer_levels = (int *)malloc(sizeof(int)*num_buffers))
        || !(buffer_channels = (int *)malloc(sizeof(int)*num_buffers)))
        goto fail;
    
    for(i = 0; i < num_pixels * 3; i++)
        u[i] = -1.5;
    
    /* Find min and max of source image for each channel */
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) private(i)
#endif    
    for(channel = 0; channel < 3; channel++)
    {
        get_min_max(&min[channel], &max[channel], 
            f + num_pixels * channel, num_pixels);
        
        if(min[channel] >= max[channel])
        {
            min[channel] = 0;
            max[channel] = 1;
        }
        
        level_step[channel] = (max[channel] - min[channel]) 
            / (num_levels - 1);
    }
    
    for(i = 0; i < num_buffers; i++)
        buffer_levels[i] = buffer_channels[i] = -1;
    
    /* The following loop is the main computation.  The ACE convolutions
     * 
     *    omega(x) * s_a(L - I(x))
     * 
     * are computed for L = min[channel] + level_step[channel]*level, where
     * level = 0, ..., num_levels-1.  The R_L are then piecewise linearly 
     * interpolated to approximate R.
     */
    for(steps = 0; steps < 3*num_levels;)
    {
        if(steps + num_threads > 3*num_levels)
            num_conv = 3*num_levels - steps;
        else if(steps == 0 && num_threads == 1)
            num_conv = 2;
        else
            num_conv = num_threads;
        
        /* Compute num_conv convolutions in parallel and store in buffers */
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) private(i)
#endif
        for(thread = 0; thread < num_conv; thread++)
        {
            int level = (steps + thread) % num_levels;
            int channel = (steps + thread) / num_levels;
            int index = (steps + thread) % num_buffers;
            const float *src = f + num_pixels * channel;
            float *dest = buffers[index];
            float L = min[channel] + level_step[channel]*level;
            
            buffer_levels[index] = level;
            buffer_channels[index] = channel;
            
            for(i = 0; i < num_pixels; i++)
            {
                float diff = alpha * (L - src[i]);
                dest[i] = (diff <= -1) ? -1 : ((diff >= 1) ? 1 : diff);
            }
            
            /* Compute buffers[index] = omega(x) * s_a(L - I(x)) */
            convolve(dest, omega_trans, num_pixels,
                plans[index][0], plans[index][1]);
        }
        
        for(thread = num_conv; thread < num_threads; thread++)
            buffer_channels[(steps + thread) % num_buffers] = -1;
        
        /* Interpolate between the levels */
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) private(i,channel)
#endif        
        for(index = 0; index < num_buffers; index++)
        {            
            int index_prev = (index) ? (index - 1) : num_threads;
            float *dest;
            const float *src, *buffer0, *buffer1;
            float L0, L1;
            
            if((channel = buffer_channels[index]) 
                != buffer_channels[index_prev]
                || channel < 0
                || buffer_levels[index_prev] + 1 != buffer_levels[index])
                continue;   /* Skip buffers that are not adjacent levels */
            
            L0 = min[channel] + level_step[channel]*(buffer_levels[index]-1);
            L1 = min[channel] + level_step[channel]*buffer_levels[index];
            dest = u + num_pixels * channel;
            src = f + num_pixels * channel;
            buffer0 = buffers[index_prev];
            buffer1 = buffers[index];
            
            /* Piecewise linear interpolation for the interval [L0, L1) */
            if(buffer_levels[index] < num_levels - 1)
            {
                for(i = 0; i < num_pixels; i++)
                    if(L0 <= src[i] && src[i] < L1)
                        dest[i] = buffer0[i] + (buffer1[i] - buffer0[i])
                            *(src[i] - L0)/level_step[channel];
            }
            else    /* Rightmost interval */
                for(i = 0; i < num_pixels; i++)
                    if(L0 <= src[i])
                        dest[i] = buffer0[i] + (buffer1[i] - buffer0[i])
                            *(src[i] - L0)/level_step[channel];
        }
        
        steps += num_conv;
    }
    
    stretch(u, num_pixels);
    success = 1;
fail:
    /* Free memory */
    if(buffer_channels)
        free(buffer_channels);
    if(buffer_levels)
        free(buffer_levels);
    free_buffers_and_plans(buffers, plans, num_buffers);
    if(omega_trans)
        fftwf_free(omega_trans);
    fftwf_cleanup();
    return success;
}


/**
 * @brief ACE automatic color enhancement using a polynomial slope function
 * @param u the enhanced output image
 * @param f the input image in planar row-major order
 * @param width, height image dimensions
 * @param alpha the slope parameter (>=1), larger implies stronger enhancement
 * @param omega_string string specifying the spatial weighting function
 * @param degree polynomial degree (can be 3, 5, 7, 9, or 11)
 * @return 1 on success, 0 on failure
 * 
 * This routine perform ACE enhancement using the fast O(N log N) algorithm of
 * Bertalmio et al.  The slope parameter must be an integer or half-integer 
 * between 1 and 8 (1, 1.5, 2, 2.5, ..., 7.5, 8).  The slope function is 
 * approximated by an \f$ L^\infty \f$-optimal polynomial of degree \c degree.
 * 
 * If non-null, parameter \c omega_string specifies the spatial weighting 
 * function \f$ \omega(x,y) \f$.  Valid choices are
 * 
 *   - "1/r"  default ACE, \f$ \omega(x,y) = 1/\sqrt{x^2 + y^2} \f$
 *   - "1"    constant, \f$ \omega(x,y) = 1 \f$
 *   - "G:#"  Gaussian where # is a number specifying \f$ \sigma \f$,
 *            \f$ \omega(x,y) = \exp(-\tfrac{x^2 + y^2}{2\sigma^2}) \f$
 * 
 * If OpenMP is enabled, the main computation loop is parallelized.  
 * 
 * note: In the parallel computation, values are summed in a nondeterministic
 * order (i.e., in whichever order threads complete).  Due to rounding 
 * effects, addition is not exactly associative and the output varies slightly
 * between runs (+/- 1 intensity level).
 */
int ace_enhance_image_poly(float *u, const float *f, 
    int width, int height, float alpha, const char *omega_string, int degree)
{
#ifndef DOXYGEN_SHOULD_SKIP_THIS    
#include "slopecoeff_inc.c"
#endif
    const float *slope_coeffs[5];
#ifdef _OPENMP
    const int num_threads = omp_get_max_threads();
#else
    const int num_threads = 1;
#endif
    const long num_pixels = ((long)width) * ((long)height);
    const long pad_num_pixels = ((long)width + 1) * ((long)height + 1);
    float **buffers = NULL, *omega_trans = NULL, *poly_coeffs = NULL;
    fftwf_plan (*plans)[2] = NULL;    
    long i;
    int iter, n, channel, success = 0;
    
    slope_coeffs[0] = slope_coeff_3;
    slope_coeffs[1] = slope_coeff_5;
    slope_coeffs[2] = slope_coeff_7;
    slope_coeffs[3] = slope_coeff_9;
    slope_coeffs[4] = slope_coeff_11;
    
    if(degree < 3)
        degree = 3;
    else if(degree > 11)
        degree = 11;
    
    /* Allocate memory */
    if(!(omega_trans = (float *)fftwf_malloc(sizeof(float)*pad_num_pixels))
        || !compute_omega_trans(omega_trans, u, width, height, omega_string)
        || !alloc_buffers_and_plans(&buffers, &plans, 
            width, height, num_threads)
        || !(poly_coeffs = (float *)malloc(
            sizeof(float)*(degree + 1)*(degree + 1))))
        goto fail;
    
    /* Select polynomial from SlopeCoeff table */
    {
        int degree_index = (degree - 3) / 2;
        int alpha_index = (int)(2*alpha + 0.5f) - 2;
        
        if(alpha_index < 0)
            alpha_index = 0;
        else if(alpha_index > 14)
            alpha_index = 14;
        
        alpha_index *= (degree_index + 2);
        
        /* Precompute coefficients */
        for(n = 0; n <= degree; n++)
        {
            int m;
            
            for(m = n + ((n % 2 == 0) ? 1 : 0); m <= degree; m += 2)
                poly_coeffs[(degree + 1)*n + m] = (((m - n + 1) % 2) ? -1 : 1)
                    * slope_coeffs[degree_index][alpha_index + (m - 1)/2]
                    * binom_coeff(m, n);
        }
    }

    /* Special case for n = zero term */
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) private(i)
#endif
    for(channel = 0; channel < 3; channel++)
    {
        const float *src = f + num_pixels * channel;
        float *dest = u + num_pixels * channel;
        
        for(i = 0; i < num_pixels; i++)
        {
            float Temp = src[i];
            float TempSqr = Temp*Temp;                
            float a = poly_coeffs[degree];
            int m = degree;
            
            while(m >= 2)
                a = a*TempSqr + poly_coeffs[m -= 2];
            
            dest[i] = a * Temp;
        }
    }
    
    /* Most of the computation time is spent in this loop. */
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) private(i)
#endif
    for(iter = 0; iter < 3*degree; iter++)
    {
#ifdef _OPENMP
        const int threadId = omp_get_thread_num();
#else
        const int threadId = 0;
#endif
        int channel = iter / degree;    /* = current image channel  */
        int n = 1 + (iter % degree);    /* = current summation term */
        const float *src = f + num_pixels * channel;
        float *dest = u + num_pixels * channel;
        float *Blurred = buffers[threadId];

        /* Compute Blurred = src to the nth power */
        int_pow(Blurred, src, num_pixels, n);
        /* convolve Blurred with omega */
        convolve(Blurred, omega_trans, num_pixels,
            plans[threadId][0], plans[threadId][1]);
        
        for(i = 0; i < num_pixels; i++)
        {
            float Temp = src[i];
            float TempSqr = Temp*Temp;
            float a = poly_coeffs[(degree + 1)*n + degree];
            int m = degree;
            
            while(m - n >= 2)
            {
                m -= 2;
                a = a*TempSqr + poly_coeffs[(degree + 1)*n + m];
            }
            
            if(n % 2 == 0)
                a *= Temp;
            
#ifdef _OPENMP            
            Blurred[i] *= a;
#else
            dest[i] += a * Blurred[i];
#endif
        }
        
#ifdef _OPENMP
#pragma omp critical
        for(i = 0; i < num_pixels; i++)
            dest[i] += Blurred[i];
#endif        
    }

    stretch(u, num_pixels);
    success = 1;
fail:
    /* Free memory */
    if(poly_coeffs)
        free(poly_coeffs);
    free_buffers_and_plans(buffers, plans, num_threads);    
    if(omega_trans)
        fftwf_free(omega_trans);
    fftwf_cleanup();
    return success;
}


/** 
 * @brief Compute the FFT of omega(x,y) 
 * @param omega_trans destination array
 * @param omega workspace array (overwritten by computation)
 * @param width, height image dimensions
 * @param omega_string string specifying the spatial weighting function
 * @return 1 on success, 0 on failure
 */
static int compute_omega_trans(float *omega_trans, 
    float *omega, int width, int height, const char *omega_string)
{
    fftwf_plan plan = NULL;
    long pad_num_pixels = ((long)width + 1) * ((long)height + 1);
    double sum;
    long i, x, y;
    
    if(!omega_string || !strcmp(omega_string, "1/r"))
        for(y = 0, i = 0; y <= height; y++) /* omega = 1/sqrt(x^2 + y^2) */
            for(x = 0; x <= width; x++, i++)
                omega[i] = (x == 0 && y == 0) ? 0
                    : 1.0f/sqrt(x*x + y*y);
    else if(!strcmp(omega_string, "1"))
        for(y = 0, i = 0; y <= height; y++) /* omega = 1*/
            for(x = 0; x <= width; x++, i++)
                omega[i] = 1.0f;
    else if(strlen(omega_string) >= 3 
        && omega_string[0] == 'G' && omega_string[1] == ':')
    {
        double sigma = atof(omega_string + 2);
        
        if(sigma <= 0)
            return 0;
        
        for(y = 0, i = 0; y <= height; y++) /* omega = Gaussian */
            for(x = 0; x <= width; x++, i++)
                omega[i] = exp(-(x*x + y*y)/(2*sigma*sigma));
    }
    else
        return 0;
    
    for(y = 0, i = 0, sum = 0; y <= height; y++)
        for(x = 0; x <= width; x++, i++)
            sum += ((x == 0 || x == width) ? 1 : 2) 
                * ((y == 0 || y == height) ? 1 : 2) 
                * omega[i];
    
    sum *= 4*pad_num_pixels;
    
    for(i = 0; i < pad_num_pixels; i++)
        omega[i] /= sum;
    
    if(!(plan = fftwf_plan_r2r_2d(height + 1, width + 1, omega, omega_trans,
        FFTW_REDFT00, FFTW_REDFT00, FFTW_ESTIMATE | FFTW_DESTROY_INPUT)))
        return 0;
    
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
    
    /* Cut last row and column from KernelTrans */
    for(y = 1, i = width; y < height; y++, i += width)
        memmove(omega_trans + i, omega_trans + i + y, sizeof(float)*width); 
    
    return 1;    
}

/** 
 * @brief Find the min and max value of an array 
 * @param min_ptr set to minimum value
 * @param max_ptr set to maximum value
 * @param data source data
 * @param num_samples number of data elements
 */
static void get_min_max(float *min_ptr, float *max_ptr, 
    const float *data, size_t num_samples)
{
    float min, max;
    size_t i;
    
    if(num_samples <= 0)
    {
        *min_ptr = *max_ptr = 0;
        return;
    }
    
    min = max = data[0];
    
    for(i = 1; i < num_samples; i++)
        if(min > data[i])
            min = data[i];
        else if(max < data[i])
            max = data[i];
    
    *min_ptr = min;
    *max_ptr = max;
}


/** 
 * @brief Evaluate integer power, hardcoded for degrees 1 to 11 
 * @param dest destination data
 * @param src source data
 * @param num_samples number of data elements
 * @param m integer power
 */
static void int_pow(float *dest, const float *src, size_t num_samples, int m)
{
    size_t i;
    
    switch(m)
    {
    case 1:
        memcpy(dest, src, sizeof(float)*num_samples);
        break;
    case 2:
        for(i = 0; i < num_samples; i++)
        {
            float x = src[i];
            dest[i] = x * x;
        }
        break;
    case 3:
        for(i = 0; i < num_samples; i++)
        {
            float x = src[i];
            dest[i] = x * x * x;
        }
        break;
    case 4:
        for(i = 0; i < num_samples; i++)
        {
            float x = src[i];
            float x2 = x * x;
            dest[i] = x2 * x2;
        }
        break;
    case 5:
        for(i = 0; i < num_samples; i++)
        {
            float x = src[i];
            float x2 = x * x;
            dest[i] = x2 * x2 * x;
        }
        break;
    case 6:
        for(i = 0; i < num_samples; i++)
        {
            float x = src[i];
            float x2 = x * x;
            dest[i] = x2 * x2 * x2;
        }
        break;
    case 7:
        for(i = 0; i < num_samples; i++)
        {
            float x = src[i];
            float x2 = x * x;
            dest[i] = x2 * x2 * x2 * x;
        }
        break;
    case 8:
        for(i = 0; i < num_samples; i++)
        {
            float x = src[i];
            float x2 = x * x;
            float x4 = x2 * x2;
            dest[i] = x4 * x4;
        }
        break;
    case 9:
        for(i = 0; i < num_samples; i++)
        {
            float x = src[i];
            float x2 = x * x;
            float x4 = x2 * x2;
            dest[i] = x4 * x4 * x;
        }
        break;
    case 10:
        for(i = 0; i < num_samples; i++)
        {
            float x = src[i];
            float x2 = x * x;
            float x4 = x2 * x2;
            dest[i] = x4 * x4 * x2;
        }
        break;
    case 11:
        for(i = 0; i < num_samples; i++)
        {
            float x = src[i];
            float x2 = x * x;
            float x4 = x2 * x2;
            dest[i] = x4 * x4 * x2 * x;
        }
        break;
    default:
        for(i = 0; i < num_samples; i++)
            dest[i] = (float)pow(src[i], m);
        break;
    }
}

/** 
 * @brief Stetch image to [0,1] 
 * @param image image data in planar order
 * @param num_pixels number of pixels 
 * 
 * Linearly stretches each color channel so that min and max values are
 * respectively 0 and 1.
 */
static void stretch(float *image, long num_pixels)
{
    int channel;
    
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
    for(channel = 0; channel < 3; channel++)
    {
        float *dest = image + channel * num_pixels;
        float min, max, Scale;
        long n;
        
        get_min_max(&min, &max, dest, num_pixels);
        
        if(min < max)
            for(n = 0, Scale = max - min; n < num_pixels; n++)
                dest[n] = (dest[n] - min) / Scale;
    }
}

/** @brief FFT-based convolution */
static void convolve(float *blurred_trans, const float *omega_trans, 
    long num_pixels, fftwf_plan forward_plan, fftwf_plan inverse_plan)
{
    long i;
    
    fftwf_execute(forward_plan);
    
    for(i = 0; i < num_pixels; i++)
        blurred_trans[i] *= omega_trans[i];
    
    fftwf_execute(inverse_plan);    
}

/** @brief Allocate buffers and FFTW plans for DCT transforms */
static int alloc_buffers_and_plans(float ***buffers_ptr, 
    fftwf_plan (**plans_ptr)[2], int width, int height, int num_buffers)
{    
    const long num_pixels = ((long)width) * ((long)height);
    float **buffers;
    fftwf_plan (*plans)[2];
    int i;
    
    if(!(*buffers_ptr = buffers = (float **)
        malloc(sizeof(float *)*num_buffers)))
        return 0;
    
    for(i = 0; i < num_buffers; i++)
        buffers[i] = NULL;
    
    if(!(*plans_ptr = plans = (fftwf_plan (*)[2])
        malloc(sizeof(fftwf_plan)*2*num_buffers)))
        return 0;
    
    for(i = 0; i < num_buffers; i++)
        plans[i][0] = plans[i][1] = NULL;
    
    /* Allocate a workspace array and create FFTW transform plans.  These will
       be used for computing convolutions in parallel. */
    for(i = 0; i < num_buffers; i++)
        if(!(buffers[i] = (float *)
            fftwf_malloc(sizeof(float)*num_pixels))
            /* DCT-II on buffers[i] */
            || !(plans[i][0] = fftwf_plan_r2r_2d(height, width, buffers[i],
            buffers[i], FFTW_REDFT10, FFTW_REDFT10, FFTW_ESTIMATE))
            /* DCT-III on buffers[i] */
            || !(plans[i][1] = fftwf_plan_r2r_2d(height, width, buffers[i],
            buffers[i], FFTW_REDFT01, FFTW_REDFT01, FFTW_ESTIMATE)))
        return 0;
    
    return 1;
}

/** @brief Free resources allocated by alloc_buffers_and_plans() */
static void free_buffers_and_plans(float **buffers, 
    fftwf_plan (*plans)[2], int num_buffers)
{
    int i;
    
    for(i = 0; i < num_buffers; i++)
    {
        if(plans[i][1])
            fftwf_destroy_plan(plans[i][1]);
        if(plans[i][0])
            fftwf_destroy_plan(plans[i][0]);
        if(buffers[i])
            fftwf_free(buffers[i]);
    }
    
    if(plans)
        free(plans);
    if(buffers)
        free(buffers);
}

/** 
 * @brief Compute binomial coefficient for small m and n 
 * @param m,n binomial arguments
 * @return binomial coefficient \f$ {m \choose n} \f$
 * 
 * Uses factorial() to compute the binomial coefficient
 * 
 * \f[ {m \choose n} := \frac{m!}{n!(m-n)!}. \f]
 */
static double binom_coeff(int m, int n)
{
    return factorial(m) / ( factorial(n) * factorial(m - n));
}

/** 
 * @brief Compute small factorial 
 * @param n factorial argument
 * @return n!
 * 
 * Computes factorial for small \c n.  factorials 0!, 1!, ..., 14! are 
 * hardcoded and recursion is used for general \c n.  Beware that large
 * factorials overflow numerical limits.
 */
static double factorial(int n)
{
    static const double table[15] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320,
        362880, 3628800, 39916800, 479001600, 6227020800, 87178291200};
    
    if(n < 0)
        return 0;
    else if(n < 15)
        return table[n];
    else
        return n * factorial(n - 1);  /* Recursion for general n */
}
