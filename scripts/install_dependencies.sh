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
