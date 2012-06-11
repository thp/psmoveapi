
#include "movegraph.h"

#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication::setGraphicsSystem("raster");
    QApplication app(argc, argv);

    QMainWindow mainWindow;

    mainWindow.resize(500, 300);
    mainWindow.setWindowTitle("PS Move API Labs - Sensor Filter");

    QWidget centralWidget;
    QBoxLayout layout(QBoxLayout::LeftToRight);
    centralWidget.setLayout(&layout);

    QTimer timer;

    QSlider slider;
    layout.addWidget(&slider);

    slider.setRange(0, 100);

    MoveGraph moveGraph;
    layout.addWidget(&moveGraph);
    mainWindow.setCentralWidget(&centralWidget);

    QObject::connect(&timer, SIGNAL(timeout()),
            &moveGraph, SLOT(readSensors()));

    QObject::connect(&slider, SIGNAL(valueChanged(int)),
            &moveGraph, SLOT(setAlpha(int)));

    slider.setValue(50);
    moveGraph.setAlpha(50);

    timer.start(1);

    mainWindow.show();

    return app.exec();
}

