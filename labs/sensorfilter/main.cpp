
#include "movegraph.h"

#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication::setGraphicsSystem("raster");
    QApplication app(argc, argv);

    QTimer timer;

    MoveGraph moveGraph;
    moveGraph.show();

    QObject::connect(&timer, SIGNAL(timeout()),
            &moveGraph, SLOT(readSensors()));

    timer.start(1);

    return app.exec();
}

