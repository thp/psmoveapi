
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
#include <math.h>

#include "psmove.h"
#include "psmove_tracker.h"

/* This performance test application uses private APIs */
#include "../../src/psmove_private.h"
#include "../../src/tracker/psmove_tracker_private.h"

/* OpenCV used for saving screenshots */
#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"

#define STABLE_FRAMES_COUNT 5

enum TestState {
    TEST_INTIALIZED,
    /* switch LEDs on */
    WAIT_FOR_STABLE_TRACKING,
    /* switch LEDs off */
    WAIT_FOR_TRACKING_LOSS,
    /* switch LEDs on */
    WAIT_FOR_TRACKING_RECOVERY,
    /* switch LEDs off */
    TEST_FINISHED,
};

struct Position {
    float x;
    float y;
    float radius;
};

struct TrackingTimestamps {
    PSMove_timestamp begin;
    PSMove_timestamp grab;
    PSMove_timestamp retrieve;
    PSMove_timestamp converted;
    PSMove_timestamp tracked;
};

struct TestContext {
    int running;

    PSMoveTracker *tracker;
    PSMove *move;

    enum TestState state;

    PSMove_timestamp tracking_1_found;
    struct Position tracking_1_position;
    struct Position tracking_1_position_stable;
    int tracking_1_frame;
    PSMove_timestamp tracking_1_stable;

    PSMove_timestamp tracking_2_leds_off;
    int tracking_2_frame;

    struct TrackingTimestamps tracking_3_lost;
    int tracking_3_frame;
    PSMove_timestamp tracking_3_stable;

    PSMove_timestamp tracking_4_leds_on;
    int tracking_4_frame;

    struct TrackingTimestamps tracking_5_recovery;
    int tracking_5_frame;
    struct Position tracking_5_position;
    struct Position tracking_5_position_stable;
    PSMove_timestamp tracking_5_stable;

    enum PSMoveTracker_Status tracker_status;

    int counter;
    int frame;
};

void
setup(struct TestContext *context)
{
    context->running = 1;

    assert(context->tracker);

    context->move = psmove_connect();
    assert(context->move);

    context->state = TEST_INTIALIZED;
    context->counter = 0;
    context->frame = 0;

    while (psmove_tracker_enable(context->tracker,
                context->move) != Tracker_CALIBRATED);

    psmove_tracker_set_auto_update_leds(context->tracker,
            context->move, PSMove_False);
}

void
teardown(struct TestContext *context)
{
    psmove_tracker_disable(context->tracker, context->move);
    psmove_disconnect(context->move);
}

void
save(int i, int initial, struct TestContext *context)
{
    void *frame = psmove_tracker_get_frame(context->tracker);
    char path[512];
    snprintf(path, sizeof(path), "end2end_%03d_%s.jpg", i,
            initial?"initial":"recovery");
    int imgParams[] = { CV_IMWRITE_JPEG_QUALITY, 90, 0 };
    cvSaveImage(path, frame, imgParams);
}

void
get_timestamps(struct TestContext *context, struct TrackingTimestamps *ts)
{
    _psmove_tracker_retrieve_stats(context->tracker,
            &(ts->begin),
            &(ts->grab),
            &(ts->retrieve),
            &(ts->converted));
    ts->tracked = _psmove_timestamp();
}

void
get_position(struct TestContext *context, struct Position *pos)
{
    psmove_tracker_get_position(context->tracker, context->move,
            &(pos->x),
            &(pos->y),
            &(pos->radius));
}

void
switch_leds_on(struct TestContext *context)
{
    unsigned char r, g, b;
    psmove_tracker_get_color(context->tracker, context->move, &r, &g, &b);
    psmove_set_leds(context->move, r, g, b);
    psmove_update_leds(context->move);
}

void
switch_leds_off(struct TestContext *context)
{
    psmove_set_leds(context->move, 0, 0, 0);
    psmove_update_leds(context->move);
}

void
print_position(const char *msg, struct Position pos, int frame)
{
    printf("%-25s (%3.2f, %3.2f, %3.2f) @ frame #%d\n",
            msg, pos.x, pos.y, pos.radius, frame);
}

float
print_timestamp(struct TestContext *context,
        const char *msg, PSMove_timestamp ts, int frame)
{
	double diff = _psmove_timestamp_value(_psmove_timestamp_diff(
                ts, context->tracking_1_found));

    printf("%-25s %.10f s @ frame #%d\n", msg, diff, frame);

    return (float)diff;
}

void
print_timestamps(struct TestContext *context,
        const char *msg, struct TrackingTimestamps ts, int frame,
        float *grab, float *tracked)
{
	double grab_diff = _psmove_timestamp_value(_psmove_timestamp_diff(
                ts.grab, context->tracking_1_found));

	double tracked_diff = _psmove_timestamp_value(_psmove_timestamp_diff(
                ts.tracked, context->tracking_1_found));

    printf("%-25s %.10f s @ frame #%d (grab time)\n", msg, grab_diff, frame);
    printf("%-25s %.10f s @ frame #%d (tracked time)\n", msg, tracked_diff, frame);

    *grab = (float)grab_diff;
    *tracked = (float)tracked_diff;
}

void
summary(int i, struct TestContext *context, FILE *csv)
{
    float lostgrab, losttrack,
          recgrab, rectrack;

    printf("\n == Test Results == \n");

    print_timestamp(context, "Initial time",
            context->tracking_1_found,
            context->tracking_1_frame);

    print_position("Initial position",
            context->tracking_1_position,
            context->tracking_1_frame);

    float stable = print_timestamp(context, "Tracking stable at",
            context->tracking_1_stable,
            context->tracking_1_frame + STABLE_FRAMES_COUNT);

    print_position("Stable position",
            context->tracking_1_position_stable,
            context->tracking_1_frame + STABLE_FRAMES_COUNT);

    float ledsoff = print_timestamp(context, "LEDs off at",
            context->tracking_2_leds_off,
            context->tracking_2_frame);

    print_timestamps(context, "Tracking lost at",
            context->tracking_3_lost,
            context->tracking_3_frame,
            &lostgrab, &losttrack);

    float loststable = print_timestamp(context, "Lost stable at",
            context->tracking_3_stable,
            context->tracking_3_frame + STABLE_FRAMES_COUNT);

    float ledson = print_timestamp(context, "LEDs on at",
            context->tracking_4_leds_on,
            context->tracking_4_frame);

    print_timestamps(context, "Tracking recovered at",
            context->tracking_5_recovery,
            context->tracking_5_frame,
            &recgrab, &rectrack);

    print_position("Recovery position",
            context->tracking_5_position,
            context->tracking_5_frame);

    float recstable = print_timestamp(context, "Recovery stable at",
            context->tracking_5_stable,
            context->tracking_5_frame + STABLE_FRAMES_COUNT);

    print_position("Stable position",
            context->tracking_5_position_stable,
            context->tracking_5_frame + STABLE_FRAMES_COUNT);

    struct Position diff;
    diff.x = fabsf(context->tracking_5_position_stable.x -
            context->tracking_1_position_stable.x);
    diff.y = fabsf(context->tracking_5_position_stable.y -
            context->tracking_1_position_stable.y);
    diff.radius = fabsf(context->tracking_5_position_stable.radius -
        context->tracking_1_position_stable.radius);

    print_position("Position diff", diff, context->frame);

    fprintf(csv, "%d,%.10f,%.10f,%.10f,%.10f,%.10f,%.10f,"
            "%.10f,%.10f,%.10f,%.10f,%.10f,%.10f\n",
           i,
           stable,
           ledsoff,
           lostgrab, losttrack, loststable,
           ledson,
           recgrab, rectrack, recstable,
           diff.x, diff.y, diff.radius);
}

#define ITERATIONS 100

int
main(int argc, char *argv[])
{
    /**
     * Test Setup:
     *  - single controller via Bluetooth
     *  - 40 cm away from camera (centered)
     **/

    PSMoveTracker *tracker = psmove_tracker_new();

    FILE *csv = fopen("test_end2end_latency.csv", "w");
    assert(csv != NULL);

    fprintf(csv, "run,stable,ledsoff,lostgrab,losttrack,loststable,"
            "ledson,recgrab,rectrack,recstable,xdiff,ydiff,rdiff\n");

    int iteration;
    for (iteration=0; iteration<ITERATIONS; iteration++) {
        printf("Iteration %d...\n", iteration);

        struct TestContext context;
        memset(&context, 0, sizeof(struct TestContext));
        context.tracker = tracker;
        setup(&context);

        while (context.running) {
            psmove_tracker_update_image(context.tracker);
            psmove_tracker_update(context.tracker, context.move);
            context.frame++;

            context.tracker_status = psmove_tracker_get_status(context.tracker,
                    context.move);

            switch (context.state) {
                case TEST_INTIALIZED:
                    switch_leds_on(&context);
                    context.counter = 0;
                    context.state = WAIT_FOR_STABLE_TRACKING;
                    printf("Waiting for stable tracking...\n");
                    break;
                case WAIT_FOR_STABLE_TRACKING:
                    if (context.tracker_status == Tracker_TRACKING) {
                        if (context.counter < STABLE_FRAMES_COUNT) {
                            if (context.counter == 0) {
                                get_position(&context, &(context.tracking_1_position));
                                context.tracking_1_found = _psmove_timestamp();
                                context.tracking_1_frame = context.frame;
                            }
                            context.counter++;
                        } else {
                            //psmove_tracker_annotate(context.tracker);
                            //save(iteration, 1, &context);
                            get_position(&context, &(context.tracking_1_position_stable));
                            context.tracking_1_stable = _psmove_timestamp();

                            switch_leds_off(&context);
                            context.tracking_2_leds_off = _psmove_timestamp();
                            context.tracking_2_frame = context.frame;

                            context.counter = 0;
                            context.state = WAIT_FOR_TRACKING_LOSS;
                            printf("Waiting for tracking loss...\n");
                        }
                    } else {
                        context.counter = 0;
                    }
                    break;
                case WAIT_FOR_TRACKING_LOSS:
                    if (context.tracker_status == Tracker_CALIBRATED) {
                        if (context.counter < STABLE_FRAMES_COUNT) {
                            if (context.counter == 0) {
                                get_timestamps(&context, &(context.tracking_3_lost));
                                context.tracking_3_frame = context.frame;
                            }
                            context.counter++;
                        } else {
                            context.tracking_3_stable = _psmove_timestamp();

                            switch_leds_on(&context);
                            context.tracking_4_leds_on = _psmove_timestamp();
                            context.tracking_4_frame = context.frame;

                            context.counter = 0;
                            context.state = WAIT_FOR_TRACKING_RECOVERY;
                            printf("Waiting for tracking recovery...\n");
                        }
                    } else {
                        context.counter = 0;
                    }
                    break;
                case WAIT_FOR_TRACKING_RECOVERY:
                    if (context.tracker_status == Tracker_TRACKING) {
                        if (context.counter < STABLE_FRAMES_COUNT) {
                            if (context.counter == 0) {
                                get_position(&context, &(context.tracking_5_position));
                                get_timestamps(&context, &(context.tracking_5_recovery));
                                context.tracking_5_frame = context.frame;
                            }
                            context.counter++;
                        } else {
                            //psmove_tracker_annotate(context.tracker);
                            //save(iteration, 0, &context);
                            get_position(&context, &(context.tracking_5_position_stable));
                            context.tracking_5_stable = _psmove_timestamp();

                            context.state = TEST_FINISHED;
                            printf("Test finished.\n");
                        }
                    } else {
                        context.counter = 0;
                    }
                    break;
                case TEST_FINISHED:
                    switch_leds_off(&context);
                    context.running = 0;
                    break;
            }

            psmove_update_leds(context.move);
        }

        summary(iteration, &context, csv);
        teardown(&context);

        usleep(500000);
    }

    psmove_tracker_free(tracker);
    fclose(csv);

    return 0;
}

