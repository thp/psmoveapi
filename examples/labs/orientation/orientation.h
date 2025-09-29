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

#ifndef ORIENTATION_H
#define ORIENTATION_H

#include <QApplication>
#include <QThread>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "psmove.h"
#include "psmove_tracker.h"
#include "psmove_tracker_opencv.h"

class Orientation : public QThread
{
    Q_OBJECT
signals:
    void orientation(qreal a, qreal b, qreal c, qreal d,
                     qreal scale, qreal x, qreal y);

public:
    Orientation() : QThread() {}

    void run() override
    {
        PSMove *move = psmove_connect();
        int quit = 0;
        float q0, q1, q2, q3;

        if (move == NULL) {
            fprintf(stderr, "Could not connect to controller.\n");
            QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
            return;
        }

        PSMoveTracker *tracker = psmove_tracker_new();

        while (psmove_tracker_enable(tracker, move) != Tracker_CALIBRATED) {
            // retry until calibrated
        }

        psmove_enable_orientation(move, true);

        while (!quit) {
            while (psmove_poll(move)) {
                if (psmove_get_buttons(move) & Btn_PS) {
                    quit = 1;
                    break;
                }
                if (psmove_get_buttons(move) & Btn_MOVE) {
                    psmove_reset_orientation(move);
                }

                psmove_get_orientation(move, &q0, &q1, &q2, &q3);

                float x, y, radius;
                psmove_tracker_get_position(tracker, move, &x, &y, &radius);

                emit orientation(q0, q1, q2, q3,
                                 1. - (qreal)radius / 150.,
                                 1. - ((qreal)x / 640.) * 2.,
                                 1. - ((qreal)y / 480.) * 2.);
            }

            psmove_tracker_update_image(tracker);
            psmove_tracker_update(tracker, NULL);

            // Legacy C API display removed. If you want it back:
            // cv::Mat frame = cv::cvarrToMat((IplImage*)psmove_tracker_opencv_get_frame(tracker));
            // cv::imshow("tracker", frame);
            // cv::waitKey(1);
        }

        psmove_tracker_free(tracker);
        psmove_disconnect(move);
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
    }
};

#endif // ORIENTATION_H
