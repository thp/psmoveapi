
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

/**
 * This application tries to mimic the output of "linmctool"
 * (http://www.pabr.org/linmctool/linmctool.en.html) but uses
 * the PS Move API instead of using L2CAP socket directly.
 *
 * You should be able to use "linmcfake" as a drop-in replacement for
 * "linmctool", especially in combination with "mccalibrate" and
 * "mctrack" - if not, it's a bug in linmcfake, and needs fixing ;)
 **/

#define MAX_CONTROLLERS 7

int main(int argc, char* argv[])
{
    PSMove *moves[MAX_CONTROLLERS];

    int controllers = psmove_count_connected();
    int i;
    int quit = 0;

    for (i=0; i<controllers; i++) {
        moves[i] = psmove_connect_by_id(i);
        if (moves[i] == NULL) {
            fprintf(stderr, "Could not connect to controller #%d.\n", i);
            return EXIT_FAILURE;
        }
    }

    while (!quit) {
        for (i=0; i<controllers; i++) {
            int x, y, z;
            int seq = psmove_poll(moves[i]);

            if (seq) {
                int which;

                if (psmove_get_buttons(moves[i]) & Btn_PS) {
                    quit = 1;
                    break;
                }

                seq -= 1;
                for (which=0; which<2; which++) {
                    printf("%d %s PSMOVE  seq=%-2d",
                            i, psmove_get_serial(moves[i]), seq*2+which);

                    psmove_get_half_frame(moves[i], Sensor_Accelerometer,
                            which, &x, &y, &z);
                    printf(" aX=%-6d aY=%-6d aZ=%-6d", x, y, z);

                    psmove_get_half_frame(moves[i], Sensor_Gyroscope,
                            which, &x, &y, &z);
                    printf(" gX=%-6d gY=%-6d gZ=%-6d", x, y, z);

                    psmove_get_magnetometer(moves[i], &x, &y, &z);
                    /* The y value of the magnetometer is inverted in linmctool */
                    printf(" mX=%-5d mY=%-5d mZ=%-5d", x, -y, z);

                    printf("\n");
                }
            }
        }
    }

    for (i=0; i<controllers; i++) {
        psmove_disconnect(moves[i]);
    }

    return 0;
}

