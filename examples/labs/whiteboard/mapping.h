#ifndef MAPPING_H
#define MAPPING_H

#include <QPointF>
#include <QRect>

#include <qmath.h>

class Mapping
{
public:
    Mapping()
        : a(0, 0), b(1, 0), c(1, 1), d(0, 1),
          m_rect(0, 0, 1, 1)
    {
    }

    void setRect(QRect rect)
    {
        m_rect = rect;
    }

    QRect rect() {
        return m_rect;
    }

    void set(QPointF *m)
    {
        this->a = m[0];
        this->b = m[1];
        this->c = m[2];
        this->d = m[3];
    }

    QPointF map(QPointF p)
    {
        // http://www.gamedev.net/topic/596392-uv-coordinate-on-a-2d-quadrilateral/page__p__4779201#entry4779201

        double C = (double)(a.y() - p.y()) * (d.x() - p.x()) - (double)(a.x() - p.x()) * (d.y() - p.y());
        double B = (double)(a.y() - p.y()) * (c.x() - d.x()) + (double)(b.y() - a.y()) * (d.x() - p.x()) - (double)(a.x() - p.x()) * (c.y() - d.y()) - (double)(b.x() - a.x()) * (d.y() - p.y());
        double A = (double)(b.y() - a.y()) * (c.x() - d.x()) - (double)(b.x() - a.x()) * (c.y() - d.y());

        double D = B * B - 4 * A * C;

        double u = (-B - qSqrt(D)) / (2 * A);

        double p1x = a.x() + (b.x() - a.x()) * u;
        double p2x = d.x() + (c.x() - d.x()) * u;
        double px = p.x();

        double v = (px - p1x) / (p2x - p1x);

        return QPointF(m_rect.x() + u*m_rect.width(),
                       m_rect.y() + v*m_rect.height());
    }

private:
    QPointF a, b, c, d;
    float w, h;
    QRect m_rect;
};

#endif // MAPPING_H
