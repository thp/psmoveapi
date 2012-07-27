
#include <QtGui>
#include "orientationview.h"
#include "orientation.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    OrientationView view;

    Orientation orientation;
    orientation.start();

    QObject::connect(&orientation, SIGNAL(orientation(qreal,qreal,qreal,qreal)),
                     &view, SLOT(orientation(qreal,qreal,qreal,qreal)));

    view.show();

    return app.exec();
}

