
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


#include <QApplication>
#include <QThread>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "psmove.h"
#include "psmove_tracker.h"

struct ControllerState
{
    bool pressed;
    int id;
    int x;
    int y;
};

class MovetyTouch : public QThread
{
    Q_OBJECT

    signals:
        void pressed(int id, int x, int y);
        void motion(int id, int x, int y);
        void released(int id, int x, int y);
        void hover(int x, int y);

    public:
        MovetyTouch() : QThread() {}

        void run()
        {
            int count = psmove_count_connected();
            int i;
            int active = 0;

            PSMoveTracker *tracker = psmove_tracker_new();
            PSMove **moves = (PSMove**)calloc(count, sizeof(PSMove*));
            ControllerState *states = (ControllerState*)calloc(count,
                    sizeof(ControllerState));

            for (i=0; i<count; i++) {
                moves[i] = psmove_connect_by_id(i);
                assert(moves[i] != NULL);

                while (psmove_tracker_enable(tracker, moves[i])
                        != Tracker_CALIBRATED) {
                    // Retry calibration until it works
                }
            }

            while (true) {
                for (i=0; i<count; i++) {
                    while (psmove_poll(moves[i]));

                    int x, y;
                    psmove_tracker_get_position(tracker, moves[i],
                            &x, &y, NULL);

                    bool pressed_now = ((psmove_get_buttons(moves[i]) & Btn_T) != 0);
                    bool moved_now = (x != states[i].x || y != states[i].y);

                    if (pressed_now != states[i].pressed) {
                        if (pressed_now) {
                            states[i].id = active;
                            active++;

                            emit pressed(states[i].id, x, y);
                        } else {
                            emit released(states[i].id, x, y);

                            active--;
                        }
                    } else if (moved_now) {
                        if (pressed_now) {
                            emit motion(states[i].id, x, y);
                        } else {
                            emit hover(x, y);
                        }
                    }

                    states[i].pressed = pressed_now;
                    states[i].x = x;
                    states[i].y = y;
                }

                psmove_tracker_update_image(tracker);
                psmove_tracker_update(tracker, NULL);

                for (i=0; i<count; i++) {
                    unsigned char r, g, b;
                    psmove_tracker_get_color(tracker, moves[i], &r, &g, &b);
                    psmove_set_leds(moves[i], r, g, b);
                    psmove_update_leds(moves[i]);
                }
            }

            psmove_tracker_free(tracker);

            for (i=0; i<count; i++) {
                psmove_disconnect(moves[i]);
            }

            free(moves);
        }
};

