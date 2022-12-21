
TEMPLATE = app

QT += widgets multimedia

DEPENDPATH += .
INCLUDEPATH += .

DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include ../../../build

SOURCES += playbackspeed.cpp

LIBS += -L../../../build/ -lpsmoveapi

