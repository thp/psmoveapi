
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
#include <assert.h>
#include <unistd.h>

#include "psmove.h"

#ifdef __APPLE__
#  include "xdo_osx.h"
#else
#  include "xdo.h"
#endif

/* Scaling factor to convert gyro values to relative pixel coordinates */
#define SCALE(v) (14*v)

/* Macro to generate code to map a PS Move button to a mouse button */
#define MAP_PSMOVE_INPUT(psm, mou, key) \
    { \
            if (pressed & (psm)) { \
                if (key) xdo_keysequence_down(xdo, CURRENTWINDOW, (key), 0); \
                if (mou) xdo_mousedown(xdo, CURRENTWINDOW, (mou)); \
            } else if (released & (psm)) { \
                if (mou) xdo_mouseup(xdo, CURRENTWINDOW, (mou)); \
                if (key) xdo_keysequence_up(xdo, CURRENTWINDOW, (key), 0); \
            } \
    }

#define BAILOUT(msg) \
    { \
        fprintf(stderr, "%s\n", msg); \
        return 1; \
    }

int
main(int argc, char *argv[])
{
    PSMove *move = psmove_connect();
    if (!move) {
        BAILOUT("Cannot connect to move controller.");
    }

    if (!psmove_has_calibration(move)) {
        BAILOUT("Calibration data not found. Pair using USB first.");
    }

    xdo_t *xdo = xdo_new(NULL);
    if (!xdo) {
        BAILOUT("Cannot create new xdo instance.");
    }

    int old_buttons = 0;
    int position_locked = 0;

    while (1) {
        while (psmove_poll(move)) {
            float gx, gy, gz;

            int buttons = psmove_get_buttons(move);
            unsigned int pressed, released;
            psmove_get_button_events(move, &pressed, &released);

            /**
             * Default button mapping:
             *  - Cross x ....... left mouse button
             *  - Move .......... middle mouse button
             *  - Circle o ...... right mouse button
             **/
            MAP_PSMOVE_INPUT(Btn_CROSS, 1, NULL);
            MAP_PSMOVE_INPUT(Btn_MOVE, 2, NULL);
            MAP_PSMOVE_INPUT(Btn_CIRCLE, 3, NULL);

            /**
             * Keyboard mappings:
             *  - Square [] ..... left cursor key
             *  - Trigger ....... alt (for combinations)
             *  - Select ........ esc
             **/
            MAP_PSMOVE_INPUT(Btn_SQUARE, 0, "Left");
            MAP_PSMOVE_INPUT(Btn_T, 0, "Alt_L");
            MAP_PSMOVE_INPUT(Btn_SELECT, 0, "Escape");

            /**
             * For use with the Compiz "Annotate" plugin:
             *  - Triangle /\ ... draw annotation
             *  - Start ......... clear annotation
             **/
            MAP_PSMOVE_INPUT(Btn_TRIANGLE, 1, "Alt+Super_L");
            MAP_PSMOVE_INPUT(Btn_START, 0, "Alt+Super_L+k");

            /**
             * Special functions (useful for presentations):
             *  - PS button ..... toggle mouse position locking
             **/
            if (pressed & Btn_PS) {
                position_locked = !position_locked;
            }

            old_buttons = buttons;

            psmove_get_gyroscope_frame(move, Frame_SecondHalf, &gx, &gy, &gz);

            int dx = SCALE(-gz);
            int dy = SCALE(-gx);

            if ((dx || dy) && !position_locked) {
                xdo_mousemove_relative(xdo, dx, dy);
            }
        }

        /* Controller sends 60 reports / second - wait for the next one */
        usleep(1000000/60);
    }

    xdo_free(xdo);

    psmove_disconnect(move);

    return 0;
}

