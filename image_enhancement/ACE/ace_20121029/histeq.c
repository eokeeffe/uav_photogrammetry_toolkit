/**
 * @file histeq.c
 * @brief Histogram equalization
 * @author Pascal Getreuer <getreuer@gmail.com>
 *
 * 
 * Copyright (c) 2011-2012, Pascal Getreuer
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "imageio.h"

#define DEFAULT_NUMBINS           256

/** @brief struct of program parameters */
typedef struct
{
    /** @brief Input file name */
    char *input_file;
    /** @brief Output file name */
    char *output_file;
    /** @brief Quality for saving JPEG images (0 to 100) */
    int jpeg_quality;
    
    /** @brief Number of histogram bins */
    long num_bins;
} program_params;


static int parse_params(program_params *param, int argc, char *argv[]);

static void print_usage()
{
    puts("Histogram equalization, P. Getreuer 2011\n");
    puts("Usage: histeq [options] <input file> <output file>\n"
        "Only " READIMAGE_FORMATS_SUPPORTED " images are supported.\n");
    puts("Options:\n");
    puts("   -b <number>     number of histogram bins (default 256)");
#ifdef USE_LIBJPEG
    puts("   -q <number>     quality for saving JPEG images (0 to 100)\n");
#endif
}


int equalize_image(float *image, int width, int height, int num_bins)
{       
    const long num_pixels = ((long)width) * ((long)height);
    const long num_el = 3 * num_pixels;
    const int num_bins_minus_one = num_bins - 1;
    float *histogram1d[3] = {NULL, NULL, NULL};
    double accum;
    long i;
    int n, channel, success = 0;
    
    if(!(histogram1d[0] = (float *)Malloc(sizeof(float)*3*num_bins)))
        goto fail;
    
    histogram1d[1] = histogram1d[0] + num_bins;
    histogram1d[2] = histogram1d[0] + 2*num_bins;
    
    for(channel = 0; channel < 3; channel++)
        for(i = 0; i < num_bins; i++)
            histogram1d[channel][i] = 0;
    
    /* Accumate channel histograms */
    for(i = 0; i < num_el; i += 3)
    {
        histogram1d[0][(int)(image[i + 0]*num_bins_minus_one + 0.5f)]++;
        histogram1d[1][(int)(image[i + 1]*num_bins_minus_one + 0.5f)]++;
        histogram1d[2][(int)(image[i + 2]*num_bins_minus_one + 0.5f)]++;
    }
    
    for(channel = 0; channel < 3; channel++)
        for(i = 0; i < num_bins; i++)
            histogram1d[channel][i] /= num_pixels;
    
    /* Convert histograms to equalization maps */
    for(channel = 0; channel < 3; channel++)
    {
        for(n = 0, accum = 0; n < num_bins; n++)
        {
            accum += histogram1d[channel][n];
            histogram1d[channel][n] = accum - histogram1d[channel][n]/2;
        }
        
        histogram1d[channel][0] = 0;
        histogram1d[channel][num_bins - 1] = 1;
    }

    /* Equalize the image */
    for(i = 0; i < num_el; i += 3)
    {
        image[i + 0] = histogram1d[0][
            (int)(image[i + 0]*num_bins_minus_one + 0.5f)];
        image[i + 1] = histogram1d[1][
            (int)(image[i + 1]*num_bins_minus_one + 0.5f)];
        image[i + 2] = histogram1d[2][
            (int)(image[i + 2]*num_bins_minus_one + 0.5f)];
    }
    
    success = 1;
fail:    
    Free(histogram1d[0]);
    return success;
}


int main(int argc, char **argv)
{
    program_params param;
    float *image = NULL;
    unsigned long time_start;
    int width, height, status = 0;
    
    if(!parse_params(&param, argc, argv))
        return 0;
    
    if(!(image = (float *)read_image(&width, &height, param.input_file,
        IMAGEIO_RGB | IMAGEIO_FLOAT)))
        goto fail;
    
    time_start = Clock();
    
    if(!equalize_image(image, width, height, param.num_bins))
        goto fail;    
    
    printf("CPU Time: %.3f s\n", 0.001f*(Clock() - time_start));
    
    if(!write_image(image, width, height, param.output_file,
        IMAGEIO_RGB | IMAGEIO_FLOAT, param.jpeg_quality))
        goto fail;
    
    status = 0;
fail:
    if(image)
        free(image);
    return status;
}


static int parse_params(program_params *param, int argc, char *argv[])
{
    static char *default_output_file = (char *)"out.png";
    char *option_string;
    char option_char;
    int i;


    if(argc < 2)
    {
        print_usage();
        return 0;
    }

    /* Set parameter defaults */
    param->input_file = 0;
    param->output_file = default_output_file;
    param->jpeg_quality = 85;
    param->num_bins = DEFAULT_NUMBINS;
    
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
            case 'b':
                param->num_bins = atoi(option_string);
                
                if(param->num_bins <= 1 || param->num_bins > 10000)
                {
                    ErrorMessage("Number of bins must be between 2 and 10000.\n");
                    return 0;
                }
                break;            
#ifdef LIBJPEG_SUPPORT
            case 'q':
                param->jpeg_quality = atoi(option_string);

                if(param->jpeg_quality <= 0 || param->jpeg_quality > 100)
                {
                    fprintf(stderr, "JPEG quality must be between 0 and 100.\n");
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
