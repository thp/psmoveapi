
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * Copyright (c) 2012 Benjamin Venditt <benjamin.venditti@gmail.com>
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

#include <stdio.h>

#include <time.h>
#include <assert.h>

#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"

#include "psmove.h"
#include "psmove_tracker.h"


int main(int arg, char** args) {
    int i;
    int count = psmove_count_connected();

    printf("### Found %d controllers.\n", count);
    if (count == 0) {
        return 1;
    }

    PSMove **controllers = (PSMove **)calloc(count, sizeof(PSMove *));

    void *frame;
    int result;

    fprintf(stderr, "Trying to init PSMoveTracker...");
    PSMoveTrackerSettings settings;
    psmove_tracker_settings_set_default(&settings);
    settings.color_mapping_max_age = 0;
	settings.exposure_mode = Exposure_LOW;
	settings.camera_mirror = PSMove_True;
    PSMoveTracker* tracker = psmove_tracker_new_with_settings(&settings);
    if (!tracker)
    {
        fprintf(stderr, "Could not init PSMoveTracker.\n");
        return 1;
    }
    fprintf(stderr, "OK\n");

    for (i=0; i<count; i++) {
        printf("Opening controller %d\n", i);
        controllers[i] = psmove_connect_by_id(i);
        assert(controllers[i] != NULL);

		for (;;) {
            printf("Calibrating controller %d...", i);
            fflush(stdout);
            result = psmove_tracker_enable(tracker, controllers[i]);

            if (result == Tracker_CALIBRATED) {
                enum PSMove_Bool auto_update_leds =
                    psmove_tracker_get_auto_update_leds(tracker,
                            controllers[i]);
                printf("OK, auto_update_leds is %s\n",
                        (auto_update_leds == PSMove_True)?"enabled":"disabled");
                break;
            } else {
                printf("ERROR - retrying\n");
            }
        }
    }

    while ((cvWaitKey(1) & 0xFF) != 27) {
        psmove_tracker_update_image(tracker);
        psmove_tracker_update(tracker, NULL);
        psmove_tracker_annotate(tracker);

        frame = psmove_tracker_get_frame(tracker);
        if (frame) {
            cvShowImage("live camera feed", frame);
        }

        for (i=0; i<count; i++) {
            /* Optional and not required by default (see auto_update_leds above)
            unsigned char r, g, b;
            psmove_tracker_get_color(tracker, controllers[i], &r, &g, &b);
            psmove_set_leds(controllers[i], r, g, b);
            psmove_update_leds(controllers[i]);
            */

            float x, y, r;
            psmove_tracker_get_position(tracker, controllers[i], &x, &y, &r);
            printf("x: %10.2f, y: %10.2f, r: %10.2f\n", x, y, r);
        }
    }

    for (i=0; i<count; i++) {
        psmove_disconnect(controllers[i]);
    }

    psmove_tracker_free(tracker);
    free(controllers);
    return 0;
}

