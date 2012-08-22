
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

int
main(int argc, char* argv[])
{
    PSMove *move;

    move = psmove_connect();

    if (move == NULL) {
        fprintf(stderr, "Could not connect to controller.\n");
        return EXIT_FAILURE;
    }

    assert(psmove_has_calibration(move));

    if (psmove_connection_type(move) == Conn_Bluetooth) {
        float ax, ay, az, gx, gy, gz;

        while (1) {
            int res = psmove_poll(move);
            if (res) {
                psmove_get_accelerometer_frame(move, Frame_SecondHalf,
                        &ax, &ay, &az);
                psmove_get_gyroscope_frame(move, Frame_SecondHalf,
                        &gx, &gy, &gz);

                printf("A: %5.2f %5.2f %5.2f ", ax, ay, az);
                printf("G: %6.2f %6.2f %6.2f ", gx, gy, gz);
                printf("\n");
            }
        }
    }

    psmove_disconnect(move);

    return EXIT_SUCCESS;
}

