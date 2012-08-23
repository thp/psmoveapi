
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

CONFIG += qt3d

SOURCES += main.cpp

SOURCES += orientationview.cpp
HEADERS += orientationview.h

HEADERS += orientation.h

DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include

LIBS += -L../../../build/ -lpsmoveapi -lpsmoveapi_tracker -lopencv_highgui

