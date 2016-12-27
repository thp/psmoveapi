#!/bin/bash

set -e
set -x

UNAME=$(uname)

case "$UNAME" in
    Linux)
        sudo add-apt-repository --yes ppa:kubuntu-ppa/backports
        sudo add-apt-repository --yes ppa:hlprasu/swig-trusty-backports
        sudo apt-get update -qq
        sudo apt-get install -q -y mingw-w64 g++-mingw-w64
        sudo apt-get install -q -y build-essential cmake             \
                                   libudev-dev libbluetooth-dev      \
                                   libv4l-dev libopencv-dev          \
                                   openjdk-7-jdk ant liblwjgl-java   \
                                   python-dev mono-mcs               \
                                   swig3.0 freeglut3-dev
        ;;
    Darwin)
        brew update
        brew install --force cmake git libtool automake autoconf swig || true
        brew unlink libtool ; brew link --overwrite libtool
        ;;
    *)
        echo "Unknown OS: $UNAME"
        exit 1
        ;;
esac
