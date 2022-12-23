
TEMPLATE = app

QT += widgets

SOURCES += main.cpp

HEADERS += movegraph.h
SOURCES += movegraph.cpp

DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include ../../../build

LIBS += -L../../../build/ -lpsmoveapi

