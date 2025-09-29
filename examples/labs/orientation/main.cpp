/**
 * PS Move API - An interface for the PS Move Motion Controller
 * (license header unchanged)
 */

#include <QApplication>
#include "orientationview.h"
#include "orientation.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    OrientationView view;   // QWidget that embeds a Qt3D window
    Orientation orientation;
    orientation.start();

    QObject::connect(&orientation,
        &Orientation::orientation,           // new function-pointer syntax
        &view,
        &OrientationView::orientation);

    view.resize(900, 700);
    view.show();

    return app.exec();
}
