
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


#include <QApplication>

#include "movetytouch.h"
#include "debugoutput.h"
#include "demoview.h"
#include "eventmapper.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    MovetyTouch movetytouch;
    DebugOutput output;
    DemoView demoview;
    EventMapper eventmapper;

    QObject::connect(&movetytouch, SIGNAL(pressed(int,int,int)),
            &output, SLOT(pressed(int,int,int)));
    QObject::connect(&movetytouch, SIGNAL(released(int,int,int)),
            &output, SLOT(released(int,int,int)));
    QObject::connect(&movetytouch, SIGNAL(motion(int,int,int)),
            &output, SLOT(motion(int,int,int)));

    QObject::connect(&movetytouch, SIGNAL(pressed(int,int,int)),
            &eventmapper, SLOT(pressed(int,int,int)));
    QObject::connect(&movetytouch, SIGNAL(released(int,int,int)),
            &eventmapper, SLOT(released(int,int,int)));
    QObject::connect(&movetytouch, SIGNAL(motion(int,int,int)),
            &eventmapper, SLOT(motion(int,int,int)));

    QObject::connect(&movetytouch, SIGNAL(hover(int,int,int)),
            &demoview, SLOT(hover(int,int,int)));

    QObject::connect(&eventmapper, SIGNAL(startDrag(int,int)),
            &demoview, SLOT(startDrag(int,int)));
    QObject::connect(&eventmapper, SIGNAL(updateDrag(int,int)),
            &demoview, SLOT(updateDrag(int,int)));
    QObject::connect(&eventmapper, SIGNAL(stopDrag(int,int)),
            &demoview, SLOT(stopDrag(int,int)));

    QObject::connect(&eventmapper, SIGNAL(startZoom(int,int,float,float)),
            &demoview, SLOT(startZoom(int,int,float,float)));
    QObject::connect(&eventmapper, SIGNAL(updateZoom(int,int,float,float)),
            &demoview, SLOT(updateZoom(int,int,float,float)));
    QObject::connect(&eventmapper, SIGNAL(stopZoom(int,int,float,float)),
            &demoview, SLOT(stopZoom(int,int,float,float)));

    QObject::connect(&movetytouch,
            SIGNAL(startRotation(int,int,int,float,float,float,float)),
            &demoview,
            SLOT(startRotation(int,int,int,float,float,float,float)));

    QObject::connect(&movetytouch,
            SIGNAL(updateRotation(int,float,float,float,float)),
            &demoview,
            SLOT(updateRotation(int,float,float,float,float)));

    QObject::connect(&movetytouch, SIGNAL(stopRotation(int)),
            &demoview, SLOT(stopRotation(int)));

    movetytouch.start();
    demoview.showFullScreen();

    return app.exec();
}

