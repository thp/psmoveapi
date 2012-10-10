
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


#ifndef PSMOVE_FUSION_H
#define PSMOVE_FUSION_H

#include "psmove.h"
#include "psmove_tracker.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Field of view of the PS Eye */
#define PSEYE_FOV_BLUE_DOT 75
#define PSEYE_FOV_RED_DOT 56


#ifndef SWIG
struct _PSMoveFusion;
typedef struct _PSMoveFusion PSMoveFusion;
#endif

ADDAPI PSMoveFusion *
ADDCALL psmove_fusion_new(PSMoveTracker *tracker, float z_near, float z_far);

ADDAPI float *
ADDCALL psmove_fusion_get_projection_matrix(PSMoveFusion *fusion);

ADDAPI float *
ADDCALL psmove_fusion_get_modelview_matrix(PSMoveFusion *fusion, PSMove *move);

ADDAPI void
ADDCALL psmove_fusion_get_position(PSMoveFusion *fusion, PSMove *move,
        float *x, float *y, float *z);

ADDAPI void
ADDCALL psmove_fusion_free(PSMoveFusion *fusion);

#ifdef __cplusplus
};
#endif

#endif /* PSMOVE_FUSION_H */
