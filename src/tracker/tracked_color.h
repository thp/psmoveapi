#ifndef __TRACKED_COLOR_H
#define __TRACKED_COLOR_H

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

struct _PSMoveTrackingColor;
typedef struct _PSMoveTrackingColor PSMoveTrackingColor;

struct _PSMoveTrackingColor {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	int is_used;
	PSMoveTrackingColor* next;
};

PSMoveTrackingColor*
tracked_color_create();

void
tracked_color_release(PSMoveTrackingColor** tc, int whole_list);

PSMoveTrackingColor*
tracked_color_find(PSMoveTrackingColor* head, unsigned char r, unsigned char g, unsigned char b);

PSMoveTrackingColor*
tracked_color_insert(PSMoveTrackingColor** head, unsigned char r, unsigned char g, unsigned char b);

void
tracked_color_remove(PSMoveTrackingColor** head, unsigned char r, unsigned char g, unsigned char b);

#endif //__TRACKED_COLOR_H
