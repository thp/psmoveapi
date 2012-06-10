
#include "movegraph.h"

MoveGraph::MoveGraph()
    : QWidget(NULL),
      move(psmove_connect()),
      readings(),
      offset(0)
{
    resize(500, 400);
    setWindowTitle("PS Move API Labs - Sensor Filter");
}

void
MoveGraph::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    int Yzero = height() / 2;
    int sensor_max = 5000;

    painter.setPen(Qt::black);
    painter.drawLine(0, Yzero, width(), Yzero);

    int newValue;
    int oldValue;

    for (int a=0; a<3; a++) {
        switch (a) {
            case 0: painter.setPen(Qt::red); break;
            case 1: painter.setPen(Qt::green); break;
            case 2: painter.setPen(Qt::blue); break;
            default: break;
        }

        for (int x=0; x<width(); x++) {
            Reading *r = &readings[(offset-x) % (sizeof(readings)/sizeof(readings[0]))];
            int value = (a==0)?(r->x):((a==1)?(r->y):(r->z));
            newValue = Yzero + value*Yzero/sensor_max;

            if (x > 0) {
                painter.drawLine(x, oldValue, x+1, newValue);
            }

            oldValue = newValue;
        }
    }
}

void
MoveGraph::readSensors()
{
    if (psmove_poll(move)) {
        int x, y, z;
        psmove_get_accelerometer(move, &x, &y, &z);

        insertReading(x, y, z);
    }
    update();
}

void
MoveGraph::insertReading(int x, int y, int z)
{
    int oldOffset = offset;
    offset = (offset + 1) % (sizeof(readings)/sizeof(readings[0]));

    Reading *r = &readings[offset];
    Reading *o = &readings[oldOffset];

    float alpha = .08;

    r->x = o->x + alpha * (x - o->x);
    r->y = o->y + alpha * (y - o->y);
    r->z = o->z + alpha * (z - o->z);
}

