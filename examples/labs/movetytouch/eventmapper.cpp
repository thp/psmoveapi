
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

