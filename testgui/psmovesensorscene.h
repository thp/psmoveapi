#ifndef PSMOVESENSORSCENE_H
#define PSMOVESENSORSCENE_H

#include <QtGui>

#include "psmoveqt.hpp"

class PSMoveSensorScene : public QGraphicsScene
{
    Q_OBJECT

    PSMoveQt *psm;
    QGraphicsLineItem *gyro;
    QGraphicsLineItem *accel;

public:
    explicit PSMoveSensorScene(QObject *parent = 0, PSMoveQt *psm = 0);

signals:

public slots:
    void onGyroChanged();
    void onAccelChanged();

};

#endif // PSMOVESENSORSCENE_H
