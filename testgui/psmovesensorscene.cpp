
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (C) 2011 Thomas Perl <m@thp.io>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
