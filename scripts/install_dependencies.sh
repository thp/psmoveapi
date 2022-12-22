#!/bin/bash

set -e
set -x

UNAME=$(uname)

case "$UNAME" in
    Linux)
        sudo apt-get update -qq
        sudo apt-get install -q -y mingw-w64 g++-mingw-w64
        sudo apt-get install -q -y build-essential cmake             \
                                   libudev-dev libbluetooth-dev      \
                                   libdbus-1-dev                     \
                                   libv4l-dev libopencv-dev          \
                                   python3-dev                       \
                                   libxrandr-dev libxinerama-dev libxcursor-dev \
                                   python3-sphinx python3-pip \
                                   libusb-dev libsdl2-dev
        ;;
    Darwin)
        brew update
        brew install --force cmake git libtool automake autoconf python libusb-compat sdl2 || true
        brew unlink libtool ; brew link --overwrite libtool
        pip3 install --user sphinx
        ;;
    MINGW64_NT-*)
        # Nothing to do, Windows OS
        ;;
    *)
        echo "Unknown OS: $UNAME"
        exit 1
        ;;
esac
