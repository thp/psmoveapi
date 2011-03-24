
#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (C) 2011 Thomas Perl <m@thp.io>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#

QT       += core gui

TARGET = testgui
TEMPLATE = app


SOURCES += main.cpp\
        psmovetestgui.cpp \
    psmovesensorscene.cpp \
    devicechooserdialog.cpp

HEADERS  += psmovetestgui.h \
    psmovesensorscene.h \
    devicechooserdialog.h

FORMS    += psmovetestgui.ui \
    devicechooserdialog.ui

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
    LIBS += -framework IOKit -framework CoreFoundation -framework IOBluetooth
    ICON = psmove.icns
}

