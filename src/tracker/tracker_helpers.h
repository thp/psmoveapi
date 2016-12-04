#pragma once
/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Benjamin Venditti <benjamin.venditti@gmail.com>
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 **/


#include "opencv2/core/core_c.h"

#include "psmove.h"


/* Color constants */
#define TH_COLOR_BLACK cvScalar(0, 0, 0, 0)
#define TH_COLOR_WHITE cvScalar(255, 255, 255, 0)
#define TH_COLOR_RED cvScalar(0, 0, 255, 0)

/* Distance of 2 CvPoints, squared */
#define th_dist_squared(a, b) (((a).x - (b).x) * \
                               ((a).x - (b).x) + \
                               ((a).y - (b).y) * \
                               ((a).y - (b).y))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Variance and average of an array of doubles
 *
 * a ... An array of double values (IN)
 * n ... The number of values in a
 * variance ... Variance result (OUT)
 * avg ... Average result (OUT)
 **/
void
th_stats(double *a, int n, double *variance, double *avg);

/**
 * Average over all 3 channels of a color
 **/
double
th_color_avg(CvScalar a);

/**
 * Sum of two CvScalar values
 **/
CvScalar
th_scalar_add(CvScalar a, CvScalar b);

/**
 * Difference of two CvScalar values
 **/
CvScalar
th_scalar_sub(CvScalar a, CvScalar b);


/**
 * Scaled CvScalar by a constant factor
 **/
CvScalar
th_scalar_mul(CvScalar a, double b);

/**
 * Single-value colorspace conversion: BGR -> HSV
 **/
CvScalar
th_brg2hsv(CvScalar bgr);

#ifdef __cplusplus
}
#endif
