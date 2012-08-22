
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


#include "movegraph.h"

MoveGraph::MoveGraph()
    : QWidget(NULL),
      move(psmove_connect()),
      labelPositive("+1g"),
      labelNegative("-1g"),
      readings(),
      offset(0)
{
    psmove_dump_calibration(move);

    memset(&readings, 0, sizeof(readings));
}

MoveGraph::~MoveGraph()
{
    psmove_disconnect(move);
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
    if (psmove_poll(move)) {
        offset = (offset + 1) % MAX_READINGS;
        psmove_get_accelerometer_frame(move, Frame_FirstHalf,
                &readings[offset][0], &readings[offset][1], &readings[offset][2]);

        offset = (offset + 1) % MAX_READINGS;
        psmove_get_accelerometer_frame(move, Frame_SecondHalf,
                &readings[offset][0], &readings[offset][1], &readings[offset][2]);
    }
    update();
}

