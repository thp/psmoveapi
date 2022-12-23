
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

QT += widgets

SOURCES += main.cpp

SOURCES += paintview.cpp
HEADERS += paintview.h

HEADERS += orientation.h

DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include ../../../build

LIBS += -L../../../build/ -lpsmoveapi -lpsmoveapi_tracker

CONFIG += link_pkgconfig
PKGCONFIG += opencv4
