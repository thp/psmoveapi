
TEMPLATE = app

SOURCES += main.cpp

SOURCES += mainwindow.cpp
HEADERS += mainwindow.h

HEADERS += Mapping.h

INCLUDEPATH += ../../../include/
LIBS += -L../../../build/ -lpsmoveapi -lpsmoveapi_tracker

