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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "psmove.h"

// Private header for psmove_port_sleep_ms()
#include "../../src/psmove_port.h"

typedef struct
{
	int r, g, b;
} Color;

Color convert_battery_to_col(enum PSMove_Battery_Level level)
{
    Color out = { .r = 0, .g = 0, .b = 0 };
    switch(level)
    {
        case Batt_MIN: out.r = 255; out.g =   0; out.b =   0; break;
        case Batt_20Percent: out.r = 255; out.g = 128; out.b =   0; break;
        case Batt_40Percent: out.r = 255; out.g = 255; out.b =   0; break;
        case Batt_60Percent: out.r = 153; out.g = 255; out.b =   0; break;
        case Batt_80Percent: out.r =  77; out.g = 255; out.b =   0; break;
        case Batt_MAX: out.r =   0; out.g = 255; out.b =   0; break;
        case Batt_CHARGING: out.r = 128; out.g =   0; out.b = 255; break;
        case Batt_CHARGING_DONE: out.r = 255; out.g =   0; out.b = 255; break;
    }
    return out;
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
        psmove_set_rate_limiting(moves[i], 1);
    }

    bool running = true;
    while(running) {
        for(i=0; i<c; i++) {
            while(psmove_poll(moves[i])) {}
            enum PSMove_Battery_Level level = psmove_get_battery(moves[i]);
            Color col = convert_battery_to_col(level);
			psmove_set_leds(moves[i], (unsigned char)col.r, (unsigned char)col.g, (unsigned char)col.b);
            psmove_update_leds(moves[i]);
            int buttons = psmove_get_buttons(moves[i]);
            if(buttons & Btn_PS)
                running = false;
        }
        psmove_port_sleep_ms(1000);
    }

    for(i=0; i<c; i++) {
    	psmove_disconnect(moves[i]);
    }

    free(moves);
    return 0;
}
