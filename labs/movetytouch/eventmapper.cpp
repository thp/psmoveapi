
#include "eventmapper.h"

#include <math.h>

EventMapper::EventMapper()
    : QObject(),
      points(),
      activePoints()
{
}

void
EventMapper::pressed(int id, int x, int y)
{
    points[id].x = x;
    points[id].y = y;

    activePoints++;

    if (activePoints == 1) {
        emit startDrag(x, y);
    } else if (activePoints == 2) {
        emit stopDrag(points[1-id].x, points[1-id].y);

        Point c = center();
        emit startZoom(c.x, c.y, distance(), degrees());
    }
}

void
EventMapper::motion(int id, int x, int y)
{
    points[id].x = x;
    points[id].y = y;

    if (activePoints == 2) {
        Point c = center();
        emit updateZoom(c.x, c.y, distance(), degrees());
    } else if (activePoints == 1) {
        emit updateDrag(points[id].x, points[id].y);
    }
}

void
EventMapper::released(int id, int x, int y)
{
    points[id].x = x;
    points[id].y = y;

    activePoints--;

    if (activePoints == 1) {
        Point c = center();

        emit stopZoom(c.x, c.y, distance(), degrees());
        emit startDrag(points[1-id].x, points[1-id].y);
    } else if (activePoints == 0) {
        emit stopDrag(x, y);
    }
}

Point
EventMapper::center()
{
    Point result;

    result.x = (points[0].x + points[1].x) / 2;
    result.y = (points[0].y + points[1].y) / 2;

    return result;
}

float
EventMapper::distance()
{
    int dx = (points[0].x - points[1].x);
    int dy = (points[0].y - points[1].y);

    return sqrtf(dx*dx + dy*dy);
}

float
EventMapper::degrees()
{
    int dx = (points[0].x - points[1].x);
    int dy = (points[0].y - points[1].y);

    return atan2f(dy, dx) * 180 / M_PI;
}

