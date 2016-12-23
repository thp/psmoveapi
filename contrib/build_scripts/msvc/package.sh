#!/bin/bash

set -e
set -x

# Git revision identifier
PSMOVEAPI_REVISION=$(git describe --long --tags)

DEST="psmoveapi-${PSMOVEAPI_REVISION}-msvc"

mkdir -p "$DEST"
cp -rv \
    README COPYING \
    include \
    bindings/python \
    build/Release/psmove.exe \
    build/Release/psmoveapi.dll \
    build/Release/psmoveapi_tracker.dll \
    "$DEST"
rm -f "$DEST"/include/*.h.in
cp build/psmove_config.h "$DEST"/include/
7z a upload/"$DEST".zip "$DEST"
rm -rf "$DEST"
