
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2020 Thomas Perl <m@thp.io>
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
#include <limits.h>

#include <algorithm>

#include "SDL.h"
#include "psmove.h"

static int
find_nav_controller(int count)
{
    for (int i=0; i<count; ++i) {
        const char *name = SDL_JoystickNameForIndex(i);

        // Linux via USB
        if (strcmp(name, "Sony Navigation Controller") == 0) {
            printf("Found USB Navigation Controller at index %d\n", i);
            return i;
        }

        // Linux via Bluetooth
        if (strcmp(name, "Navigation Controller") == 0) {
            printf("Found Bluetooth Navigation Controller at index %d\n", i);
            return i;
        }
    }

    return -1;
}

static const struct {
    const char *name;
    int index;
} BUTTONS[] = {
    { "CROSS", NavBtn_CROSS },
    { "CIRCLE", NavBtn_CIRCLE },

    { "L1", NavBtn_L1 },
    { "L2", NavBtn_L2 },
    { "L3", NavBtn_L3 },

    { "PS", NavBtn_PS },

    { "UP", NavBtn_UP },
    { "DOWN", NavBtn_DOWN },
    { "LEFT", NavBtn_LEFT },
    { "RIGHT", NavBtn_RIGHT },
};

int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_JOYSTICK);

    int count = SDL_NumJoysticks();
    if (count == 0) {
        printf("No joysticks connected\n");
        exit(1);
    }

    printf("Found %d joystick(s)\n", count);

    int index = find_nav_controller(count);

    if (index == -1) {
        printf("Could not find a Navigation Controller\n");
        exit(1);
    }

    printf("Found Joystick %d: %s\n\n", index, SDL_JoystickNameForIndex(index));

    SDL_Joystick *joy = SDL_JoystickOpen(index);

    int ps_count = 0;

    if (joy) {
        SDL_Event e;
        while (ps_count < 50) {
            SDL_JoystickUpdate();

            if (SDL_JoystickGetButton(joy, NavBtn_PS)) {
                ++ps_count;
            } else {
                ps_count = 0;
            }

            Sint16 x = SDL_JoystickGetAxis(joy, 0);
            Sint16 y = SDL_JoystickGetAxis(joy, 1);
            Sint16 z = SDL_JoystickGetAxis(joy, 2);

            int xpos = 5 + 5 * (int)x / SHRT_MAX;
            int ypos = 3 + 3 * (int)y / SHRT_MAX;
            int zpos = 10 + 10 * (int)z / SHRT_MAX;

            printf("  +-----------+  Trigger: %+.2f [", (float)z/SHRT_MAX);
            for (int z=0; z<20; ++z) {
                if (zpos > z) {
                    printf("#");
                } else {
                    printf("_");
                }
            }
            printf("]\n");
            size_t i=0;
            for (int y=0; y<7; ++y) {
                printf("  |");
                for (int x=0; x<11; ++x) {
                    if (ypos == y && xpos == x) {
                        printf("X");
                    } else {
                        printf(" ");
                    }
                }
                printf("|      ");

                for (int k=0; y>0 && k<2 && i < sizeof(BUTTONS)/sizeof(BUTTONS[0]); ++k) {
                    printf("[%c] %-10s ",
                            SDL_JoystickGetButton(joy, BUTTONS[i].index) ? 'x' : '_',
                            BUTTONS[i].name);
                    ++i;
                }

                printf("\n");
            }
            printf("  +-----------+  Analog Stick: (x=%+.1f, y=%+.1f)\n",
                    (float)x/SHRT_MAX, (float)y/SHRT_MAX);

            printf("\npress the [PS] button for 500 ms to exit...");

            fflush(stdout);
            SDL_Delay(10);

            printf("\n\033[11A");
        }

        printf("\033[11BPS button longpress -- exiting\n");
    } else {
        printf("Couldn't open Joystick 0\n");
    }

    // Close if opened
    if (SDL_JoystickGetAttached(joy)) {
        SDL_JoystickClose(joy);
    }

    SDL_Quit();

    return 0;
}
