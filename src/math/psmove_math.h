#pragma once
/**
* PS Move API - An interface for the PS Move Motion Controller
* Copyright (c) 2011, 2012 Thomas Perl <m@thp.io>
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

/* General math utility functions */

//-- includes -----
#include <float.h>
#include <math.h>

//-- constants ----
#define k_real_max FLT_MAX
#define k_real_min FLT_MIN

#define k_positional_epsilon 0.001f
#define k_normal_epsilon 0.0001f
#define k_real_epsilon FLT_EPSILON

#define k_real_pi (float)M_PI
#define k_real_two_pi 2.f*k_real_pi
#define k_real_half_pi 0.5f*k_real_pi

//-- macros ----
#define is_nearly_equal(a, b, epsilon) (fabsf(a-b) <= epsilon)
#define is_nearly_zero(x) is_nearly_equal(x, 0.0f, k_real_epsilon)

#define safe_divide_with_default(numerator, denomenator, default_result) (is_nearly_zero(denomenator) ? (default_result) : (numerator / denomenator));

#ifndef sgn
#define sgn(x) (((x) >= 0) ? 1 : -1)
#endif

#ifndef sqr
#define sqr(x) (x*x)
#endif

//-- inline -----
#ifdef __cplusplus
extern "C" {
#endif	

float clampf(float x, float lo, float hi);
float clampf01(float x);

float lerpf(float a, float b, float u);
float lerp_clampf(float a, float b, float u);

float degrees_to_radians(float x);
float radians_to_degrees(float x);

#ifdef __cplusplus
}
#endif
