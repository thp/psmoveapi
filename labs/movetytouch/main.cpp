
#include <QCoreApplication>
#include "movetytouch.h"
#include "debugoutput.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    MovetyTouch movetytouch;
    movetytouch.start();

    DebugOutput output;

    QObject::connect(&movetytouch, SIGNAL(pressed(int,int,int)),
            &output, SLOT(pressed(int,int,int)));
    QObject::connect(&movetytouch, SIGNAL(released(int,int,int)),
            &output, SLOT(released(int,int,int)));
    QObject::connect(&movetytouch, SIGNAL(motion(int,int,int)),
            &output, SLOT(motion(int,int,int)));
    QObject::connect(&movetytouch, SIGNAL(hover(int,int)),
            &output, SLOT(hover(int,int)));

    return app.exec();
}

