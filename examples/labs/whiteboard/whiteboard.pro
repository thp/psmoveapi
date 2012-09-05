
TEMPLATE = app

SOURCES += main.cpp

SOURCES += mainwindow.cpp
HEADERS += mainwindow.h

HEADERS += mapping.h

INCLUDEPATH += ../../../include/
LIBS += -L../../../build/ -lpsmoveapi -lpsmoveapi_tracker

