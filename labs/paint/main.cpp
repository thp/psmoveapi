
#include <QtGui>
#include "paintview.h"
#include "orientation.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    PaintView view;

    Orientation orientation;
    orientation.start();

    QObject::connect(&orientation,
            SIGNAL(orientation(qreal,qreal,qreal,qreal,qreal,qreal,qreal,qreal)),
            &view,
            SLOT(orientation(qreal,qreal,qreal,qreal,qreal,qreal,qreal,qreal)));
    QObject::connect(&orientation,
            SIGNAL(newcolor(int,int,int)),
            &view,
            SLOT(newcolor(int,int,int)));

    view.show();

    return app.exec();
}

