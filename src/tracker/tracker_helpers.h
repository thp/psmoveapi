/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Benjamin Venditti <benjamin.venditti@gmail.com>
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

#ifndef TRACKER_HELPERS_H
#define TRACKER_HELPERS_H

#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"
#include <time.h>

#include "psmove.h"

#define th_odd(x) (((x)/2)*2+1)
#define th_black cvScalarAll(0x0)
#define th_white cvScalar(0xFF,0xFF,0xFF,0)
#define th_green cvScalar(0,0xff,0,0)
#define th_red cvScalar(0,0,0xff,0)
#define th_blue cvScalar(0xff,0,0,0)
#define th_yellow cvScalar(0,0xff,0xff,0)
#define th_min(x,y) ((x)<(y)?(y):(x))
#define th_max(x,y) ((x)<(y)?(x):(y))
#define th_scalar_to_rgb_int(c)(((int)(c).val[2])<<16 && ((int)(c).val[1])<<8 && ((int)(c).val[0]))
#define th_PI 3.14159265358979
#define th_dist_squared(a,b) (pow((a).x-(b).x,2)+pow((a).y-(b).y,2))
#define th_dist(a,b) (sqrt(pow((a).x-(b).x,2)+pow((a).y-(b).y,2)))

#define th_esc_key 27
#define th_space_key 32

// some basic statistical functions on arrays
double th_var(double* src, int len);
double th_avg(double* src, int len);
double th_magnitude(double* src, int len);
void th_minus(double* l, double* r, double* result, int len);
void th_plus(double* l, double* r, double* result, int len);
void th_mul(double* array, double f, double* result, int len);

// only creates the image/storage, if it does not already exist, or has different properties (size, depth, channels, blocksize ...)
// returns (1: if image/storage is created) (0: if nothing has been done)
int th_create_image(IplImage** img, CvSize s, int depth, int channels);
int th_create_mem_storage(CvMemStorage** stor, int block_size);
int th_create_hist(CvHistogram** hist, int dims, int* sizes, int type, float** ranges, int uniform);

void th_put_text(IplImage* img, const char* text, CvPoint p, CvScalar color, float scale);
IplImage* th_plot_hist(CvHistogram* hist, int bins, const char* windowName, CvScalar lineColor);

// simly saves a CvArr* on the filesystem
int th_save_jpg(const char* path, const CvArr* image, int quality);
int th_save_jpgEx(const char* folder, const char* filename, int prefix, const CvArr* image, int quality);

// prints a array to system out ala {a,b,c...}
void th_print_array(double* src, int len);

// converts HSV color to a BGR color and back
CvScalar th_hsv2bgr(CvScalar hsv);
CvScalar th_brg2hsv(CvScalar bgr);

// waits until the uses presses ESC (only works if a windo is visible)
void th_wait_esc();
void th_wait(char c);
void th_wait_move_button(PSMove* move, int button);
int th_move_button(PSMove* move, int button);

void th_equalize_image(IplImage* img);
int th_file_exists(const char* file);
#endif // TRACKER_HELPERS_H
