 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2014 Jonathan Whiting <jonathan@ignika.com>
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

 /**
 * A testing tool to help verify the responsiveness of connected move
 * controllers. Connects to all available controllers and then:
 *
 * 1) Live updates led color based on accelerometer. Provides general feedback.
 * 2) Sets led white when move button held. Specific 'how quick' feedback.
 * 3) Rumbles for 1 second in every 5. For controller to controller comparison.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "psmove.h"

int convert_accel_to_col(int accel)
{
    accel = (accel*256)/35000;
    if(accel < 0) accel = 0;
    if(accel > 255) accel = 255;
    return accel;
}

int main(int argc, char* argv[])
{
    if (!psmove_init(PSMOVE_CURRENT_VERSION)) {
        fprintf(stderr, "PS Move API init failed (wrong version?)\n");
        exit(1);
    }

    int i, c;
    c = psmove_count_connected();
    printf("Connected controllers: %d\n", c);
    if(c==0)
        return 1;

    PSMove **moves = (PSMove **)malloc(sizeof(PSMove *)*c);

    for(i=0; i<c; i++) {
        moves[i] = psmove_connect_by_id(i);
        psmove_set_rate_limiting(moves[i], PSMove_True);
    }

    bool running = true;
    while(running) {
        for(i=0; i<c; i++) {
            while(psmove_poll(moves[i])) {}
            int buttons = psmove_get_buttons(moves[i]);
            int x, y, z, r, g, b;
            psmove_get_accelerometer(moves[i], &x, &y, &z);
            r = convert_accel_to_col(x);
            g = convert_accel_to_col(y);
            b = convert_accel_to_col(z);
            if(buttons & Btn_PS)
                running = false;
            else if(buttons & Btn_MOVE)
                psmove_set_leds(moves[i], 255, 255, 255);
            else
				psmove_set_leds(moves[i], (unsigned char)r, (unsigned char)g, (unsigned char)b);
            clock_t t = clock();
            if(((t/CLOCKS_PER_SEC) % 5) == 0)
                psmove_set_rumble(moves[i], 255);
            else
                psmove_set_rumble(moves[i], 0);
            psmove_update_leds(moves[i]);
        }
    }

    for(i=0; i<c; i++) {
        psmove_disconnect(moves[i]);
    }
    free(moves);

    return 0;
}
