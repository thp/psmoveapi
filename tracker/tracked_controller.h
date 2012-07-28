#ifndef __TRACKED_CONTROLLER_H
#define __TRACKED_CONTROLLER_H

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

#include "opencv2/core/core_c.h"

#include "psmove.h"

struct _TrackedController;
typedef struct _TrackedController TrackedController;

struct _TrackedController {
	PSMove* move;
	// defined color
	CvScalar dColor;
	// estimated color
	CvScalar eColor;
	CvScalar eColorHSV;

	int roi_x;
	int roi_y;
	//CvRect roi; // this saves the current region of interest
	int roi_level; // the current index for the level of ROI

	float x,y,r;
	TrackedController* next;
};

TrackedController*
tracked_controller_create();

void
tracked_controller_release(TrackedController** tc, int whole_list);

TrackedController*
tracked_controller_find(TrackedController* head, PSMove* data);

TrackedController*
tracked_controller_insert(TrackedController** head, PSMove* data);

void
tracked_controller_remove(TrackedController** head, PSMove* data);

#endif //__TRACKED_CONTROLLER_H
