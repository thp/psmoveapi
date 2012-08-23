
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

SOURCES += main.cpp

SOURCES += paintview.cpp
HEADERS += paintview.h

HEADERS += orientation.h

DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include

LIBS += -L../../../build/ -lpsmoveapi -lpsmoveapi_tracker

