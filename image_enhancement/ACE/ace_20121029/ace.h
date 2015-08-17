/**
 * @file ace.h
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

#ifndef ACE_H
#define ACE_H

int ace_enhance_image_interp(float *u, const float *f, 
    int width, int height, float alpha, const char *omega_string, 
    int num_levels);
int ace_enhance_image_poly(float *u, const float *f, 
    int width, int height, float alpha, const char *omega_string, int degree);

#endif /* ACE_H */
