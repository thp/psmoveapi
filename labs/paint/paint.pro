
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

SOURCES += main.cpp
SOURCES += ../../external/MadgwickAHRS/MadgwickAHRS.c

SOURCES += paintview.cpp
HEADERS += paintview.h

HEADERS += orientation.h

DEPENDPATH += ../..
INCLUDEPATH += ../..

DEPENDPATH += ../../external
INCLUDEPATH += ../../external

LIBS += -L../../build/ -lpsmoveapi -lopencv_highgui

