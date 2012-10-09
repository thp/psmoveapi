
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
#include <math.h>

#include "psmove.h"
#include "psmove_tracker.h"

class Orientation : public QThread
{
    Q_OBJECT

    signals:
        void newposition(int id, qreal scale, qreal x, qreal y, qreal trigger);
        void newcolor(int id, int r, int g, int b);
        void newimage(void *image);
        void backup_frame();
        void restore_frame();

    private:
        PSMoveTracker *tracker;

    public:
        Orientation() : QThread() {
            tracker = psmove_tracker_new();
        }

        void get_size(int &width, int &height) {
            psmove_tracker_update_image(tracker);
            psmove_tracker_get_size(tracker, &width, &height);
        }

        void run()
        {
            int i;
            int count = psmove_count_connected();

            PSMove **moves = (PSMove**)calloc(count, sizeof(PSMove*));

            for (i=0; i<count; i++) {
                moves[i] = psmove_connect_by_id(i);
                assert(moves[i] != NULL);
            }

            int quit = 0;

            for (i=0; i<count; i++) {
                while (psmove_tracker_enable(tracker, moves[i])
                        != Tracker_CALIBRATED) {
                    // Wait until calibration is done
                }
            }

            while (!quit) {
                int again;

                do {
                    again = 0;

                    for (i=0; i<count; i++) {
                        PSMove *move = moves[i];

                        int res = psmove_poll(move);

                        if (!res) {
                            continue;
                        }

                        again++;

                        if (psmove_get_buttons(move) & Btn_PS) {
                            quit = 1;
                            break;
                        }

                        if (psmove_get_buttons(move) & Btn_SELECT) {
                            emit backup_frame();
                        }

                        if (psmove_get_buttons(move) & Btn_START) {
                            emit restore_frame();
                        }

                        if (psmove_get_buttons(move) & Btn_MOVE) {
                            emit newcolor(i, 255, 255, 255);
                        }

                        if (psmove_get_buttons(move) & Btn_CROSS) {
                            emit newcolor(i, 0, 0, 255);
                        }

                        if (psmove_get_buttons(move) & Btn_SQUARE) {
                            emit newcolor(i, 255, 255, 0);
                        }

                        if (psmove_get_buttons(move) & Btn_TRIANGLE) {
                            emit newcolor(i, 0, 255, 0);
                        }

                        if (psmove_get_buttons(move) & Btn_CIRCLE) {
                            emit newcolor(i, 255, 0, 0);
                        }

                        float x, y, radius;
                        psmove_tracker_get_position(tracker, move, &x, &y, &radius);
                        emit newposition(i, radius, x, y,
                                (qreal)psmove_get_trigger(move) / 255.);
                    }
                } while (again);

                psmove_tracker_update_image(tracker);
                psmove_tracker_update(tracker, NULL);
                emit newimage(psmove_tracker_get_frame(tracker));
            }

            psmove_tracker_free(tracker);

            for (i=0; i<count; i++) {
                psmove_disconnect(moves[i]);
            }

            QApplication::quit();
        }
};

