
TEMPLATE = app

DEPENDPATH += .
INCLUDEPATH += .

DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include

CONFIG += mobility
MOBILITY += multimedia

SOURCES += playbackspeed.cpp

LIBS += -L../../../build/ -lpsmoveapi

