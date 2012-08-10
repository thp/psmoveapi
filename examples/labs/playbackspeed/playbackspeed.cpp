
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
#include <QMediaPlayer>
#include <QDir>

#include <assert.h>
#include <math.h>

#include "psmove.h"
#include "psmove_calibration.h"

/* normal playback rate (1x) and direction of the turntable */
#define TURNTABLE_RPM -45

/* convert rad/s to RPM */
#define RAD_PER_S_TO_RPM(x) ((x)*60/(2*M_PI))

/* input filename to play back (in current working directory) */
#define INPUT_FILENAME "input.wav"

/* How many frames have to be read for the speed to be updated? */
#define READ_FRAME_UPDATE_RATE 60


class PSMoveRotationReaderThread : public QThread
{
    public:
        PSMoveRotationReaderThread(QMediaPlayer *player)
            : QThread(),
              player(player)
        {}

        void run()
        {
            PSMove *move;
            PSMoveCalibration *calibration;

            /* 6 values = ax, ay, az, gx, gy, gz (accel + gyro) */
            int inputs[6] = { 0, 0, 0, 0, 0, 0 };
            float outputs[6];

            int i = 0;
            qreal rate = 1.;

            assert((move = psmove_connect()) != NULL);
            assert(psmove_connection_type(move) == Conn_Bluetooth);

            assert((calibration = psmove_calibration_new(move)) != NULL);
            assert(psmove_calibration_supported(calibration));

            while (true) {
                if (psmove_poll(move)) {
                    psmove_get_half_frame(move,
                            Sensor_Gyroscope,
                            Frame_SecondHalf,
                            &inputs[3], &inputs[4], &inputs[5]);

                    psmove_calibration_map(calibration, inputs, outputs, 6);

                    /**
                     * Playback rate is calculated based on the RPM of the
                     * controller on the Z axis (outputs[5]) and the normal
                     * playback rate of the turntable (see above).
                     **/
                    rate = RAD_PER_S_TO_RPM(outputs[5]) / TURNTABLE_RPM;

                    /* Rate-limiting the updates of the playback rate */
                    if (i % READ_FRAME_UPDATE_RATE == 0) {
                        /**
                         * On Linux, the rate must not be negative. It might be
                         * possible that on Mac OS X or Windows, the rate can
                         * also be negative (depends on the backend).
                         **/
                        if (rate > 0 && rate != player->playbackRate()) {
                            qint64 pos = player->position();
                            player->setPlaybackRate(rate);
                            player->setPosition(pos);
                        }
                    }

                    i++;
                }
            }

            psmove_calibration_free(calibration);
            psmove_disconnect(move);
        }

    private:
        QMediaPlayer *player;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMediaPlayer player;

    PSMoveRotationReaderThread thread(&player);

    QString filename = QDir().absoluteFilePath(INPUT_FILENAME);
    QUrl url = QUrl::fromLocalFile(filename);
    player.setMedia(QMediaContent(url));

    /* Start with a really slow playback rate */
    player.setPlaybackRate(.01);
    player.play();

    thread.start();

    return app.exec();
}

