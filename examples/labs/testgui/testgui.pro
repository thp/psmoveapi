
#
# PS Move API - An interface for the PS Move Motion Controller
# Copyright (c) 2011 Thomas Perl <m@thp.io>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
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

DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include

DEPENDPATH += ../../../bindings/qt
INCLUDEPATH += ../../../bindings/qt

LIBS += -L../../../build -lpsmoveapi -L../../../build/bindings/qt -lpsmoveapiqt

macx {
    ICON = psmove.icns
}

