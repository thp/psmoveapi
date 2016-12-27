#!/bin/bash
# PS Move API for Pocket C.H.I.P -- Build Script
# 2016-12-27 Thomas Perl <m@thp.io>

set -e
set -x

BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/../../"
cd "$BASE"

export PSMOVEAPI_CUSTOM_PLATFORM_NAME="pocketchip"
export PSMOVEAPI_CMAKE_EXTRA_ARGS="-DPSMOVE_USE_POCKET_CHIP=ON"
bash -e -x scripts/build_package.sh
