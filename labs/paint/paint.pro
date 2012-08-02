
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

SOURCES += main.cpp

SOURCES += paintview.cpp
HEADERS += paintview.h

HEADERS += orientation.h

DEPENDPATH += ../..
INCLUDEPATH += ../..

LIBS += -L../../build/ -lpsmoveapi -lopencv_highgui

