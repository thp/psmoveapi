
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


#include "demoview.h"

#include <QGraphicsRotation>
#include <QQuaternion>
#include <QDebug>
#include <QMatrix4x4>
#include <QDir>

#include <math.h>

DemoView::DemoView()
    : QGraphicsView()
{
    resize(640, 480);

    QGraphicsScene *scene = new QGraphicsScene;
    setScene(scene);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);

    scene->setBackgroundBrush(Qt::black);

    scene->addPixmap(QPixmap("test.png"));

    QDir imagesDir("images/");

    QStringList images = imagesDir.entryList();
    int i;
    foreach (QString filename, images) {
        QGraphicsPixmapItem *pixmap = scene->addPixmap(
                QPixmap(imagesDir.filePath(filename)));
        pixmap->moveBy(width()/2+i*20, height()/2+i*30);
        i++;
    }

    hovers[0] = scene->addEllipse(0, 0, 0, 0,
            QPen(Qt::transparent), QColor(255, 0, 255, 200));
    hovers[1] = scene->addEllipse(0, 0, 0, 0,
            QPen(Qt::transparent), QColor(0, 255, 255, 200));
}

void
DemoView::resizeEvent(QResizeEvent *event)
{
    setSceneRect(0, 0, width(), height());
}

void
DemoView::startDrag(int x, int y)
{
    project(x, y);
    drag.target = pinpoint(x, y);
    drag.x = x;
    drag.y = y;

    if (drag.target) {
        drag.target->setOpacity(.9);
    }
}

void
DemoView::updateDrag(int x, int y)
{
    project(x, y);
    if (drag.target) {
        int dx = x - drag.x;
        int dy = y - drag.y;
        drag.target->moveBy(dx, dy);
        drag.x = x;
        drag.y = y;
    }
}

void
DemoView::stopDrag(int x, int y)
{
    updateDrag(x, y);
    if (drag.target) {
        drag.target->setOpacity(1);
    }
    drag.target = NULL;
}

void
DemoView::startZoom(int x, int y, float distance, float degrees)
{
    project(x, y);
    qDebug() << "startZoom";
    zoom.target = pinpoint(x, y);

    zoom.x = x;
    zoom.y = y;
    zoom.distance = distance;
    zoom.degrees = degrees;

    if (zoom.target) {
        QPointF transformOrigin(x, y);
        zoom.target->setTransformOriginPoint(
                zoom.target->mapFromScene(transformOrigin));
        zoom.target->setOpacity(.9);
    }
}

void
DemoView::updateZoom(int x, int y, float distance, float degrees)
{
    project(x, y);
    if (zoom.target) {
        QPointF c = zoom.target->mapFromScene(x, y);

        float factor = distance / zoom.distance;
        QTransform scale;
        scale.translate(c.x(), c.y());
        scale.scale(factor, factor);
        scale.translate(-c.x(), -c.y());
        zoom.target->setTransform(scale, true);

        float ddegrees = degrees - zoom.degrees;
        QTransform rotate;
        rotate.translate(c.x(), c.y());
        rotate.rotate(ddegrees);
        rotate.translate(-c.x(), -c.y());
        zoom.target->setTransform(rotate, true);

        int dx = x - zoom.x;
        int dy = y - zoom.y;
        zoom.target->moveBy(dx, dy);

        zoom.x = x;
        zoom.y = y;
        zoom.distance = distance;
        zoom.degrees = degrees;
    }
}

void
DemoView::stopZoom(int x, int y, float distance, float degrees)
{
    updateZoom(x, y, distance, degrees);
    if (zoom.target) {
        zoom.target->setOpacity(1);
    }
    zoom.target = NULL;
}

void
DemoView::hover(int idx, int x, int y)
{
    project(x, y);
    int size = 39;
    hovers[idx]->setRect(QRectF(x-size/2, y-size/2, size, size));
}


void
DemoView::startRotation(int id, int x, int y, float q0, float q1, float q2, float q3)
{
    rotate[id].target = pinpoint(x, y);
    rotate[id].x = x;
    rotate[id].y = y;
    rotate[id].q0 = q0;
    rotate[id].q1 = q1;
    rotate[id].q2 = q2;
    rotate[id].q3 = q3;
}

void
DemoView::updateRotation(int id, float q0, float q1, float q2, float q3)
{
#if 0
    if (rotate[id].target) {
        QQuaternion then(rotate[id].q0, rotate[id].q1, rotate[id].q2, rotate[id].q3);
        QQuaternion now(q0, q1, q2, q3);
        QQuaternion diff = now - then;

        QGraphicsPixmapItem *tpm = (QGraphicsPixmapItem*)rotate[id].target;

        QMatrix4x4 mat;
        //mat.perspective(90, width()/height(), -1, 100);
        //setTransform(mat.toTransform());

        int width = tpm->pixmap().width();
        int height = tpm->pixmap().height();

        //mat.translate(width/2, height/2);
        mat.rotate(diff);
        //mat.translate(-width/2, -height/2);

        rotate[id].target->setTransform(mat.toTransform(), true);
        rotate[id].q0 = q0;
        rotate[id].q1 = q1;
        rotate[id].q2 = q2;
        rotate[id].q3 = q3;
    }
#endif
}

void
DemoView::stopRotation(int id)
{
    rotate[id].target = NULL;
}

QGraphicsItem *
DemoView::pinpoint(int x, int y)
{
    QGraphicsItem *result;
    hovers[0]->hide();
    hovers[1]->hide();
    result = scene()->itemAt(x, y);
    hovers[0]->show();
    hovers[1]->show();
    return result;
}

void
DemoView::project(int &x, int &y)
{
    float xx;
    float yy;

    xx = (float)x / 640. * width();
    yy = (float)y / 480. * height();

    x = xx;
    y = yy;
}

