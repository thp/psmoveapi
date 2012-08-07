
TEMPLATE = app

SOURCES += main.cpp

HEADERS += movegraph.h
SOURCES += movegraph.cpp

DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include

LIBS += -L../../../build/ -lpsmoveapi

