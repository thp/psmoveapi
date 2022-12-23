Building PS Move API from source
================================

Note that the `PS Move API Github Releases`_ page contains
prebuilt library downloads, you might prefer to use those.

.. _`PS Move API GitHub Releases`: https://github.com/thp/psmoveapi/releases

Requirements for all platforms:

- `CMake`_ 3.16 or newer (tested with 3.25.1)


Building on macOS 13
--------------------

You need to install the following requirements:

- `Homebrew`_

.. _`Homebrew`: http://brew.sh/

Install the dependencies::

    bash -e -x scripts/install_dependencies.sh

To build from source, you can use the build script::

    bash -e -x scripts/macos/build-macos

The build script builds for the CPU architecture of the current
build machine (using ``uname -m`` for detection). By default, on
an Apple Silicon machine, this builds for ``arm64``, and for
Intel machines, ``x86_64`` is the build target.


Building on Ubuntu 22.04
------------------------

Install the dependencies::

    bash -e -x scripts/install_dependencies.sh

To build from source, you can use the build script::

    bash -e -x scripts/linux/build-debian


Building on Windows (Visual Studio)
-----------------------------------

You need to install the following requirements:

- `Visual Studio Community 2022`_
- `Git`_


.. _`Visual Studio Community 2022`: https://visualstudio.microsoft.com/downloads/
.. _`CMake`: http://www.cmake.org/download/
.. _`Git`: https://git-scm.com/

Automatic build
~~~~~~~~~~~~~~~

A build script is provided which will take care of the build for you. The
script will generate the Visual Studio solution and build everything in Debug
and Release.

Run the batch files in a "Developer Command Prompt for VS 2022" (you will find
this in the start menu) from the PS Move API checkout folder.

For Visual Studio 2022 and 64-bit use::

    call scripts/visualc/build_msvc.bat 2022 x64

For Visual Studio 2022 and 32-bit use::

    call scripts/visualc/build_msvc.bat 2022 x86

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



Python bindings
---------------

For Python 3, use the ``ctypes``-based ``psmoveapi.py`` bindings. The old
SWIG-based bindings are not supported anymore.


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

