
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

#include "psmove.h"
#include "psmove_private.h"
#include "psmove_orientation.h"

#include "../external/MadgwickAHRS/MadgwickAHRS.h"


struct _PSMoveOrientation {
    PSMove *move;

    /* 9 values = 3x accelerometer + 3x gyroscope + 3x magnetometer */
    int input[9];
    float output[9];

    /* Current sampling frequency */
    float sample_freq;

    /* Sample frequency measurements */
    long sample_freq_measure_start;
    long sample_freq_measure_count;

    /* Output value as quaternion */
    float quaternion[4];
};


PSMoveOrientation *
psmove_orientation_new(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, NULL);

    if (!psmove_has_calibration(move)) {
#ifdef PSMOVE_DEBUG
        fprintf(stderr, "[PSMOVE] Can't create orientation - no calibration!\n");
#endif
        return NULL;
    }

    PSMoveOrientation *orientation = calloc(1, sizeof(PSMoveOrientation));

    orientation->move = move;

    /* Initial sampling frequency */
    orientation->sample_freq = 120.0f;

    /* Measurement */
    orientation->sample_freq_measure_start = psmove_util_get_ticks();
    orientation->sample_freq_measure_count = 0;

    /* Initial quaternion */
    orientation->quaternion[0] = 1;
    orientation->quaternion[1] = 0;
    orientation->quaternion[2] = 0;
    orientation->quaternion[3] = 0;

    return orientation;
}


int
psmove_orientation_poll(PSMoveOrientation *orientation)
{
    psmove_return_val_if_fail(orientation != NULL, 0);

    int frame;
    int seq;

    long now = psmove_util_get_ticks();
    if (now - orientation->sample_freq_measure_start >= 1000) {
        float measured = ((float)orientation->sample_freq_measure_count) /
            ((float)(now-orientation->sample_freq_measure_start))*1000.;

#ifdef PSMOVE_DEBUG
        printf("[PSMOVE] Measured sample_freq: %f\n", measured);
#endif

        orientation->sample_freq = measured;
        orientation->sample_freq_measure_start = now;
        orientation->sample_freq_measure_count = 0;
    }

    seq = psmove_poll(orientation->move);

    if (seq) {
        /* We get 2 measurements per call to psmove_poll() */
        orientation->sample_freq_measure_count += 2;

        psmove_get_magnetometer(orientation->move,
                &orientation->input[6],
                &orientation->input[7],
                &orientation->input[8]);

        for (frame=0; frame<2; frame++) {
            psmove_get_accelerometer_frame(orientation->move, frame,
                    &orientation->output[0],
                    &orientation->output[1],
                    &orientation->output[2]);

            psmove_get_gyroscope_frame(orientation->move, frame,
                    &orientation->output[3],
                    &orientation->output[4],
                    &orientation->output[5]);

            MadgwickAHRSupdate(orientation->quaternion,
                    orientation->sample_freq,

                    -orientation->output[0],
                    orientation->output[1],
                    orientation->output[2],

                    orientation->output[3],
                    orientation->output[5],
                    -orientation->output[4],

                    /* Magnetometer orientation disabled for now */
                    0, 0, 0
#if 0
                    orientation->output[6],
                    orientation->output[8],
                    orientation->output[7]
#endif
            );
        }
    }

    return seq;
}

void
psmove_orientation_get_quaternion(PSMoveOrientation *orientation,
        float *q0, float *q1, float *q2, float *q3)
{
    psmove_return_if_fail(orientation != NULL);

    if (q0) {
        *q0 = orientation->quaternion[0];
    }

    if (q1) {
        *q1 = orientation->quaternion[1];
    }

    if (q2) {
        *q2 = orientation->quaternion[2];
    }

    if (q3) {
        *q3 = orientation->quaternion[3];
    }
}

void
psmove_orientation_set_quaternion(PSMoveOrientation *orientation,
        float q0, float q1, float q2, float q3)
{
    psmove_return_if_fail(orientation != NULL);

    orientation->quaternion[0] = q0;
    orientation->quaternion[1] = q1;
    orientation->quaternion[2] = q2;
    orientation->quaternion[3] = q3;
}

void
psmove_orientation_free(PSMoveOrientation *orientation)
{
    psmove_return_if_fail(orientation != NULL);

    free(orientation);
}

