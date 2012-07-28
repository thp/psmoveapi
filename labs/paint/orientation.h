
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
#include "psmove_calibration.h"
#include "psmove_tracker.h"

#include "MadgwickAHRS/MadgwickAHRS.h"

extern "C" {
    void cvShowImage(const char *, void*);
};

class Orientation : public QThread
{
    Q_OBJECT

    signals:
        void orientation(qreal a, qreal b, qreal c, qreal d,
                qreal scale, qreal x, qreal y, qreal trigger);
        void newcolor(int r, int g, int b);

    public:
        Orientation() : QThread() {}

        void run()
        {
            PSMove *move = psmove_connect();
            int quit = 0;

            if (move == NULL) {
                fprintf(stderr, "Could not connect to controller.\n");
                QApplication::quit();
            }

            PSMoveTracker *tracker = psmove_tracker_new();
            int result = psmove_tracker_enable(tracker, move);
            assert(result == Tracker_CALIBRATED);

            PSMoveCalibration *calibration = psmove_calibration_new(move);
            psmove_calibration_load(calibration);
            assert(psmove_calibration_supports_method(calibration,
                        Calibration_USB));

            while (!quit) {
                int frame;
                int input[9];
                float output[9]; // ax, ay, az, gx, gy, gz, mx, my, mz
                while (int seq = psmove_poll(move)) {
                    if (psmove_get_buttons(move) & Btn_PS) {
                        quit = 1;
                        break;
                    }

                    if (psmove_get_buttons(move) & Btn_MOVE) {
                        q0 = 1.;
                        q1 = q2 = q3 = 0.;
                        emit newcolor(0, 0, 0);
                    }

                    if (psmove_get_buttons(move) & Btn_CROSS) {
                        emit newcolor(0, 0, 255);
                    }

                    if (psmove_get_buttons(move) & Btn_SQUARE) {
                        emit newcolor(255, 0, 128);
                    }

                    if (psmove_get_buttons(move) & Btn_TRIANGLE) {
                        emit newcolor(0, 255, 0);
                    }

                    if (psmove_get_buttons(move) & Btn_CIRCLE) {
                        emit newcolor(255, 0, 0);
                    }

                    psmove_get_magnetometer(move, &input[6], &input[7], &input[8]);

                    seq -= 1;

                    for (frame=0; frame<2; frame++) {
                        psmove_get_half_frame(move, Sensor_Accelerometer,
                                (enum PSMove_Frame)frame,
                                &input[0], &input[1], &input[2]);

                        psmove_get_half_frame(move, Sensor_Gyroscope,
                                (enum PSMove_Frame)frame,
                                &input[3], &input[4], &input[5]);

                        psmove_calibration_map(calibration, input, output, 9);

                        MadgwickAHRSupdate(output[3], output[4], output[5],
                                output[0], output[1], output[2],
                                output[6], output[7], output[8]);
                    }

                    int x, y, radius;
                    psmove_tracker_get_position(tracker, move, &x, &y, &radius);
                    emit orientation(q0, q1, q2, q3,
                            radius,
                            x,
                            y,
                            (qreal)psmove_get_trigger(move) / 255.);
                }

                psmove_tracker_update_image(tracker);
                psmove_tracker_update(tracker, NULL);

                //cvShowImage("asdf", psmove_tracker_get_image(tracker));

                unsigned char r, g, b;
                psmove_tracker_get_color(tracker, move, &r, &g, &b);
                psmove_set_leds(move, r, g, b);
                psmove_update_leds(move);
            }

            psmove_tracker_free(tracker);
            psmove_calibration_destroy(calibration);
            psmove_disconnect(move);
            QApplication::quit();
        }
};

