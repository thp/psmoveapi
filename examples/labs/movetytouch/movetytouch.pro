
CONFIG += debug

TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

SOURCES += main.cpp

SOURCES += demoview.cpp
HEADERS += demoview.h

SOURCES += eventmapper.cpp
HEADERS += eventmapper.h

HEADERS += movetytouch.h
HEADERS += debugoutput.h

DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include

LIBS += -L../../../build/ -lpsmoveapi -lpsmoveapi_tracker -lm

