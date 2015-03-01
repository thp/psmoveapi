
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
#include <math.h>

#include "psmove.h"
#include "psmove_private.h"
#include "psmove_orientation.h"

#if defined(PSMOVE_WITH_MADGWICK_AHRS)
#  include "../external/MadgwickAHRS/MadgwickAHRS.h"
#endif


struct _PSMoveOrientation {
    PSMove *move;

    /* 9 values = 3x accelerometer + 3x gyroscope + 3x magnetometer */
    float output[9];

    /* Current sampling frequency */
    float sample_freq;

    /* Sample frequency measurements */
    long sample_freq_measure_start;
    long sample_freq_measure_count;

    /* Output value as quaternion */
    float quaternion[4];

    /* Quaternion measured when controller points towards camera */
    float reset_quaternion[4];
};


PSMoveOrientation *
psmove_orientation_new(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, NULL);

    if (!psmove_has_calibration(move)) {
        psmove_DEBUG("Can't create orientation - no calibration!\n");
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
    orientation->quaternion[0] = 1.f;
    orientation->quaternion[1] = 0.f;
    orientation->quaternion[2] = 0.f;
    orientation->quaternion[3] = 0.f;

    return orientation;
}


void
psmove_orientation_update(PSMoveOrientation *orientation)
{
    psmove_return_if_fail(orientation != NULL);

    int frame;

    long now = psmove_util_get_ticks();
    if (now - orientation->sample_freq_measure_start >= 1000) {
        float measured = ((float)orientation->sample_freq_measure_count) /
            ((float)(now-orientation->sample_freq_measure_start))*1000.;
        psmove_DEBUG("Measured sample_freq: %f\n", measured);

        orientation->sample_freq = measured;
        orientation->sample_freq_measure_start = now;
        orientation->sample_freq_measure_count = 0;
    }

    /* We get 2 measurements per call to psmove_poll() */
    orientation->sample_freq_measure_count += 2;

    psmove_get_magnetometer_vector(orientation->move,
            &orientation->output[6],
            &orientation->output[7],
            &orientation->output[8]);

    float q0 = orientation->quaternion[0];
    float q1 = orientation->quaternion[1];
    float q2 = orientation->quaternion[2];
    float q3 = orientation->quaternion[3];

    for (frame=0; frame<2; frame++) {
        psmove_get_accelerometer_frame(orientation->move, frame,
                &orientation->output[0],
                &orientation->output[1],
                &orientation->output[2]);

        psmove_get_gyroscope_frame(orientation->move, frame,
                &orientation->output[3],
                &orientation->output[4],
                &orientation->output[5]);

#if defined(PSMOVE_WITH_MADGWICK_AHRS)
        MadgwickAHRSupdate(orientation->quaternion,
                orientation->sample_freq,

                /* Accelerometer */
                orientation->output[0],
                orientation->output[1],
                orientation->output[2],

                /* Gyroscope */
                orientation->output[3],
                orientation->output[4],
                orientation->output[5],

                /* Magnetometer */
                orientation->output[6],
                orientation->output[7],
                orientation->output[8]
        );
#else
        psmove_DEBUG("Built without Madgwick AHRS - no orientation tracking");
        return;
#endif

        if (isnan(orientation->quaternion[0]) ||
            isnan(orientation->quaternion[1]) ||
            isnan(orientation->quaternion[2]) ||
            isnan(orientation->quaternion[3])) {
            psmove_DEBUG("Orientation is NaN!");
            orientation->quaternion[0] = q0;
            orientation->quaternion[1] = q1;
            orientation->quaternion[2] = q2;
            orientation->quaternion[3] = q3;
        }
    }
}

void
psmove_orientation_get_quaternion(PSMoveOrientation *orientation,
        float *q0, float *q1, float *q2, float *q3)
{
    psmove_return_if_fail(orientation != NULL);

    /* first factor (reset quaternion) */
    float a_s = orientation->reset_quaternion[0];
    float a_x = orientation->reset_quaternion[1];
    float a_y = orientation->reset_quaternion[2];
    float a_z = orientation->reset_quaternion[3];

    /* second factor (quaternion) */
    float b_s = orientation->quaternion[0];
    float b_x = orientation->quaternion[1];
    float b_y = orientation->quaternion[2];
    float b_z = orientation->quaternion[3];

    /**
     * Quaternion multiplication:
     * http://lxr.kde.org/source/qt/src/gui/math3d/qquaternion.h#198
     **/
    float ww = (a_z + a_x) * (b_x + b_y);
    float yy = (a_s - a_y) * (b_s + b_z);
    float zz = (a_s + a_y) * (b_s - b_z);
    float xx = ww + yy + zz;
    float qq = .5f * (xx + (a_z - a_x) * (b_x - b_y));

    /* Result */
    float r_s = qq - ww + (a_z - a_y) * (b_y - b_z);
    float r_x = qq - xx + (a_x + a_s) * (b_x + b_s);
    float r_y = qq - yy + (a_s - a_x) * (b_y + b_z);
    float r_z = qq - zz + (a_z + a_y) * (b_s - b_x);

    if (q0) {
        *q0 = r_s;
    }

    if (q1) {
        *q1 = r_x;
    }

    if (q2) {
        *q2 = r_y;
    }

    if (q3) {
        *q3 = r_z;
    }
}

void
psmove_orientation_reset_quaternion(PSMoveOrientation *orientation)
{
    psmove_return_if_fail(orientation != NULL);
    float q0 = orientation->quaternion[0];
    float q1 = orientation->quaternion[1];
    float q2 = orientation->quaternion[2];
    float q3 = orientation->quaternion[3];

    /**
     * Normalize and conjugate in one step:
     *  - Normalize via the length
     *  - Conjugate using (scalar, x, y, z) -> (scalar, -x, -y, -z)
     **/
    double length = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    orientation->reset_quaternion[0] = q0 / length;
    orientation->reset_quaternion[1] = -q1 / length;
    orientation->reset_quaternion[2] = -q2 / length;
    orientation->reset_quaternion[3] = -q3 / length;
}

void
psmove_orientation_free(PSMoveOrientation *orientation)
{
    psmove_return_if_fail(orientation != NULL);

    free(orientation);
}

