TEMPLATE = app
TARGET = orientation

CONFIG += c++17
QT += core gui widgets 3dcore 3drender 3dinput 3dextras 3dlogic

# Sources / headers
SOURCES += \
    main.cpp \
    orientationview.cpp

HEADERS += \
    orientationview.h \
    orientation.h

# PS Move include/lib layout as before
DEPENDPATH += .
INCLUDEPATH += .
DEPENDPATH += ../../../include
INCLUDEPATH += ../../../include

# NEW: where your psmove landed
INCLUDEPATH += /usr/local/include
# Sometimes headers live in a nested folder, add this too if needed:
INCLUDEPATH += /usr/local/include/psmoveapi

# Link PS Move and tracker
LIBS += -L../../../build/ -lpsmoveapi -lpsmoveapi_tracker

# Optional: OpenCV (comment out if not available)
CONFIG += link_pkgconfig
PKGCONFIG += opencv4

# On some distros you might need pthread explicitly
unix:LIBS += -lpthread
