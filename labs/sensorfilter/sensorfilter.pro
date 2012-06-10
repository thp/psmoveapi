
TEMPLATE = app
DEPENDPATH += . ../..
INCLUDEPATH += . ../..
LIBS += -L../../build/ -lpsmoveapi

SOURCES += main.cpp

HEADERS += movegraph.h
SOURCES += movegraph.cpp

