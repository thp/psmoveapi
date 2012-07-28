
#include "paintview.h"

#include <QPainter>

PaintView::PaintView(QWidget *parent)
    : QWidget(parent),
      m_painting(640, 480),
      m_cursor(0, 0),
      m_color(Qt::red),
      m_has_old(false),
      m_old(0, 0)
{
    resize(m_painting.size());
}

PaintView::~PaintView()
{
}

void PaintView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.drawPixmap(0, 0, m_painting);

    painter.setBrush(m_color);
    painter.setPen(Qt::transparent);
    painter.drawRect(0, 0, 20, 20);

    painter.setBrush(Qt::transparent);
    painter.setPen(QPen(Qt::white, 3));
    painter.drawLine(m_cursor+QPointF(-10, -10), m_cursor+QPoint(10, 10));
    painter.drawLine(m_cursor+QPointF(10, -10), m_cursor+QPoint(-10, 10));
}

void PaintView::orientation(qreal a, qreal b, qreal c, qreal d,
        qreal scale, qreal x, qreal y, qreal trigger)
{
    int size = trigger * 10;

    x = 640 - x;

    m_cursor = QPoint(x, y);
    update();

    m_has_old = m_has_old && (size > 0);

    if (size) {
        QPainter painter(&m_painting);

        if (m_has_old) {
            painter.setBrush(Qt::transparent);
            painter.setPen(QPen(m_color, size));
            painter.drawLine(m_old, QPoint(x, y));
        }

        painter.setBrush(m_color);
        painter.setPen(Qt::transparent);

        //painter.drawEllipse(x-size/2, y-size/2, size, size);

        m_old = QPoint(x, y);
        m_has_old = (size > 0);
    }
}

void
PaintView::newcolor(int r, int g, int b)
{
    if (r+g+b == 0) {
        m_painting.fill(Qt::black);
    } else {
        m_color.setRgb(r, g, b);
    }
}

