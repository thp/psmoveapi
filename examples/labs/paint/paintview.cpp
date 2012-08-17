
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


#include "paintview.h"

#include <stdio.h>

#include <QPainter>

#include "psmove_tracker.h"

PaintView::PaintView(QWidget *parent, int width, int height)
    : QWidget(parent),
      m_painting(width, height),
      m_painting_backup(m_painting.size()),
      m_cursor(),
      m_color(),
      m_image(NULL)
{
    resize(m_painting.size());

    m_painting.fill(Qt::transparent);
    m_painting_backup = m_painting;

    m_cursor[0] = QPoint(-50, -50);
    m_cursor[1] = QPoint(-50, -50);

    newcolor(0, 255, 255, 255);
    newcolor(1, 0, 0, 0);
}

PaintView::~PaintView()
{
    if (m_image) {
        delete m_image;
    }
}

void PaintView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    if (m_image) {
        painter.drawImage(0, 0, m_image->rgbSwapped().mirrored(true, false));
    }

    painter.drawPixmap(0, 0, m_painting);

    for (int i=0; i<2; i++) {
        painter.setBrush(m_color[i]);
        painter.setPen(Qt::transparent);
        painter.drawRect(i*20, 0, 20, 20);

        painter.setBrush(Qt::transparent);
        painter.setPen(QPen(m_color[i], 2));
        painter.drawLine(m_cursor[i]+QPointF(-10, -10), m_cursor[i]+QPoint(10, 10));
        painter.drawLine(m_cursor[i]+QPointF(10, -10), m_cursor[i]+QPoint(-10, 10));
    }

}

void PaintView::newposition(int id, qreal scale, qreal x, qreal y, qreal trigger)
{
    int size = 3;//scale;
    x = m_painting.width() - x;

    QPoint old_position = m_cursor[id];
    m_cursor[id] = QPoint(x, y);

    if (trigger) {
        QPainter painter(&m_painting);

        painter.setBrush(Qt::white);
        painter.setPen(QPen(m_color[id], size));//Qt::transparent);
        //painter.drawEllipse(x-size, y-size, size*2, size*2);
        painter.drawLine(old_position, m_cursor[id]);
    }

    update();
}

void
PaintView::backup_frame()
{
    m_painting_backup = m_painting;
}

void
PaintView::restore_frame()
{
    m_painting = m_painting_backup;
}

void
PaintView::newcolor(int id, int r, int g, int b)
{
    m_color[id].setRgb(r, g, b);
    //m_color[id].setAlpha(10);
}

void
PaintView::newimage(void *image)
{
    static void *oldptr = NULL;

    if (image != oldptr) {
        oldptr = image;

        IplImage *img = (IplImage*)image;
        assert(img->nChannels == 3);
        assert(img->depth == 8);

        m_image = new QImage((uchar*)img->imageData,
                img->width,
                img->height,
                img->widthStep,
                QImage::Format_RGB888);
    }
}

