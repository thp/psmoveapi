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

#include "tracked_color.h"

PSMoveTrackingColor*
tracked_color_create() {
	PSMoveTrackingColor* tc = (PSMoveTrackingColor*) calloc(1, sizeof(PSMoveTrackingColor));

	tc->r = 0;
	tc->g = 0;
	tc->b = 0;
	tc->is_used = 0;
	tc->next = 0;
	return tc;
}

void tracked_color_release(PSMoveTrackingColor** tc, int whole_list) {
	if (whole_list == 0) {
		free(*tc);
	} else {
		PSMoveTrackingColor* tmp = *tc;
		PSMoveTrackingColor* del;
		for (; tmp != 0x0;) {
			del = tmp;
			tmp = tmp->next;
			free(del);
		}
		*tc = 0x0;
	}
}

PSMoveTrackingColor*
tracked_color_find(PSMoveTrackingColor* head, unsigned char r, unsigned char g, unsigned char b) {
	PSMoveTrackingColor* tmp = head;
	for (; tmp != 0x0; tmp = tmp->next) {
		if (tmp->r == r && tmp->g == g && tmp->b == b)
			return tmp;
	}
	return 0;
}

PSMoveTrackingColor* tracked_color_insert(PSMoveTrackingColor** head, unsigned char r, unsigned char g, unsigned char b) {
	PSMoveTrackingColor* tmp = *head;
	PSMoveTrackingColor* last = 0x0;

	for (; tmp != 0x0; tmp = tmp->next) {
		last = tmp;
	}

	PSMoveTrackingColor* item = tracked_color_create();
	item->r = r;
	item->g = g;
	item->b = b;

	if (last == 0x0) {
		// set the head
		*head = item;
	} else {
		// insert at the end
		last->next = item;
	}
	return item;
}

void tracked_color_remove(PSMoveTrackingColor** head, unsigned char r, unsigned char g, unsigned char b) {
	PSMoveTrackingColor* tmp = *head;
	PSMoveTrackingColor* delete = 0x0;
	PSMoveTrackingColor* prev = 0x0;

	for (; tmp != 0x0; tmp = tmp->next) {
		if (tmp->r == r && tmp->g == g && tmp->b == b) {
			delete = tmp;
			break;
		}
		prev = tmp;
	}

	if (delete != 0x0) {
		// unhinge the item (fix the pointers)
		if (prev != 0x0)
			prev->next = delete->next;

		if (delete == *head) {
			*head = delete->next;
		}

		tracked_color_release(&delete, 0);
	}
}

