
/**
 * PS Move API
 * Copyright (c) 2011 Thomas Perl <thp.io/about>
 * All Rights Reserved
 **/

#include <QtCore>
#include <QtGui>
#include <QtDeclarative>

#include <iostream>

#include "psmoveqt.hpp"

int
main(int argc, char *argv[])
{
    int count;

    count = PSMoveQt::count();

    if (count < 2) {
        std::cout << "Please connect at least 2 controllers." << std::endl;
    } else {
        std::cout << "Controllers connected: " << count << std::endl;
    }

    QApplication app(argc, argv);

    // Register the "PSMove" object for usage in QML
    PSMoveQt::registerQML();

    QDeclarativeView view;
    view.setWindowTitle("PS Move API - Qt Declarative Example (Multiple Controllers)");
    view.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:/multiple.qml"));
    view.show();

    return app.exec();
}

