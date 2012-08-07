
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

QT += opengl

SOURCES += main.cpp

SOURCES += view.cpp
HEADERS += view.h

HEADERS += orientation.h

DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include

LIBS += -L../../../build/ -lpsmoveapi -lGLU -lglut

