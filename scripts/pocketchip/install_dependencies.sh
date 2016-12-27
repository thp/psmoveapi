#!/bin/bash
# PS Move API for Pocket C.H.I.P -- Dependencies Installer
# 2016-12-27 Thomas Perl <m@thp.io>

set -e
set -x

sudo apt-get update -qq
sudo apt-get install -q -y build-essential cmake             \
                           libudev-dev libbluetooth-dev      \
                           libv4l-dev libopencv-dev          \
                           python-dev swig3.0 freeglut3-dev  \
                           python-sphinx
