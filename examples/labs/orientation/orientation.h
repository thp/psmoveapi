/**
 * PS Move API - An interface for the PS Move Motion Controller
 * (license header unchanged)
 */
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
