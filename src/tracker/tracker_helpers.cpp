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

#include <stdio.h>

#include "tracker_helpers.h"

#include "opencv2/imgproc/imgproc_c.h"

void
th_stats(double *a, int n, double *variance, double *avg)
{
    double sum, diff;
    int i;

    sum = 0.;
    for (i=0; i<n; i++) {
        sum += a[i];
    }

    *avg = sum / n;

    sum = 0.;
    for (i=0; i<n; i++) {
        diff = a[i] - *avg;
        sum += diff * diff;
    }

    *variance = sum / (n - 1);
}

double
th_color_avg(CvScalar a)
{
    return (a.val[0] + a.val[1] + a.val[2]) / 3.;
}

CvScalar
th_scalar_add(CvScalar a, CvScalar b)
{
    CvScalar result;

    result.val[0] = a.val[0] + b.val[0];
    result.val[1] = a.val[1] + b.val[1];
    result.val[2] = a.val[2] + b.val[2];
    result.val[3] = a.val[3] + b.val[3];

    return result;
}

CvScalar
th_scalar_sub(CvScalar a, CvScalar b)
{
    CvScalar result;

    result.val[0] = a.val[0] - b.val[0];
    result.val[1] = a.val[1] - b.val[1];
    result.val[2] = a.val[2] - b.val[2];
    result.val[3] = a.val[3] - b.val[3];

    return result;
}

CvScalar
th_scalar_mul(CvScalar a, double b)
{
    CvScalar result;

    result.val[0] = a.val[0] * b;
    result.val[1] = a.val[1] * b;
    result.val[2] = a.val[2] * b;
    result.val[3] = a.val[3] * b;

    return result;
}

CvScalar
th_brg2hsv(CvScalar bgr)
{
    static IplImage *img_hsv = NULL;
    static IplImage *img_bgr = NULL;

    /**
     * We use two dummy 1x1 images here, and set/get the color from from these
     * images (using cvSet/cvAvg) in order to be able to use cvCvtColor() and
     * the same algorithm used when using cvCvtColor() on the camera image.
     **/

    if (!img_hsv) {
        img_hsv = cvCreateImage(cvSize(1, 1), IPL_DEPTH_8U, 3);
    }

    if (!img_bgr) {
        img_bgr = cvCreateImage(cvSize(1, 1), IPL_DEPTH_8U, 3);
    }

    cvSet(img_bgr, bgr, NULL);
    cvCvtColor(img_bgr, img_hsv, CV_BGR2HSV);

    return cvAvg(img_hsv, NULL);
}

