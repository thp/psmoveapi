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

//-- includes ----
#include "psmove_quaternion.hpp"

#include <assert.h>

//-- macros ----
#ifdef isfinite
#define is_valid_float(x) (!std::isnan(x) && isfinite(x))
#else
#define is_valid_float(x) (!std::isnan(x))
#endif

#ifdef NDEBUG
#define assert_valid_float(x) assert(is_valid_float(x))
#else
#define assert_valid_float(x)     ((void)0)
#endif

//-- globals ----
const glm::quat g_psmove_quaternion_zero = glm::quat(0, 0, 0, 0);
const glm::quat *k_psmove_quaternion_zero = &g_psmove_quaternion_zero;

const glm::quat g_psmove_quaternion_identity = glm::quat(1, 0, 0, 0);
const glm::quat *k_psmove_quaternion_identity = &g_psmove_quaternion_identity;

//-- public methods ----

glm::quat
psmove_quaternion_safe_divide_with_default(const glm::quat &q, const float divisor, const glm::quat &default_result)
{
    glm::quat q_n;

	if (!is_nearly_zero(divisor))
	{
		q_n = q / divisor;
	}
	else
	{
		q_n = default_result;
	}

	return q_n;
}

glm::quat
psmove_quaternion_normalized_lerp(const glm::quat &a, const glm::quat &b, const float u)
{
    glm::quat q= a*(1.f - u) + b*u;
    
    psmove_quaternion_normalize_with_default(q, g_psmove_quaternion_identity);

    return q;
}

float
psmove_quaternion_normalize_with_default(glm::quat &inout_v, const glm::quat &default_result)
{
	const float magnitude = glm::length(inout_v);
	inout_v = psmove_quaternion_safe_divide_with_default(inout_v, magnitude, default_result);
	return magnitude;
}

bool
psmove_quaternion_is_valid(const glm::quat &q)
{
	return is_valid_float(q.x) && is_valid_float(q.y) && is_valid_float(q.z) && is_valid_float(q.w);
}

glm::vec3
psmove_vector3f_clockwise_rotate(const glm::quat &q, const glm::vec3 &v)
{
	assert_quaternion_is_normalized(q);

	// GLM rotates counterclockwise (i.e. q*v*q^-1), 
	// while we want the inverse of that (q^-1*v*q)
    return glm::conjugate(q) * v;
}