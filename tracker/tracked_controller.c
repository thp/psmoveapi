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

#include "psmove.h"

#include "tracked_controller.h"
#include "tracker_helpers.h"

#include "../external/iniparser/iniparser.h"
#include "../external/iniparser/dictionary.h"

#define COLOR_MAPPING_FILE "ColorMappings.ini"

TrackedController*
tracked_controller_create() {
	TrackedController* tc = (TrackedController*) calloc(1, sizeof(TrackedController));

	tc->move = 0x0;

	tc->dColor = cvScalar(0, 0, 0, 0);
	tc->eFColor = cvScalar(0, 0, 0, 0);
	tc->eFColorHSV = cvScalar(0, 0, 0, 0);

	tc->eColor = cvScalar(0, 0, 0, 0);
	tc->eColorHSV = cvScalar(0, 0, 0, 0);

	tc->roi_x = 0;
	tc->roi_y = 0;
	tc->roi_level = 0;

	tc->x = 0;
	tc->y = 0;
	tc->r = 0;
	tc->rs = 0;

	tc->mx = 0;
	tc->my = 0;

	tc->is_tracked = 0;
	tc->last_color_update = 0;

	tc->next = 0x0;
	return tc;
}

void tracked_controller_release(TrackedController** tc, int whole_list) {
	if (whole_list == 0) {
		free(*tc);
	} else {
		TrackedController* tmp = *tc;
		TrackedController* del;
		for (; tmp != 0x0;) {
			del = tmp;
			tmp = tmp->next;
			free(del);
		}
		*tc = 0x0;
	}
}

TrackedController*
tracked_controller_find(TrackedController* head, PSMove* data) {
	TrackedController* tmp = head;
	for (; tmp != 0x0; tmp = tmp->next) {
		if (tmp->move == data)
			return tmp;
	}
	return 0;
}

TrackedController* tracked_controller_insert(TrackedController** head, PSMove* data) {
	TrackedController* tmp = *head;
	TrackedController* last = 0x0;

	for (; tmp != 0x0; tmp = tmp->next) {
		last = tmp;
	}

	TrackedController* item = tracked_controller_create();
	item->move = data;

	if (last == 0x0) {
		// set the head
		*head = item;
	} else {
		// insert at the end
		last->next = item;
	}
	return item;
}

void tracked_controller_remove(TrackedController** head, PSMove* data) {
	TrackedController* tmp = *head;
	TrackedController* delete = 0x0;
	TrackedController* prev = 0x0;

	for (; tmp != 0x0; tmp = tmp->next) {
		if (tmp->move == data) {
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

		tracked_controller_release(&delete, 0);
	}
}

char *
color_mapping_file_filename()
{
    const char *data_dir = psmove_util_get_data_dir();
    char *result = malloc(strlen(data_dir) + strlen(COLOR_MAPPING_FILE) + 1);

    strcpy(result, data_dir);
    strcat(result, COLOR_MAPPING_FILE);

    return result;
}

void tracked_controller_save_colors(TrackedController* head) {
	char key[128];
	char value[128];

        char *filename = color_mapping_file_filename();

	dictionary* ini = iniparser_load(filename);
	iniparser_set(ini, "ColorMapping", 0);

	TrackedController* tmp = head;

	for (; tmp != 0x0; tmp = tmp->next) {
		sprintf(key, "ColorMapping:%X%X%X", (int) tmp->dColor.val[2], (int) tmp->dColor.val[1], (int) tmp->dColor.val[0]);
		sprintf(value, "%X%X%X", (int) tmp->eFColor.val[2], (int) tmp->eFColor.val[1], (int) tmp->eFColor.val[0]);
		iniparser_set(ini, key, value);
	}
	iniparser_save_ini(ini, filename);
	dictionary_del(ini);
        free(filename);
}

int tracked_controller_load_color(TrackedController* tc) {
	int loaded = 0;
	char key[128];
        char *filename = color_mapping_file_filename();
	dictionary* ini = iniparser_load(filename);
        free(filename);
	sprintf(key, "ColorMapping:%X%X%X", (int) tc->dColor.val[2], (int) tc->dColor.val[1], (int) tc->dColor.val[0]);
	if (iniparser_find_entry(ini, key)) {
		char* sValue = iniparser_getstring(ini, key, "");
		int value = 0;
		sscanf(sValue, "%X", &value);
		tc->eFColor.val[2] = value >> 16 & 0xFF;
		tc->eFColor.val[1] = value >> 8 & 0xFF;
		tc->eFColor.val[0] = value & 0xFF;

		tc->eColor.val[2] = tc->eFColor.val[2];
		tc->eColor.val[1] = tc->eFColor.val[1];
		tc->eColor.val[0] = tc->eFColor.val[0];

		tc->eColorHSV = th_brg2hsv(tc->eColor);
		tc->eFColorHSV = th_brg2hsv(tc->eFColor);

		loaded = 1;
	} else
		dictionary_del(ini);
	return loaded;
}

