
#include "paintview.h"

#include <stdio.h>

#include <QPainter>

PaintView::PaintView(QWidget *parent)
    : QWidget(parent),
      m_painting(640, 480),
      m_painting_backup(m_painting.size()),
      m_cursor(0, 0),
      m_color(Qt::black),
      m_image(NULL)
{
    resize(m_painting.size());

    m_painting.fill(Qt::transparent);
    m_painting_backup = m_painting;

    newcolor(255, 0, 0);
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

    painter.setBrush(m_color);
    painter.setPen(Qt::transparent);
    painter.drawRect(0, 0, 20, 20);

    painter.setBrush(Qt::transparent);
    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(m_cursor+QPointF(-10, -10), m_cursor+QPoint(10, 10));
    painter.drawLine(m_cursor+QPointF(10, -10), m_cursor+QPoint(-10, 10));

}

void PaintView::newposition(qreal scale, qreal x, qreal y, qreal trigger)
{
    int size = scale;
    x = m_painting.width() - x;

    m_cursor = QPoint(x, y);

    if (trigger) {
        QPainter painter(&m_painting);

        painter.setBrush(m_color);
        painter.setPen(Qt::transparent);
        painter.drawEllipse(x-size, y-size, size*2, size*2);
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
PaintView::newcolor(int r, int g, int b)
{
    m_color.setRgb(r, g, b);
    m_color.setAlpha(10);
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

