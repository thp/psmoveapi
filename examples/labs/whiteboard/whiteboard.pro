
TEMPLATE = app

QT += widgets

SOURCES += main.cpp

SOURCES += mainwindow.cpp
HEADERS += mainwindow.h

HEADERS += mapping.h

INCLUDEPATH += ../../../include/ ../../../build
LIBS += -L../../../build/ -lpsmoveapi -lpsmoveapi_tracker

