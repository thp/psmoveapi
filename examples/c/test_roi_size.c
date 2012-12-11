
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



#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "psmove.h"
#include "psmove_tracker.h"

/* This performance test application uses private APIs */
#include "../../src/psmove_private.h"
#include "../../src/tracker/psmove_tracker_private.h"

/* OpenCV used for saving screenshots */
#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"

struct TestContext {
    int count;
    PSMoveTracker *tracker;
    PSMove **moves;

    /* Capture timestamps */
    PSMove_timestamp capture_begin;
    PSMove_timestamp capture_grab;
    PSMove_timestamp capture_retrieve;
    PSMove_timestamp capture_converted;
};

void
setup(struct TestContext *context)
{
    int i;

    context->count = psmove_count_connected();
    context->tracker = psmove_tracker_new();
    psmove_tracker_set_mirror(context->tracker, PSMove_True);
    context->moves = calloc(context->count, sizeof(PSMove *));

    for (i=0; i<context->count; i++) {
        context->moves[i] = psmove_connect_by_id(i);
        assert(context->moves[i]);
        while (psmove_tracker_enable(context->tracker,
                    context->moves[i]) != Tracker_CALIBRATED);
    }

    _psmove_tracker_fix_roi_size(context->tracker);
}

void
teardown(struct TestContext *context)
{
    int i;

    for (i=0; i<context->count; i++) {
        psmove_tracker_disable(context->tracker,
                context->moves[i]);
        psmove_disconnect(context->moves[i]);
    }

    free(context->moves);
    psmove_tracker_free(context->tracker);
}

#define ITERATIONS 500

void
save(int i, int roi_size, struct TestContext *context)
{
    void *frame = psmove_tracker_get_frame(context->tracker);
    char path[512];
    snprintf(path, sizeof(path), "roi_perf_%03d_%03d.jpg", roi_size, i);
    int imgParams[] = { CV_IMWRITE_JPEG_QUALITY, 90, 0 };
    cvSaveImage(path, frame, imgParams);
}

int
main(int argc, char *argv[])
{
    printf("\n -- PS Move API ROI Performance Test -- \n\n");
    int roi_sizes[] = {480, 240, 120};
    int roi;
    int rois = sizeof(roi_sizes)/sizeof(roi_sizes[0]);

    float data[ITERATIONS][rois];
    float position[ITERATIONS][rois][3]; // x, y, r

    /**
     * Test file for this test is available from:
     * http://code.google.com/p/moveonpc/downloads/detail?name=test_roi_size.avi
     *
     * You need to have exactly one controller connected.
     **/
    putenv(PSMOVE_TRACKER_FILENAME_ENV "=test_roi_size.avi");
    putenv(PSMOVE_TRACKER_COLOR_ENV "=723a8c");

    for (roi=0; roi<rois; roi++) {
        printf("Testing tracking performance: %d\n", roi_sizes[roi]);

        char tmp[strlen(PSMOVE_TRACKER_ROI_SIZE_ENV) + 10];
        sprintf(tmp, "%s=%d", PSMOVE_TRACKER_ROI_SIZE_ENV, roi_sizes[roi]);
        putenv(tmp);

        struct TestContext context;
        setup(&context);
        assert(context.count == 1);

        int i;
        for (i=0; i<ITERATIONS; i++) {
            psmove_tracker_update_image(context.tracker);

            int j;
            for (j=0; j<context.count; j++) {
                PSMove_timestamp track_begin = _psmove_timestamp();
                psmove_tracker_update(context.tracker, context.moves[j]);
                PSMove_timestamp track_end = _psmove_timestamp();

                float tracking = _psmove_timestamp_value(_psmove_timestamp_diff(track_end, track_begin));
                data[i][roi] = tracking;

                float x, y, r;
                psmove_tracker_get_position(context.tracker, context.moves[j], &x, &y, &r);
                position[i][roi][0] = x;
                position[i][roi][1] = y;
                position[i][roi][2] = r;
            }

            psmove_tracker_annotate(context.tracker);
            if (i % 50 == 0) {
                save(i, roi_sizes[roi], &context);
            }
        }

        teardown(&context);
    }

    FILE *fp = fopen("roi_size.csv", "w");
    assert(fp != NULL);

    /* Header */
    fprintf(fp, "frame");
    int j;
    for (roi=0; roi<rois; roi++) {
        fprintf(fp, ",roi%d", roi_sizes[roi]);
    }
    for (roi=0; roi<rois; roi++) {
        fprintf(fp, ",x%d,y%d,r%d",
                roi_sizes[roi],
                roi_sizes[roi],
                roi_sizes[roi]);
    }
    fprintf(fp, "\n");

    /* Data */
    int i;
    for (i=0; i<ITERATIONS; i++) {
        fprintf(fp, "%d", i);
        for (roi=0; roi<rois; roi++) {
            fprintf(fp, ",%.10f", data[i][roi]);
        }
        for (roi=0; roi<rois; roi++) {
            fprintf(fp, ",%.10f,%.10f,%.10f",
                    position[i][roi][0],
                    position[i][roi][1],
                    position[i][roi][2]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);

    return 0;
}

