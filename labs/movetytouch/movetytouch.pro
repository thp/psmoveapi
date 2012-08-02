
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

SOURCES += main.cpp

HEADERS += movetytouch.h
HEADERS += debugoutput.h

DEPENDPATH += ../..
INCLUDEPATH += ../..

CONFIG += link_pkgconfig
PKGCONFIG += opencv

LIBS += -L../../build/ -lpsmoveapi

