
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

    movetytouch.start();
    demoview.show();

    return app.exec();
}

