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
