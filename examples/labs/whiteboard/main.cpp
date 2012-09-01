#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication::setGraphicsSystem("raster");
    QApplication a(argc, argv);
    MainWindow w;
    w.showFullScreen();

    return a.exec();
}
