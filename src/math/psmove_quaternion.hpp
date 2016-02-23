
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

/* Math functions related to quaternions */
#ifndef __PSMOVE_QUATERNION_H
#define __PSMOVE_QUATERNION_H

//-- includes -----
#include "psmove_glm_math.hpp"
#include "psmove_vector.h"

//-- pre-declarations ----

//-- structures -----

//-- constants -----
extern const glm::quat *k_psmove_quaternion_zero;
extern const glm::quat *k_psmove_quaternion_identity;

//-- interface -----

glm::quat
psmove_quaternion_safe_divide_with_default(const glm::quat &q, const float divisor, const glm::quat &default_result);

glm::quat
psmove_quaternion_normalized_lerp(const glm::quat &a, const glm::quat &b, const float u);

float
psmove_quaternion_normalize_with_default(glm::quat &inout_v, const glm::quat &default_result);

bool
psmove_quaternion_is_valid(const glm::quat &q);

glm::vec3
psmove_vector3f_clockwise_rotate(const glm::quat &q, const glm::vec3 &v);

//-- macros -----
#define assert_quaternion_is_normalized(q) assert(is_nearly_equal(glm::dot(q,q), 1.f, k_normal_epsilon))

#endif // __PSMOVE_QUATERNION_H