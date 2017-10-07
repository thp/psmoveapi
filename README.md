![PS Move API](./contrib/header.png)
===========

[![Documentation Status](https://readthedocs.org/projects/psmoveapi/badge/?version=latest)](https://psmoveapi.readthedocs.io/en/latest)  [![Build Status](https://travis-ci.org/thp/psmoveapi.svg?branch=master)](https://travis-ci.org/thp/psmoveapi)  [![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/thp/psmoveapi?branch=master&svg=true)](https://ci.appveyor.com/project/thp/psmoveapi)

The PS Move API is an [open source](https://github.com/thp/psmoveapi/blob/master/COPYING) library for Linux, macOS and Windows to access the Sony Move Motion Controller via Bluetooth and USB directly from your PC without the need for a PS3. Tracking in 3D space is possible using a PS Eye (on Linux, Windows and macOS), an iSight camera (on macOS) or any other suitable camera source.

PS Move API has successfully participated in [Google Summer of Code 2012](http://www.google-melange.com/gsoc/homepage/google/gsoc2012). Detailed documentation can be found in [my master's thesis](http://thp.io/2012/thesis/) about sensor fusion.


Core Features
-------------

 * Pairing of Bluetooth controllers via USB
 * Setting LEDs and rumble via USB and Bluetooth
 * Reading inertial sensors and buttons via Bluetooth
 * Supporting extension devices (such as the Sharp Shooter and the Racing Wheel)
 * Tracking up to 5 controllers in 3D space via OpenCV
 * 3D orientation tracking via an open source AHRS algorithm
 * Sensor fusion for augmented and virtual reality applications

Supported Languages
-------------------

 * Core library written in C for portability and performance
 * SWIG-based bindings for Python, Java, Processing and C#
 * ctypes-based bindings for Python 3

Need Help?
----------

 * Free community-based support via the [PS Move Mailing List](https://groups.google.com/forum/#!aboutgroup/psmove)
 * Professional support and custom development [upon request](http://thp.io/about)

Hacking the Source
------------------

 * Coding style: No strict rules; keep consistent with the surrounding code
 * Patches should be submitted on Github as [pull request](https://github.com/thp/psmoveapi/pulls)
 * Bug reports and feature requests can be added to the [issue tracker](https://github.com/thp/psmoveapi/issues)

Licensing
---------

The PS Move API source code is released under the terms of a Simplified BSD-style license, the exact license text can be found in the [COPYING](https://github.com/thp/psmoveapi/blob/master/COPYING) file.

Some third party code under ["external/"](https://github.com/thp/psmoveapi/blob/master/external) might be licensed under a different license. Compiling PS Move API with these modules is optional, you can use CMake options to configure which features you need. CMake will give you a hint about the library licensing for your current configuration depending on your options at configure time.

In general, all dependencies are under a MIT- or BSD-style license, with the
exception of the following dependencies:

 - PS3EYEDriver: Released under the MIT license, parts based on GPL code

   For interfacing with the PSEye camera on macOS (only in the tracker)

   CMake option: `PSMOVE_USE_PS3EYE_DRIVER` (disabled by default)

More information about the third party modules and licenses:

  - http://thp.io/2012/thesis/thesis.pdf (page 51-53)
  - File ["external/README"](https://github.com/thp/psmoveapi/blob/master/external/README) in this source tree


More Information
----------------

 * License: Simplified BSD-style license (see [COPYING](https://github.com/thp/psmoveapi/blob/master/COPYING) and "Licensing" above)
 * Maintainer: Thomas Perl <m@thp.io>
 * Website: http://thp.io/2010/psmove/
 * Git repository: https://github.com/thp/psmoveapi
 * Mailing list: psmove@googlegroups.com (see the [web interface](https://groups.google.com/forum/#!aboutgroup/psmove) for details)
 * Documentation: https://psmoveapi.readthedocs.io/


