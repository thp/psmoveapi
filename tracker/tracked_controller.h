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
#include <time.h>

struct _TrackedController;
typedef struct _TrackedController TrackedController;

struct _TrackedController {
	PSMove* move;

	CvScalar dColor;			// defined color
	CvScalar eFColor;			// first estimated color (BGR)
	CvScalar eFColorHSV;		// first estimated color (HSV)
	CvScalar eColor;			// estimated color (BGR)
	CvScalar eColorHSV; 		// estimated color (HSV)
	int roi_x, roi_y;			// x/y - Coordinates of the ROI
	int roi_level; 	 			// the current index for the level of ROI
	float mx, my;				// x/y - Coordinates of center of mass of the blob
	float x, y, r;				// x/y - Coordinates of the controllers sphere and its radius
	float rs;					// a smoothed variant of the radius
	int is_tracked;				// 1 if tracked 0 otherwise
	time_t last_color_update;	// the timestamp when the last color adaption has been performed
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

void
tracked_controller_save_colors(TrackedController* head);

int
tracked_controller_load_color(TrackedController* tc);

#endif //__TRACKED_CONTROLLER_H
