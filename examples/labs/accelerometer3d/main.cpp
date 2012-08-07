
#include <QtGui>

#include "orientation.h"
#include "view.h"

#include <GL/glut.h>

int main(int argc, char **argv)
{
    glutInit(&argc, argv);

    QApplication app(argc, argv);

    View view;
    Orientation orientation;

    QObject::connect(&orientation, SIGNAL(updateQuaternion(QQuaternion)),
            &view, SLOT(updateQuaternion(QQuaternion)));

    QObject::connect(&orientation, SIGNAL(updateAccelerometer(QVector3D)),
            &view, SLOT(updateAccelerometer(QVector3D)));

    QObject::connect(&orientation, SIGNAL(updateButtons(int,int)),
            &view, SLOT(updateButtons(int,int)));

    view.show();
    orientation.start();

    return app.exec();
}

