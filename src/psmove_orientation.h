
 /**
 * PS Move API - An interface for the PS Move Motion Controller
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


#ifndef PSMOVE_ORIENTATION_H
#define PSMOVE_ORIENTATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "psmove.h"


struct _PSMoveOrientation;
typedef struct _PSMoveOrientation PSMoveOrientation;


ADDAPI PSMoveOrientation *
ADDCALL psmove_orientation_new(PSMove *move);

ADDAPI void
ADDCALL psmove_orientation_update(PSMoveOrientation *orientation);

ADDAPI void
ADDCALL psmove_orientation_get_quaternion(PSMoveOrientation *orientation,
        float *q0, float *q1, float *q2, float *q3);

ADDAPI void
ADDCALL psmove_orientation_reset_quaternion(PSMoveOrientation *orientation);

ADDAPI void
ADDCALL psmove_orientation_free(PSMoveOrientation *orientation);


#ifdef __cplusplus
}
#endif

#endif
