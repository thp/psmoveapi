Building PS Move API from source
================================

Note that the `PS Move API Github Releases`_ page contains
prebuilt library downloads, you might prefer to use those.

.. _`PS Move API GitHub Releases`: https://github.com/thp/psmoveapi/releases


Building on macOS 10.14
-----------------------

You need to install the following requirements:

- `Homebrew`_

.. _`Homebrew`: http://brew.sh/

Install the dependencies::

    bash -e -x scripts/install_dependencies.sh

To build from source, you can use the build script::

    bash -e -x scripts/macos/build-macos


Building on Ubuntu 18.04
------------------------

Install the dependencies::

    bash -e -x scripts/install_dependencies.sh

To build from source, you can use the build script::

    bash -e -x scripts/linux/build-debian


Building on Windows (Visual Studio)
-----------------------------------

You need to install the following requirements:

- `Visual Studio Community 2017`_ or newer
- `CMake`_ 3.9.1 or newer
- `Git`_


.. _`Visual Studio Community 2017`: https://www.visualstudio.com/en-us/downloads/
.. _`CMake`: http://www.cmake.org/cmake/resources/software.html
.. _`Git`: https://gitforwindows.org/

Automatic build
~~~~~~~~~~~~~~~

A build script is provided which will take care of the build for you. The
script will generate the Visual Studio solution and build everything in Debug
and Release.

Run the batch files in the "Developer Command Prompt for VS 2017" (you will
find it in the start menu folder "Visual Studio 2017", for VS 2013/2015 the
menu items should be named similarly) from the PS Move API checkout folder.

For Visual Studio 2017 and 64-bit use::

    call scripts/visualc/build_msvc.bat 2017 x64

For Visual Studio 2017 and 32-bit use::

    call scripts/visualc/build_msvc.bat 2017 x86

The resulting binaries will be placed in ``build-x86/`` (for the 32-bit build)
and ``build-x64/`` (for the 64-bit build).


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

    bash -x scripts/mingw64/cross-compile x86_64-w64-mingw32
    bash -x scripts/mingw64/cross-compile i686-w64-mingw32



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



Python bindings
---------------

Python bindings (among others) are built using SWIG. So make sure you have
that installed. CMake will let you know if SWIG could not be found in the
initial configure step. Look in CMake's output in the section "Language
bindings".

Also required is the Python library (``libpython-dev`` on Linux). If you
have multiple versions of Python installed (most likely some 2.x and 3.x)
chances are CMake decides to use the wrong one. Again, look in CMake's
output in the section "Language bindings" which version of the Python
library CMake is using for the build. Make sure it matches the version you
want to run your Python scripts with later. They must be the same!

If CMake does not choose the correct version right away, use the option
``PSMOVE_PYTHON_VERSION`` to set the desired one. Usually it is sufficient
to set this to either 2 or 3 (for Python 2 and 3, respectively), but minor
versions are also supported. So you could choose between building for
Python 2.6 and 2.7. If you are running CMake from the command line set the
version like so::

    cmake .. -DPSMOVE_PYTHON_VERSION=2

Check CMake's output to verify that the correct version is now found; some
flavor of Python 2 in this example. If CMake still uses the wrong one, try
removing all the files CMake generated in the ``build`` directory and run
again.

Testing the build
~~~~~~~~~~~~~~~~~

A lot of Python example scripts are provided in the ``examples/python/``
directory. They are laid out so that when you build the library (and its
Python bindings) in the customary ``build`` folder in the PSMove API
checkout, the Python examples should find the modules without needing to
install anything. We suggest you start with ``always.py`` which you can
directly call from within the ``build`` directory like so::

    python ../examples/python/always.py

This script does not require Bluetooth and should thus provide an easy
way to test the Python bindings. Simply connect your Move controller via
USB and run the script as shown above. If that is working, continue with
``pair.py`` to set everything up for using Bluetooth.

