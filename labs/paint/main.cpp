
#include <QtGui>
#include "paintview.h"
#include "orientation.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    PaintView view;

    Orientation orientation;
    orientation.start();

    QObject::connect(&orientation, SIGNAL(newposition(qreal,qreal,qreal,qreal)),
            &view, SLOT(newposition(qreal,qreal,qreal,qreal)));
    QObject::connect(&orientation, SIGNAL(newcolor(int,int,int)),
            &view, SLOT(newcolor(int,int,int)));
    QObject::connect(&orientation, SIGNAL(backup_frame()),
            &view, SLOT(backup_frame()));
    QObject::connect(&orientation, SIGNAL(restore_frame()),
            &view, SLOT(restore_frame()));

    view.show();

    return app.exec();
}

