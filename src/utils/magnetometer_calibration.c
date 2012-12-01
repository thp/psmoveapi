
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

#include "psmove.h"

#define MINIMUM_REQUIRED_RANGE 320

int
main(int arg, char** args)
{
    PSMove *move;

    int i;
    int count = psmove_count_connected();
    for (i=0; i<count; i++) {
        PSMove *move = psmove_connect_by_id(i);
        int old_range = 0;
        if (psmove_connection_type(move) == Conn_Bluetooth) {
            int calibration_done = 0;
            psmove_reset_magnetometer_calibration(move);
            printf("Calibrating PS Move #%d\n", i);
            fflush(stdout);
            while (!calibration_done) {
                while (psmove_poll(move)) {
                    int pressed, released;
                    psmove_get_button_events(move, &pressed, &released);

                    /* This call calculates the new raw min/max values */
                    psmove_get_magnetometer_vector(move, NULL, NULL, NULL);

                    int range = psmove_get_magnetometer_calibration_range(move);
                    int percentage = 100 * range / MINIMUM_REQUIRED_RANGE;
                    if (percentage > 100) {
                        percentage = 100;
                    } else if (percentage < 0) {
                        percentage = 0;
                    }

                    psmove_set_leds(move, 2.5 * (100 - percentage), 2.5 * percentage, 0);
                    psmove_update_leds(move);

                    if (range > MINIMUM_REQUIRED_RANGE) {
                        if (old_range > 0) {
                            printf("\rCalibration done. Press the MOVE button to continue.\n");
                            old_range = 0;
                        }
                        if (pressed & Btn_MOVE) {
                            psmove_set_leds(move, 0, 0, 0);
                            psmove_update_leds(move);
                            psmove_save_magnetometer_calibration(move);
                            calibration_done = 1;
                            break;
                        }
                    } else if (range > old_range) {
                        printf("\rRotate the controller in all directions: %d%%", percentage);
                        fflush(stdout);
                        old_range = range;
                    }
                }

            }
        } else {
            printf("Ignoring non-Bluetooth PS Move #%d\n", i);
        }

        psmove_disconnect(move);
    }

    return 0;
}

