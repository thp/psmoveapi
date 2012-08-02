
#include "demoview.h"

#include <QDebug>

DemoView::DemoView()
    : QGraphicsView()
{
    resize(640, 480);

    QGraphicsScene *scene = new QGraphicsScene;
    setScene(scene);

    scene->setBackgroundBrush(Qt::black);
    scene->addPixmap(QPixmap("test.png"))->setScale(.4);

    hovers[0] = scene->addEllipse(0, 0, 0, 0, QPen(Qt::transparent), QColor(255, 255, 255, 128));
    hovers[1] = scene->addEllipse(0, 0, 0, 0, QPen(Qt::transparent), QColor(255, 255, 255, 128));
}

void
DemoView::startDrag(int x, int y)
{
    drag.target = pinpoint(x, y);
    drag.x = x;
    drag.y = y;
}

void
DemoView::updateDrag(int x, int y)
{
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
    drag.target = NULL;
}

void
DemoView::startZoom(int x, int y, float distance, float degrees)
{
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
    }
}

void
DemoView::updateZoom(int x, int y, float distance, float degrees)
{
    if (zoom.target) {
        float ddegrees = degrees - zoom.degrees;
        float factor = distance / zoom.distance;
        qDebug() << "updateZoom" << factor << degrees;
        zoom.target->setScale(zoom.target->scale()*factor);
        zoom.target->setRotation(zoom.target->rotation()+ddegrees);
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
    zoom.target = NULL;
}

void
DemoView::hover(int idx, int x, int y)
{
    int size = 19;
    hovers[idx]->setRect(QRectF(x-size/2, y-size/2, size, size));
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

