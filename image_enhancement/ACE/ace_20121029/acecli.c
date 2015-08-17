/**
 * @file acecli.c
 * @brief ACE automatic color enhancement command line program
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

/**
 * @mainpage
 * @verbinclude readme.txt
 */

#include <math.h>
#include <string.h>
#include <ctype.h>
#ifdef _OPENMP
#include "omp.h"
#endif
#include "ace.h"
#include "imageio.h"

#define VERBOSE 0


/** @brief struct of program parameters */
typedef struct
{    
    /** @brief Input file name */
    char *input_file;
    /** @brief Output file name */
    char *output_file;
    /** @brief Quality for saving JPEG images (0 to 100) */
    int jpeg_quality;   
    /** @brief Slope parameter, larger implies stronger in enhancement */
    float alpha;
    /** @brief Spatial weighting \f$ \omega(x,y) \f$ */
    char *omega_string;
    /** @brief method: polynomial or interpolation */
    char *method;
    /** @brief method param */
    int method_param;
} program_params;


int parse_params(program_params *param, int argc, char *argv[]);

/** @brief Print program usage help message */
void print_usage()
{
    puts("ACE automatic color enhancement, P. Getreuer 2012");
#ifdef _OPENMP
    printf("Using OpenMP with %d threads\n", omp_get_max_threads());
#endif    
    puts("\nSyntax: ace [options] <input file> <output file>\n\n"
        "Only " READIMAGE_FORMATS_SUPPORTED " images are supported.\n");
    puts("Options:");    
    puts("  -a <number>  alpha, stronger implies stronger enhancement");    
    puts("  -w <omega>   omega, spatial weighting function, choices are");
    puts("               1/r      default ACE, omega(x,y) = 1/sqrt(x^2+y^2)");
    puts("               1        constant, omega(x,y) = 1");
    puts("               G:#      Gaussian, where # specifies sigma,");
    puts("                        omega(x,y) = exp(-(x^2+y^2)/(2 sigma^2))");
    puts("  -m <method>  method to use for fast computation, choices are");
    puts("               interp:# interpolate s_a(L - I(x)) with # levels\n");
    puts("               poly:#   polynomial s_a with degree #");
#ifdef USE_LIBJPEG
    puts("  -q <number>  quality for saving JPEG images (0 to 100)\n");
#endif
    puts("Example: ");
    puts("   ace -a 5 -w 1/r -m interp:12 input.bmp output.bmp\n");
}

#include "omp.h"

int main(int argc, char *argv[])
{
    program_params param;
    float *f = NULL, *u = NULL;
    unsigned long time_start;
    int width, height;
    int status = 1, success;
    
    if(!parse_params(&param, argc, argv))
        return 0;
    
    /* Read the input image */
    if(!(f = (float *)read_image(&width, &height, param.input_file,
        IMAGEIO_FLOAT | IMAGEIO_PLANAR | IMAGEIO_RGB)))
        goto fail;

    /* Allocate the output image */
    if(!(u = (float *)Malloc(sizeof(float)*3*
        ((long int)width)*((long int)height))))
        goto fail;
    
    printf("Enhancing %dx%d image, alpha = %.4f, omega = %s\n", 
        width, height, param.alpha, param.omega_string);
#ifdef _OPENMP
    printf("Using OpenMP with %d threads\n", omp_get_max_threads());
#endif
    time_start = Clock();
        
    /* ACE enhancement */
    if(!param.method || !strcmp(param.method, "interp"))
    {
        printf("Interpolation with %d levels\n", param.method_param);
        success = ace_enhance_image_interp(u, f, width, height, 
            param.alpha, param.omega_string, param.method_param);
    }
    else
    {
        printf("Degree %d polynomial approximation\n", param.method_param);
        success = ace_enhance_image_poly(u, f, width, height, 
            param.alpha, param.omega_string, param.method_param);
    }
    
    if(!success)
    {
        ErrorMessage("Error in computation.\n");
        goto fail;  
    }
    
    printf("CPU Time: %.3f s\n", 0.001f*(Clock() - time_start));
    
    /* Write the output image */
    if(!write_image(u, width, height, param.output_file, 
        IMAGEIO_FLOAT | IMAGEIO_PLANAR | IMAGEIO_RGB, param.jpeg_quality))
        goto fail;
#if VERBOSE > 0
    else
        printf("Output written to \"%s\".\n", param.output_file);
#endif
    
    status = 0;	/* Finished successfully, set exit status to zero. */
    
fail:
    Free(u);
    Free(f);
    return status;
}


int parse_params(program_params *param, int argc, char *argv[])
{
    static char *default_output_file = (char *)"out.bmp";
    static char *default_omega = (char *)"1/r";
    char *option_string;
    char option_char;
    int i;

    if(argc < 2)
    {
        print_usage();
        return 0;
    }

    /* Set parameter defaults */
    param->input_file = NULL;
    param->output_file = default_output_file;
    param->jpeg_quality = 85;
    param->alpha = 5;
    param->omega_string = default_omega;
    param->method = NULL;
    param->method_param = 8;
    
    for(i = 1; i < argc;)
    {
        if(argv[i] && argv[i][0] == '-')
        {
            if((option_char = argv[i][1]) == 0)
            {
                ErrorMessage("Invalid parameter format.\n");
                return 0;
            }

            if(argv[i][2])
                option_string = &argv[i][2];
            else if(++i < argc)
                option_string = argv[i];
            else
            {
                ErrorMessage("Invalid parameter format.\n");
                return 0;
            }
            
            switch(option_char)
            {
            case 'a':   /* Read slope parameter alpha */
                param->alpha = atof(option_string);
                
                if(param->alpha < 1 || param->alpha > 8)
                {
                    ErrorMessage("alpha must be between 1 and 8.\n");
                    return 0;
                }
                break;
            case 'w':   /* Read spatial weighting omega */
                param->omega_string = option_string;
                break;
            case 'm':   /* Read method string */
                {
                    char *method_param;
                    param->method = option_string;
                    
                    if((method_param = strchr(param->method, ':')))
                    {
                        *(method_param++) = '\0';
                        param->method_param = atoi(method_param);
                    }
                    else
                        param->method_param = -1;
                    
                    if(!strcmp(param->method, "interp"))
                    {
                        if(param->method_param == -1)
                            param->method_param = 8;
                        else if(param->method_param < 2)
                        {
                            ErrorMessage("Interpolation levels must be"
                                " at least 2.\n");
                            return 0;
                        }
                    }
                    else if(!strcmp(param->method, "poly"))
                    {
                        if(param->method_param == -1)
                            param->method_param = 9;
                        else if(param->method_param % 2 == 0 
                            || param->method_param < 3
                            || param->method_param > 11)
                        {
                            ErrorMessage("Polynomial degree must be"
                                " 3, 5, 7, 9, or 11.\n");
                            return 0;
                        }
                    }
                    else
                    {
                        ErrorMessage("Unknown method \"%s\".\n", 
                            param->method);
                        return 0;
                    }
                }
                break;
                
#ifdef USE_LIBJPEG
            case 'q':
                param->jpeg_quality = atoi(option_string);

                if(param->jpeg_quality <= 0 || param->jpeg_quality > 100)
                {
                    ErrorMessage("JPEG quality must be between 0 and 100.\n");
                    return 0;
                }
                break;
#endif
            case '-':
                print_usage();
                return 0;
            default:
                if(isprint(option_char))
                    ErrorMessage("Unknown option \"-%c\".\n", option_char);
                else
                    ErrorMessage("Unknown option.\n");

                return 0;
            }

            i++;
        }
        else
        {
            if(!param->input_file)
                param->input_file = argv[i];
            else
                param->output_file = argv[i];

            i++;
        }
    }
    
    if(!param->input_file)
    {
        print_usage();
        return 0;
    }

    return 1;
}
