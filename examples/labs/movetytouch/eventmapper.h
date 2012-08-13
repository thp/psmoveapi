
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

#ifndef EVENTMAPPER_H
#define EVENTMAPPER_H

#include <QObject>
#include <QPoint>

typedef struct {
    int x;
    int y;
} Point;

class EventMapper : public QObject
{
    Q_OBJECT

    public:
        EventMapper();

    public slots:
        void pressed(int id, int x, int y);
        void motion(int id, int x, int y);
        void released(int id, int x, int y);

    signals:
        /* Drag'n'drop */
        void startDrag(int x, int y);
        void updateDrag(int x, int y);
        void stopDrag(int x, int y);

        /* Rotation and zoom */
        void startZoom(int x, int y, float distance, float degrees);
        void updateZoom(int x, int y, float distance, float degrees);
        void stopZoom(int x, int y, float distance, float degrees);

    private:
        Point center();
        float distance();
        float degrees();

        Point points[2];
        int activePoints;
};

#endif
