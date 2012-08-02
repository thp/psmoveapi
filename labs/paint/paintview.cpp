
#include "paintview.h"

#include <QPainter>

PaintView::PaintView(QWidget *parent)
    : QWidget(parent),
      m_painting(640, 480),
      m_painting_backup(640, 480),
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
    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(m_cursor+QPointF(-10, -10), m_cursor+QPoint(10, 10));
    painter.drawLine(m_cursor+QPointF(10, -10), m_cursor+QPoint(-10, 10));
}

void PaintView::newposition(qreal scale, qreal x, qreal y, qreal trigger)
{
    int size = trigger?scale:0;//trigger * 30;

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

