
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
    bool t_pressed;
    int t_id;

    bool move_pressed;
    int move_id;

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
        void hover(int idx, int x, int y);

        void startRotation(int id, int x, int y, float q0, float q1, float q2, float q3);
        void updateRotation(int id, float q0, float q1, float q2, float q3);
        void stopRotation(int id);

    public:
        MovetyTouch() : QThread() {}

        void run()
        {
            int count = psmove_count_connected();
            int i;
            int t_active = 0;
            int move_active = 0;

            PSMoveTracker *tracker = psmove_tracker_new();
            PSMove **moves = (PSMove**)calloc(count, sizeof(PSMove*));
            ControllerState *states = (ControllerState*)calloc(count,
                    sizeof(ControllerState));

            for (i=0; i<count; i++) {
                moves[i] = psmove_connect_by_id(i);
                assert(moves[i] != NULL);

                psmove_enable_orientation(moves[i], PSMove_True);
                assert(psmove_has_orientation(moves[i]));

                while (psmove_tracker_enable(tracker, moves[i])
                        != Tracker_CALIBRATED) {
                    // Retry calibration until it works
                }
            }

            while (true) {
                for (i=0; i<count; i++) {
                    while (psmove_poll(moves[i]));

                    if (psmove_get_buttons(moves[i]) & Btn_PS) {
                        QApplication::quit();
                    }

                    float x, y;
                    psmove_tracker_get_position(tracker, moves[i],
                            &x, &y, NULL);
                    x = 640. - x;

                    bool pressed_now = ((psmove_get_buttons(moves[i]) & Btn_T) != 0);
                    bool moved_now = (x != states[i].x || y != states[i].y);

                    if (pressed_now != states[i].t_pressed) {
                        if (pressed_now) {
                            states[i].t_id = t_active;
                            t_active++;

                            emit pressed(states[i].t_id, x, y);
                        } else {
                            emit released(states[i].t_id, x, y);

                            t_active--;
                        }
                    } else if (moved_now) {
                        if (pressed_now) {
                            emit motion(states[i].t_id, x, y);
                        }
                        emit hover(i, x, y);
                    }

                    float q0, q1, q2, q3;
                    psmove_get_orientation(moves[i], &q0, &q1, &q2, &q3);

                    bool move_now = ((psmove_get_buttons(moves[i]) & Btn_MOVE) != 0);

                    if (move_now != states[i].move_pressed) {
                        if (move_now) {
                            states[i].move_id = move_active;
                            move_active++;

                            q0 = 1.;
                            q1 = q2 = q3 = 0.;
                            psmove_reset_orientation(moves[i]);

                            emit startRotation(states[i].move_id, x, y,
                                    q0, q1, q2, q3);
                        } else {
                            emit stopRotation(states[i].move_id);

                            move_active--;
                        }
                    } else if (states[i].move_pressed) {
                        emit updateRotation(states[i].move_id, q0, q1, q2, q3);
                    }

                    states[i].t_pressed = pressed_now;
                    states[i].move_pressed = move_now;
                    states[i].x = x;
                    states[i].y = y;
                }

                psmove_tracker_update_image(tracker);
                psmove_tracker_update(tracker, NULL);
            }

            psmove_tracker_free(tracker);

            for (i=0; i<count; i++) {
                psmove_disconnect(moves[i]);
            }

            free(moves);
        }
};

