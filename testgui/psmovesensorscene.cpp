
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2011 Thomas Perl <m@thp.io>
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


#include "psmovesensorscene.h"

PSMoveSensorScene::PSMoveSensorScene(QObject *parent, PSMoveQt *psm) :
    QGraphicsScene(parent),
    psm(psm),
    gyro(NULL),
    accel(NULL)
{
    setSceneRect(-100, -100, 200, 200);

    gyro = new QGraphicsLineItem(NULL, this);
    gyro->setLine(0, 0, 0, 0);
    gyro->setPen(QPen(Qt::red, 2));

    connect(psm, SIGNAL(gyroChanged()),
            this, SLOT(onGyroChanged()));

    accel = new QGraphicsLineItem(NULL, this);
    accel->setLine(0, 0, 0, 0);
    accel->setPen(QPen(Qt::blue, 2));

    connect(psm, SIGNAL(accelerometerChanged()),
            this, SLOT(onAccelChanged()));
}

void PSMoveSensorScene::onGyroChanged()
{
    gyro->setLine(0, 0, -psm->gz()/100, -psm->gx()/100);
}

void PSMoveSensorScene::onAccelChanged()
{
    accel->setLine(0, 0, -psm->ax()/100, -psm->az()/100);
}
