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

/* Math functions related to 3d vectors */

//-- includes -----
#include <stdbool.h>
#include <assert.h>

#include "psmove_math.h"
#include "psmove.h"

//-- interface -----
#ifdef __cplusplus
extern "C" {
#endif	
ADDAPI extern const PSMove_3AxisVector *k_psmove_vector_zero;
ADDAPI extern const PSMove_3AxisVector *k_psmove_vector_one;
ADDAPI extern const PSMove_3AxisVector *k_psmove_vector_i;
ADDAPI extern const PSMove_3AxisVector *k_psmove_vector_j;
ADDAPI extern const PSMove_3AxisVector *k_psmove_vector_k;
	
ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_xyz(const float x, const float y, const float z);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_add(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_subtract(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_scale(const PSMove_3AxisVector *v, const float s);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_divide_by_scalar_unsafe(const PSMove_3AxisVector *v, const float divisor);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_divide_by_vector_unsafe(const PSMove_3AxisVector *v, const PSMove_3AxisVector *divisor);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_divide_by_scalar_with_default(const PSMove_3AxisVector *v, const float divisor, const PSMove_3AxisVector *default_result);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_divide_by_vector_with_default(const PSMove_3AxisVector *v, const PSMove_3AxisVector *divisor, const PSMove_3AxisVector *default_result);

ADDAPI float
ADDCALL psmove_3axisvector_dot(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b);

ADDAPI float
ADDCALL psmove_3axisvector_min_component(const PSMove_3AxisVector *v);

ADDAPI float
ADDCALL psmove_3axisvector_max_component(const PSMove_3AxisVector *v);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_min_vector(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_max_vector(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b);

ADDAPI float
ADDCALL psmove_3axisvector_length_squared(const PSMove_3AxisVector *v);

ADDAPI float
ADDCALL psmove_3axisvector_length(const PSMove_3AxisVector *v);

ADDAPI float
ADDCALL psmove_3axisvector_normalize_with_default(PSMove_3AxisVector *inout_v, const PSMove_3AxisVector *default_result);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_3axisvector_apply_transform(const PSMove_3AxisVector *v, const PSMove_3AxisTransform *m);

#ifdef __cplusplus
}
#endif

//-- macros -----
#define assert_vector_is_normalized(v) assert(is_nearly_equal(psmove_vector_length_squared(v), 1.f, k_normal_epsilon))
#define assert_vectors_are_perpendicular(a, b) assert(is_nearly_equal(psmove_vector_dot(a,b), 0.f, k_normal_epsilon))
