
CONFIG += debug

TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SOURCES += main.cpp

SOURCES += demoview.cpp
HEADERS += demoview.h

SOURCES += eventmapper.cpp
HEADERS += eventmapper.h

HEADERS += movetytouch.h
HEADERS += debugoutput.h

mac {
    QT_CONFIG -= no-pkg-config
    PKG_CONFIG = /usr/local/bin/pkg-config
}
CONFIG += link_pkgconfig
packagesExist(psmoveapi) {
    PKGCONFIG += psmoveapi
    LIBS += -lpsmoveapi -lpsmoveapi_tracker
} else {
    DEPENDPATH += ../../../include
    INCLUDEPATH += ../../../include ../../../build
    LIBS += -L../../../build/ -lpsmoveapi -lpsmoveapi_tracker
}
LIBS += -lm
