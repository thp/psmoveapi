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
#include "psmove.h"
#include "psmove_math.h"
#include "psmove_vector.h"
#include <assert.h>
#include <stdlib.h>

//-- globals ----
const PSMove_3AxisVector g_psmove_vector_zero = {{ {0, 0, 0} }};
const PSMove_3AxisVector *k_psmove_vector_zero = &g_psmove_vector_zero;

const PSMove_3AxisVector g_psmove_vector_one = {{ {1, 1, 1} }};
const PSMove_3AxisVector *k_psmove_vector_one = &g_psmove_vector_one;

const PSMove_3AxisVector g_psmove_vector_i = {{ {1, 0, 0} }};
const PSMove_3AxisVector *k_psmove_vector_i = &g_psmove_vector_i;

const PSMove_3AxisVector g_psmove_vector_j = {{ {0, 1, 0} }};
const PSMove_3AxisVector *k_psmove_vector_j = &g_psmove_vector_j;

const PSMove_3AxisVector g_psmove_vector_k = {{ {0, 0, 1} }};
const PSMove_3AxisVector *k_psmove_vector_k = &g_psmove_vector_k;

//-- public methods ----
PSMove_3AxisVector
psmove_3axisvector_xyz(const float x, const float y, const float z)
{
	PSMove_3AxisVector v = {{ { x, y, z } }};

	return v;
}

PSMove_3AxisVector
psmove_3axisvector_add(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b)
{
	PSMove_3AxisVector v;

	v.x = a->x + b->x;
	v.y = a->y + b->y;
	v.z = a->z + b->z;

	return v;
}

PSMove_3AxisVector
psmove_3axisvector_subtract(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b)
{
	PSMove_3AxisVector v;

	v.x = a->x - b->x;
	v.y = a->y - b->y;
	v.z = a->z - b->z;

	return v;
}

PSMove_3AxisVector
psmove_3axisvector_scale(const PSMove_3AxisVector *v, const float s)
{
	PSMove_3AxisVector w;

	w.x = v->x * s;
	w.y = v->y * s;
	w.z = v->z * s;

	return w;
}

PSMove_3AxisVector
psmove_3axisvector_divide_by_scalar_unsafe(const PSMove_3AxisVector *v, const float divisor)
{
	PSMove_3AxisVector result;

	result.x = v->x / divisor;
	result.y = v->y / divisor;
	result.z = v->z / divisor;

	return result;
}

PSMove_3AxisVector
psmove_3axisvector_divide_by_vector_unsafe(const PSMove_3AxisVector *v, const PSMove_3AxisVector *divisor)
{
	PSMove_3AxisVector result;

	result.x = v->x / divisor->x;
	result.y = v->y / divisor->y;
	result.z = v->z / divisor->z;

	return result;
}

PSMove_3AxisVector
psmove_3axisvector_divide_by_scalar_with_default(const PSMove_3AxisVector *v, const float divisor, const PSMove_3AxisVector *default_result)
{
	PSMove_3AxisVector w;

	if (!is_nearly_zero(divisor))
	{
		w.x = v->x / divisor;
		w.y = v->y / divisor;
		w.z = v->z / divisor;
	}
	else
	{
		w = *default_result;
	}

	return w;
}

PSMove_3AxisVector
psmove_3axisvector_divide_by_vector_with_default(const PSMove_3AxisVector *v, const PSMove_3AxisVector *divisor, const PSMove_3AxisVector *default_result)
{
	PSMove_3AxisVector result;

	result.x = safe_divide_with_default(v->x, divisor->x, default_result->x);
	result.y = safe_divide_with_default(v->y, divisor->y, default_result->y);
	result.z = safe_divide_with_default(v->z, divisor->z, default_result->z);

	return result;
}

float
psmove_3axisvector_dot(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b)
{
	return (a->x*b->x + a->y*b->y + a->z*b->z);
}

float
psmove_3axisvector_min_component(const PSMove_3AxisVector *v)
{
	return (float)fmin((float)fmin(v->x, v->y), v->z);
}

float
psmove_3axisvector_max_component(const PSMove_3AxisVector *v)
{
	return (float)fmax((float)fmax(v->x, v->y), v->z);
}

PSMove_3AxisVector
psmove_3axisvector_min_vector(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b)
{
	return psmove_3axisvector_xyz((float)fmin(a->x, b->x), (float)fmin(a->y, b->y), (float)fmin(a->z, b->z));
}

PSMove_3AxisVector
psmove_3axisvector_max_vector(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b)
{
	return psmove_3axisvector_xyz((float)fmax(a->x, b->x), (float)fmax(a->y, b->y), (float)fmax(a->z, b->z));
}

float
psmove_3axisvector_length_squared(const PSMove_3AxisVector *v)
{
	return psmove_3axisvector_dot(v,v);
}

float
psmove_3axisvector_length(const PSMove_3AxisVector *v)
{
	return sqrtf(psmove_3axisvector_dot(v, v));
}

float
psmove_3axisvector_normalize_with_default(PSMove_3AxisVector *inout_v, const PSMove_3AxisVector *default_result)
{
	const float length = psmove_3axisvector_length(inout_v);

	*inout_v = psmove_3axisvector_divide_by_scalar_with_default(inout_v, length, default_result);

	return length;
}

float
psmove_radians_between_vectors(const PSMove_3AxisVector *a, const PSMove_3AxisVector *b)
{
	float divisor = psmove_3axisvector_length(a)*psmove_3axisvector_length(b);
	assert(false); // no unit test

	return !is_nearly_zero(divisor) ? acosf(psmove_3axisvector_dot(a, b) / divisor) : 0.f;
}

PSMove_3AxisVector
psmove_3axisvector_apply_transform(const PSMove_3AxisVector *v, const PSMove_3AxisTransform *m)
{
	PSMove_3AxisVector result =
		psmove_3axisvector_xyz(
			m->row0[0]*v->x + m->row0[1]*v->y + m->row0[2]*v->z,
			m->row1[0]*v->x + m->row1[1]*v->y + m->row1[2]*v->z,
			m->row2[0]*v->x + m->row2[1]*v->y + m->row2[2]*v->z);

	return result;
}
