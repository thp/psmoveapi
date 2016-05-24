
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

#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"

#include "psmove.h"
#include "psmove_tracker.h"

int main(int arg, char** args) {
    int count = psmove_count_connected();

    int i;
    void *frame;

    if (count == 0) {
        printf("No controllers connected.\n");
        return 1;
    }
    PSMove **moves = (PSMove **)calloc(count, sizeof(PSMove *));

    PSMoveTracker* tracker = psmove_tracker_new();

    for (i=0; i<count; i++) {
        moves[i] = psmove_connect_by_id(i);
        assert(moves[i] != NULL);

        while (psmove_tracker_enable(tracker, moves[i])
                != Tracker_CALIBRATED);
    }

    unsigned char r, g, b;
    psmove_tracker_get_camera_color(tracker, moves[0], &r, &g, &b);
    printf("Controller color: %02x%02x%02x\n", r, g, b);

    CvVideoWriter *writer = cvCreateVideoWriter("out.avi",
            CV_FOURCC('M','J','P','G'), 30, cvSize(640, 480), 1);

    while ((cvWaitKey(1) & 0xFF) != 27) {
        psmove_tracker_update_image(tracker);
        psmove_tracker_update(tracker, NULL);

        frame = psmove_tracker_get_frame(tracker);
        if (frame) {
            cvWriteFrame(writer, frame);
        }

        psmove_tracker_annotate(tracker);
        frame = psmove_tracker_get_frame(tracker);
        if (frame) {
            cvShowImage("live camera feed", frame);
        }
    }

    cvReleaseVideoWriter(&writer);

    for (i=0; i<count; i++) {
        psmove_disconnect(moves[i]);
    }

    psmove_tracker_free(tracker);
    free(moves);
    return 0;
}

