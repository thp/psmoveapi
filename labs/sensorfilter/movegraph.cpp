
#include "movegraph.h"

MoveGraph::MoveGraph()
    : QWidget(NULL),
      move(psmove_connect()),
      filter(psmove_filter_new(move)),
      calibration(psmove_calibration_new(move)),
      labelPositive("+1g"),
      labelNegative("-1g"),
      readings(),
      offset(0)
{
    psmove_calibration_load(calibration);
    psmove_calibration_dump(calibration);

    memset(&readings, 0, sizeof(readings));
}

void
MoveGraph::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    int Yzero = height() / 2;

    painter.setPen(QColor(50, 50, 50));
    painter.drawLine(0, Yzero, width(), Yzero);
    int rightBorder = width() - 10 - qMax(labelPositive.size().width(),
            labelNegative.size().width());

    for (int g=offset%50; g<rightBorder; g+=50) {
        painter.drawLine(g, 0, g, height());
    }

    painter.drawLine(0, Yzero/2, rightBorder, Yzero/2);
    painter.drawLine(0, Yzero*3/2, rightBorder, Yzero*3/2);

    painter.setPen(Qt::white);
    painter.drawStaticText(rightBorder + 5, Yzero/2 - labelPositive.size().height()/2, labelPositive);
    painter.drawStaticText(rightBorder + 5, Yzero/2*3 - labelNegative.size().height()/2, labelNegative);

    int newValue;
    int oldValue = 0;
    int idx;

    for (int a=0; a<3; a++) {
        switch (a) {
            case 0: painter.setPen(Qt::red); break;
            case 1: painter.setPen(Qt::green); break;
            case 2: painter.setPen(Qt::blue); break;
            default: break;
        }

        for (int x=0; x<qMin(rightBorder, MAX_READINGS); x++) {
            idx = (offset-x) % MAX_READINGS;
            while (idx < 0) idx += MAX_READINGS;

            newValue = Yzero + readings[idx][a] * Yzero/2.;

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
    int a[3];

    if (psmove_poll(move)) {
        psmove_filter_update(filter);

        offset = (offset + 1) % MAX_READINGS;

        psmove_filter_get_accelerometer(filter, &a[0], &a[1], &a[2]);

        psmove_calibration_map(calibration, a, readings[offset], 3);
    }
    update();
}

void
MoveGraph::setAlpha(int alpha)
{
    psmove_filter_set_alpha(filter, (float)alpha / 100.);
}

