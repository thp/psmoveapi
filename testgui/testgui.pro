#-------------------------------------------------
#
# Project created by QtCreator 2011-03-19T14:02:02
#
#-------------------------------------------------

QT       += core gui

TARGET = testgui
TEMPLATE = app


SOURCES += main.cpp\
        psmovetestgui.cpp

HEADERS  += psmovetestgui.h

FORMS    += psmovetestgui.ui

RESOURCES +=

SOURCES += ../qt/psmoveqt.cpp
HEADERS += ../qt/psmoveqt.hpp

SOURCES += ../psmove.c
HEADERS += ../psmove.h

HEADERS += ../hidapi/hidapi/hidapi.h

INCLUDEPATH += ..
INCLUDEPATH += ../qt
INCLUDEPATH += ../hidapi/hidapi

macx {
    SOURCES += ../hidapi/mac/hid.c
    LIBS += -framework IOKit -framework CoreFoundation
    ICON = psmove.icns
}

