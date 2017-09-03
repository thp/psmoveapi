Building PS Move API from source
================================


Building on macOS 10.12
-----------------------

You need to install the following requirements:

- `Homebrew`_

.. _`Homebrew`: http://brew.sh/

Install the dependencies::

    brew install cmake libtool automake

To build from source, you can use the build script::

    bash -e -x scripts/macos/build-macos


Building on Ubuntu 14.04
------------------------

Automatic build
~~~~~~~~~~~~~~~

A build script is provided which will take care of the build for you::

    bash -e -x scripts/linux/build-debian

Manual build
~~~~~~~~~~~~

1. Install the build-dependencies::

    sudo apt-get install \
        build-essential cmake \
        libudev-dev libbluetooth-dev libv4l-dev libopencv-dev \
        openjdk-7-jdk ant liblwjgl-java \
        python-dev \
        mono-mcs \
        swig3.0 \
        freeglut3-dev

2. Build the API (standalone - if you want to make changes)::

    cd ~/src/psmoveapi/
    mkdir build
    cd build
    JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64/ cmake ..
    make -j4


Building on Windows (Visual Studio)
-----------------------------------

You need to install the following requirements:

- `Visual Studio Community 2013`_ or `Visual Studio Community 2015`_
- `CMake`_

.. _`Visual Studio Community 2013`: http://www.visualstudio.com/en-us/news/vs2013-community-vs.aspx
.. _`Visual Studio Community 2015`: https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx
.. _`CMake`: http://www.cmake.org/cmake/resources/software.html

Automatic build
~~~~~~~~~~~~~~~

A build script is provided which will take care of the build for you. The
script will generate the Visual Studio solution and build everything in Debug
and Release.

Run the batch files in the "VS2015 x64 Native Tools Command Prompt" (you'll
find it in your start menu; the VS2013 variant should be named similarly)
from the PS Move API checkout folder.

For Visual Studio 2013, use::

    call scripts/visualc/build_msvc_2013.bat

For Visual Studio 2015, use::

    call scripts/visualc/build_msvc_2015.bat

Or browse to ``scripts/visualc/`` in Explorer and
double-click on ``build_msvc_2013.bat`` or ``build_msvc_2015.bat`` as desired.

The resulting binaries will be placed in ``build/[Debug/Release]``


Cross-Compiling for Windows (MinGW)
-----------------------------------

To cross-compile for Windows on Ubuntu::

    sudo apt-get install mingw-w64 cmake

To build manually without the tracker::

    mkdir build-win32
    cd build-win32
    cmake \
        -DCMAKE_TOOLCHAIN_FILE=../cmake/i686-w64-mingw32.toolchain \
        -DPSMOVE_BUILD_TRACKER=OFF \
        ..

    mkdir build-win64
    cd build-win64
    cmake \
        -DCMAKE_TOOLCHAIN_FILE=../cmake/x86_64-w64-mingw32.toolchain \
        -DPSMOVE_BUILD_TRACKER=OFF \
        ..

Or use the ready-made build script::

    sh -x scripts/mingw64/cross-compile x86_64-w64-mingw32
    sh -x scripts/mingw64/cross-compile i686-w64-mingw32



Building for the Pocket C.H.I.P
-------------------------------

PS Move API now supports the Pocket C.H.I.P, an embedded Linux computer
running a Debian-based operating system. The device has built-in Bluetooth,
WIFI, a standard-sized USB port and a 3.5mm headphone jack, making it
suitable for portable PS Move applications.

To build on a Pocket C.H.I.P, ``ssh`` into your device (or use the Terminal)
and then build the release tarball::

    bash -e -x scripts/pocketchip/install_dependencies.sh
    bash -e -x scripts/pocketchip/build.sh


Installation and Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to be able to use the PS Move Motion Controllers without ``root``
access, you need to install an udev rules file on your C.H.I.P::

    sudo cp contrib/99-psmove.rules /etc/udev/rules.d/

Also, not all kernels ship with the required ``hidraw`` support, you can
check if your kernel does by running the following command after bootup::

    dmesg | grep hidraw

A kernel with hidraw will print something like the following::

    [    1.265000] hidraw: raw HID events driver (C) Jiri Kosina

If your kernel does not have hidraw support, you should install the newest
Firmware for your Pocket C.H.I.P, and make sure to install all updates via ``apt``.
