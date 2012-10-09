
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


#include <QThread>
#include <QDebug>

#include <QQuaternion>
#include <QVector3D>

#include <assert.h>

#include "psmove.h"

class Orientation : public QThread
{
    Q_OBJECT

    signals:
        void updateQuaternion(QQuaternion quaternion);
        void updateAccelerometer(QVector3D accelerometer);
        void updateButtons(int buttons, int trigger);

    public:
        Orientation() : QThread() {}

        void run()
        {
            PSMove *move;

            QVector3D accelerometer;
            QQuaternion quaternion;

            assert((move = psmove_connect()) != NULL);
            psmove_enable_orientation(move, PSMove_True);
            assert(psmove_has_orientation(move));

            while (true) {
                while (psmove_poll(move)) {
                    int buttons = psmove_get_buttons(move);
                    int trigger = psmove_get_trigger(move);
                    emit updateButtons(buttons, trigger);

                    if (buttons & Btn_MOVE) {
                        psmove_reset_orientation(move);
                    }

                    if (buttons & Btn_SQUARE) {
                        psmove_set_leds(move, 255, 0, 255);
                    } else if (buttons & Btn_TRIANGLE) {
                        psmove_set_leds(move, 0, 255, 0);
                    } else if (buttons & Btn_CROSS) {
                        psmove_set_leds(move, 0, 0, 255);
                    } else if (buttons & Btn_CIRCLE) {
                        psmove_set_leds(move, 255, 0, 0);
                    } else {
                        psmove_set_leds(move, 0, 0, 0);
                    }
                    psmove_update_leds(move);

                    float q0, q1, q2, q3;
                    psmove_get_orientation(move, &q0, &q1, &q2, &q3);
                    quaternion = QQuaternion(q0, q1, q2, q3);
                    emit updateQuaternion(quaternion);
                    //qDebug() << "updateQuaternion:" << quaternion;

                    float ax, ay, az;
                    psmove_get_accelerometer_frame(move, Frame_SecondHalf,
                            &ax, &ay, &az);
                    accelerometer = QVector3D(ax, ay, az);
                    emit updateAccelerometer(accelerometer);
                    //qDebug() << "updateAccelerometer:" << accelerometer;
                }
            }

            psmove_disconnect(move);
        }
};

