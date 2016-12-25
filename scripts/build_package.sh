#!/bin/bash

set -e
set -x

BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/../"
cd "$BASE"

case "$BUILD_TYPE" in
    linux-native-clang)
        bash -e -x scripts/linux/build-debian
        ;;
    linux-cross-mingw64)
        bash -e -x scripts/mingw64/cross-compile x86_64-w64-mingw32
        ;;
    linux-cross-mingw32)
        bash -e -x scripts/mingw64/cross-compile i686-w64-mingw32
        ;;
    macos-native-clang)
        bash -e -x scripts/macos/build-macos
        ;;
    *)
        echo "Invalid/unknown \$BUILD_TYPE value: '$BUILD_TYPE'"
        exit 1
        ;;
esac
