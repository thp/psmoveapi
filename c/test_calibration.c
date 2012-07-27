
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
#include "psmove_calibration.h"

int
main(int argc, char* argv[])
{
    PSMove *move;
    PSMoveCalibration *calibration;

    move = psmove_connect();

    if (move == NULL) {
        fprintf(stderr, "Could not connect to controller.\n");
        return EXIT_FAILURE;
    }

    calibration = psmove_calibration_new(move);
    psmove_calibration_load(calibration);
    psmove_calibration_dump(calibration);

    assert(psmove_calibration_supports_method(calibration,
                Calibration_USB));

    if (psmove_connection_type(move) == Conn_Bluetooth) {
        int in[9];
        float out[9];

        while (1) {
            int res = psmove_poll(move);
            if (res) {
                psmove_get_half_frame(move, Sensor_Accelerometer,
                        Frame_FirstHalf, &in[0], &in[1], &in[2]);
                psmove_get_half_frame(move, Sensor_Gyroscope,
                        Frame_FirstHalf, &in[3], &in[4], &in[5]);
                psmove_get_magnetometer(move,
                        &in[6], &in[7], &in[8]);

                psmove_calibration_map(calibration, in, out, 9);

                printf("A: %5.2f %5.2f %5.2f ", out[0], out[1], out[2]);
                printf("G: %6.2f %6.2f %6.2f ", out[3], out[4], out[5]);
                printf("M: %9.2f %9.2f %9.2f ", out[6], out[7], out[8]);
                printf("\n");
            }
        }
    }

    psmove_calibration_destroy(calibration);
    psmove_disconnect(move);

    return EXIT_SUCCESS;
}

