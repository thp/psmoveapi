#include <QtGui/QApplication>
#include "psmovetestgui.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PSMoveTestGUI w;
    w.show();

    return a.exec();
}
