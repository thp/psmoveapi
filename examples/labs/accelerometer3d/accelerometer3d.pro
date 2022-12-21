
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

isEqual(QT_MAJOR_VERSION, 6): error("Requires Qt4 or Qt5. QGLWidget needs ported to QOpenGLWidget for work with Qt6")
isEqual(QT_MAJOR_VERSION, 5): QT += widgets
QT += opengl

SOURCES += main.cpp

SOURCES += view.cpp
HEADERS += view.h

HEADERS += orientation.h

mac {
    QT_CONFIG -= no-pkg-config
    PKG_CONFIG = /usr/local/bin/pkg-config
}
CONFIG += link_pkgconfig
packagesExist(psmoveapi) {
    PKGCONFIG += psmoveapi
} else {
    DEPENDPATH += ../../../include
    INCLUDEPATH += ../../../include ../../../build
    LIBS += -L../../../build/ -lpsmoveapi
}
LIBS += -lGLU -lglut
