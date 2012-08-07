
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

SOURCES += main.cpp

SOURCES += paintview.cpp
HEADERS += paintview.h

HEADERS += orientation.h

DEPENDPATH += ../..
INCLUDEPATH += ../..

CONFIG += link_pkgconfig
PKGCONFIG += opencv

LIBS += -L../../build/ -lpsmoveapi

