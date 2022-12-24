#!/bin/bash

set -e
set -x

BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/../"
cd "$BASE"

pkg_tarball() {
    PLATFORM_PKG="tar czvf"
    PLATFORM_PKG_EXT=".tar.gz"
}

pkg_zipfile_7z() {
    PLATFORM_PKG="7z a -bb1"
    PLATFORM_PKG_EXT=".zip"
}

pkg_zipfile_zip() {
    PLATFORM_PKG="zip -r"
    PLATFORM_PKG_EXT=".zip"
}

if [ -z "$BUILD_TYPE" ]; then
    echo "\$BUILD_TYPE is not set, trying to auto-detect"
    UNAME=$(uname)
    case "$UNAME" in
        Darwin)
            BUILD_TYPE="macos-native-clang-$(uname -m)"
            ;;
        Linux)
            BUILD_TYPE="linux-native-clang"
            ;;
        MINGW64_NT-*)
            BUILD_TYPE="windows-native-msvc-x64"
            ;;
        *)
            echo "Could not auto-detect build type, please set \$BUILD_TYPE"
            exit 1
            ;;
    esac
fi

case "$BUILD_TYPE" in
    linux-native-clang)
        BUILDDIR=build
        PLATFORM_BIN="
        $BUILDDIR/psmove
        "
        PLATFORM_LIB="
        $BUILDDIR/libpsmoveapi.so
        $BUILDDIR/libpsmoveapi_tracker.so
        "
        JAVA_JAR="$BUILDDIR/psmoveapi.jar"
        JAVA_NATIVE="$BUILDDIR/libpsmove_java.so"
        CSHARP_NATIVE="$BUILDDIR/psmoveapi_csharp.so"
        PROCESSING_BINDINGS="$BUILDDIR/psmove_processing_linux.zip"

        pkg_tarball

        PLATFORM_NAME="linux"
        bash -e -x scripts/linux/build-debian
        ;;
    linux-cross-mingw*)
        BUILDDIR=build
        PLATFORM_BIN="
        $BUILDDIR/psmove.exe
        "
        PLATFORM_LIB="
        $BUILDDIR/libpsmoveapi.dll
        $BUILDDIR/libpsmoveapi_tracker.dll
        "
        JAVA_JAR="$BUILDDIR/psmoveapi.jar"
        JAVA_NATIVE="$BUILDDIR/psmove_java.dll"
        CSHARP_NATIVE="$BUILDDIR/psmoveapi_csharp.dll"
        PROCESSING_BINDINGS="$BUILDDIR/psmove_processing_windows.zip"

        pkg_zipfile_zip

        case "$BUILD_TYPE" in
            linux-cross-mingw64)
                PLATFORM_NAME="mingw64"
                bash -e -x scripts/mingw64/cross-compile x86_64-w64-mingw32
                ;;
            linux-cross-mingw32)
                PLATFORM_NAME="mingw32"
                bash -e -x scripts/mingw64/cross-compile i686-w64-mingw32
                ;;
            *)
                echo "Invalid \$BUILD_TYPE: '$BUILD_TYPE'"
                exit 1
                ;;
        esac
        ;;
    macos-native-clang-*)
        MAC_ARCH=${BUILD_TYPE#macos-native-clang-}
        BUILDDIR=build-$MAC_ARCH
        PLATFORM_BIN="
        $BUILDDIR/psmove
        "
        PLATFORM_LIB="
        $BUILDDIR/libpsmoveapi.dylib
        $BUILDDIR/libpsmoveapi_tracker.dylib
        "
        pkg_tarball

        JAVA_JAR="$BUILDDIR/psmoveapi.jar"
        JAVA_NATIVE="$BUILDDIR/libpsmove_java.jnilib"
        CSHARP_NATIVE="$BUILDDIR/psmoveapi_csharp.so"
        PROCESSING_BINDINGS="$BUILDDIR/psmove_processing_macosx.zip"

        PLATFORM_NAME="macos-$MAC_ARCH"
        bash -e -x scripts/macos/build-macos "$MAC_ARCH"
        ;;
    windows-native-msvc-*)
        WIN_ARCH=${BUILD_TYPE#windows-native-msvc-}
        BUILDDIR="build-${WIN_ARCH}"
        PLATFORM_BIN="
        $BUILDDIR/psmove.exe
        "
        PLATFORM_LIB="
        $BUILDDIR/psmoveapi.dll
        $BUILDDIR/psmoveapi.lib
        $BUILDDIR/psmoveapi_tracker.dll
        $BUILDDIR/psmoveapi_tracker.lib
        "
        JAVA_JAR="$BUILDDIR/psmoveapi.jar"
        JAVA_NATIVE="$BUILDDIR/psmove_java.dll"
        CSHARP_NATIVE="$BUILDDIR/psmoveapi_csharp.dll"
        PROCESSING_BINDINGS="$BUILDDIR/psmove_processing_windows.zip"

        pkg_zipfile_7z

        PLATFORM_NAME="windows-msvc2022-${WIN_ARCH}"
        chmod +x ./scripts/visualc/build_msvc.bat
        ./scripts/visualc/build_msvc.bat 2022 ${WIN_ARCH}
        ;;
    *)
        echo "Invalid/unknown \$BUILD_TYPE value: '$BUILD_TYPE'"
        exit 1
        ;;
esac

# Git revision identifier
PSMOVEAPI_REVISION=$(git describe --tags)

DEST="psmoveapi-${PSMOVEAPI_REVISION}-${PLATFORM_NAME}"
mkdir -p "$DEST"

cp -v README.md COPYING "$DEST/"

make -C docs html || echo "Not building docs"
if [ -d docs/_build/html ]; then
    cp -rv docs/_build/html "$DEST/docs/"
fi

mkdir -p "$DEST/include"
cp -v include/*.h $BUILDDIR/psmove_config.h "$DEST/include/"

mkdir -p "$DEST/bindings/python"
cp -rv bindings/python/psmoveapi.py "$DEST/bindings/python/"

if [ -f "$JAVA_JAR" ]; then
    mkdir -p "$DEST/bindings/java"
    cp -rv $JAVA_JAR "$DEST/bindings/java/"
    cp -rv $JAVA_NATIVE "$DEST/bindings/java/"
fi

if [ -f "$CSHARP_NATIVE" ]; then
    mkdir -p "$DEST/bindings/csharp"
    cp -rv $CSHARP_NATIVE "$DEST/bindings/csharp"
    cp -v "$BUILDDIR"/*.cs "$DEST/bindings/csharp"
fi

if [ -f "$PROCESSING_BINDINGS" ]; then
    mkdir -p "$DEST/bindings/processing"
    cp -rv $PROCESSING_BINDINGS "$DEST/bindings/processing"
fi

if [ ! -z "$PLATFORM_BIN" ]; then
    mkdir -p "$DEST/bin"
    cp -v $PLATFORM_BIN "$DEST/bin/"
fi

if [ ! -z "$PLATFORM_LIB" ]; then
    mkdir -p "$DEST/lib"
    cp -v $PLATFORM_LIB "$DEST/lib/"
fi

mkdir -p upload
$PLATFORM_PKG "upload/${DEST}${PLATFORM_PKG_EXT}" "$DEST"
rm -rf "$DEST"
