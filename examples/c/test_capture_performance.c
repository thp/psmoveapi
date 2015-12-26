
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
save(int i, struct TestContext *context)
{
    void *frame = psmove_tracker_get_frame(context->tracker);
    char path[512];
    snprintf(path, sizeof(path), "capture_perf_%03d.jpg", i);
    int imgParams[] = { CV_IMWRITE_JPEG_QUALITY, 90, 0 };
    cvSaveImage(path, frame, imgParams);
}

int
main(int argc, char *argv[])
{
    struct TestContext context;

    setup(&context);

    printf("\n -- PS Move API Camera Performance Test -- \n");

    printf("\nTesting frame grab performance\n");
    FILE *fp = fopen("capture_grab.csv", "w");
    assert(fp != NULL);
    fprintf(fp, "frame,grab,retrieve,converted");
    int j;
    for (j=0; j<context.count; j++) {
        fprintf(fp, ",tracking%d", j);
    }
    fprintf(fp, ",start,diff\n");
    int i;
    double first = _psmove_timestamp_value(_psmove_timestamp());
    double last = 0;
    for (i=0; i<ITERATIONS; i++) {
        if (i % 50 == 0)
        {
            printf("Capturing Frame %d\n", i);
        }
        psmove_tracker_update_image(context.tracker);
        _psmove_tracker_retrieve_stats(context.tracker,
                &(context.capture_begin),
                &(context.capture_grab),
                &(context.capture_retrieve),
                &(context.capture_converted));

        double start =  _psmove_timestamp_value(_psmove_timestamp());
		double grab = _psmove_timestamp_value(_psmove_timestamp_diff(context.capture_grab, context.capture_begin));
		double retrieve = _psmove_timestamp_value(_psmove_timestamp_diff(context.capture_retrieve, context.capture_grab));
		double converted = _psmove_timestamp_value(_psmove_timestamp_diff(context.capture_converted, context.capture_retrieve));
        fprintf(fp, "%d,%.10f,%.10f,%.10f", i, grab, retrieve, converted);

        for (j=0; j<context.count; j++) {
            PSMove_timestamp track_begin = _psmove_timestamp();
            psmove_tracker_update(context.tracker, context.moves[j]);
            PSMove_timestamp track_end = _psmove_timestamp();
            double tracking = _psmove_timestamp_value(_psmove_timestamp_diff(track_end, track_begin));
            fprintf(fp, ",%.10f", tracking);
        }
        fprintf(fp, ",%.10f,%.10f\n", start, (start-last));

        psmove_tracker_annotate(context.tracker);
        save(i, &context);
        last = start;
    }
    fclose(fp);

    //printf("\nTesting SMART READ performance (rate-limited LED setting)\n");
    //printf("\nTesting BAD READ performance (continous LED setting)\n");
    //printf("\nTesting RAW READ performance (no LED setting)\n");

    printf("%d frames captured in %f seconds (%.1f fps)\n", i, (last-first), i/(last-first));
    printf("\n");

    teardown(&context);
    return 0;
}
