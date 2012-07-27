
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

CONFIG += qt3d

SOURCES += main.cpp
SOURCES += ../../external/MadgwickAHRS/MadgwickAHRS.c

SOURCES += orientationview.cpp
HEADERS += orientationview.h

HEADERS += orientation.h

DEPENDPATH += ../..
INCLUDEPATH += ../..

DEPENDPATH += ../../external
INCLUDEPATH += ../../external

LIBS += -L../../build/ -lpsmoveapi

