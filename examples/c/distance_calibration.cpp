
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

#include <stdio.h>

#include <time.h>
#include <assert.h>

#include "opencv2/opencv_modules.hpp"
#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"

#include "psmove.h"
#include "psmove_tracker.h"

struct measurement {
    float distance_cm;
    float radius_px;
};
typedef struct measurement measurement;

#define MEASUREMENTS 50
#define MEASUREMENTS_CM_START 20
#define MEASUREMENTS_CM_STEP 5

void
save(void *image, int distance)
{
    char path[512];
    snprintf(path, sizeof(path), "distance_%03d.jpg", distance);
    int imgParams[] = { CV_IMWRITE_JPEG_QUALITY, 90, 0 };
    cvSaveImage(path, image, imgParams);
}

int
main(int arg, char** args)
{
    measurement measurements[MEASUREMENTS];
    float distance = MEASUREMENTS_CM_START;
    int pos = 0;

    PSMove *move = psmove_connect();

    if (move == NULL) {
        printf("Could not connect to the default controller.\n");
        return 1;
    }

    PSMoveTracker* tracker = psmove_tracker_new();

    if (tracker == NULL) {
        printf("Could not create tracker.\n");
        return 2;
    }

    printf("Calibrating controller...\n");
    while (psmove_tracker_enable(tracker, move) != Tracker_CALIBRATED);

    while (cvWaitKey(1) != 27 && pos < MEASUREMENTS) {
        psmove_tracker_update_image(tracker);
        psmove_tracker_update(tracker, NULL);
        printf("Distance: %.2f cm\n", distance);

        void *frame = psmove_tracker_get_frame(tracker);
        cvShowImage("Camera", frame);

        float x, y, radius;
        psmove_tracker_get_position(tracker, move, &x, &y, &radius);

        unsigned int pressed, released;
        while (psmove_poll(move));
        psmove_get_button_events(move, &pressed, &released);
        if (pressed & Btn_CROSS) {
            // Save current measurement
            save(frame, (int)distance);
            measurements[pos].distance_cm = distance;
            measurements[pos].radius_px = radius;
            distance += MEASUREMENTS_CM_STEP;
            pos++;
        } else if (pressed & Btn_CIRCLE && pos > 0) {
            // Go back and retry previous measurement
            distance -= MEASUREMENTS_CM_STEP;
            pos--;
        }
    }

    int i;
    FILE *fp = fopen("distance.csv", "w");
    fprintf(fp, "distance,radius\n");
    for (i=0; i<pos; i++) {
        fprintf(fp, "%.5f,%.5f\n",
                measurements[i].distance_cm,
                measurements[i].radius_px);
    }
    fclose(fp);

    psmove_tracker_free(tracker);
    psmove_disconnect(move);

    return 0;
}

