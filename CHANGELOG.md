# Changelog

All notable changes to this project will be documented in this file.

The format mostly follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)
starting after version 4.0.12, but historic entries might not.

## [Unreleased]

### Added

- macOS: Add build support for M1/Apple Silicon (Fixes #440)
- Linux/Debian: Force building tracker and test programs (Fixes #437)
- Added `psmove_tracker_opencv.h` for OpenCV-specific functions
- `psmove_fusion_glm.h`: Convenience wrapper functions returning `glm` types
- `psmove_tracker_glm.h`: Convenience wrapper functions returning `glm` types
- `psmove_tracker_count_connected()` now returns the number of connected V4L2 video devices on Linux
- `psmove_tracker_get_next_unused_color()` for previewing the next tracked color

### Changed

- CI: Migrate from Travis CI (macOS, Linux), and AppVeyor (Windows) to Github Actions
- Linux: Build OpenCV from source
- `CMakeLists.txt`: Add check for 'git submodule init' (Fixes #352)
- New binary magnetometer calibration format (Fixes #452); this changes the
  file format and controllers might need to be re-calibrated after the update;
  the filename also changed from "BTADDR.magnetometer.csv" to "BTADDR.magnetometer.dat"
- Update build instructions for newer versions
- Updated OpenCV to Version 4 (some feature such as camera and distance calibration
  are currently not ported to OpenCV 4 and have been disabled for now)
- `external/libusb-1.0`: Updated to revision f3619c40 for VS2022 support
- `external/PS3EYEDriver`: Update to latest revision
- `psmove firmware-info`: List firmware info for all connected controllers (not just the first one),
  print device model number
- `psmove_fusion_get_position()` now determines the Z coordinate of the controller
  analytically based on the current projection and projected size on the X axis
- `external/glm`: Update to version 0.9.9.8
- New logging system and macros, includes source file names and line numbers
- CMake minimum requirement version bumped to version 3.16 (the version in Ubuntu 20.04 LTS)
- Require C++14 support (previously was C++11)
- Slightly improved annotation overlay for `psmove_tracker_annotate()`
- Camera tracking-related tools are now merged into the `psmove` CLI utility
- `psmove_tracker_annotate()` got additional parameters for showing/hiding the status bar and ROIs
- Tracker dimming factor is now limited to between 0.01 (1% intensity) and 1.0 (100% intensity)
- The MinGW builds now explicitly require Windows 7 as minimum version
- Camera devices on Linux - when using OpenCV - are now properly detected and enumerated even if
  there are "holes" in the numbering (e.g. /dev/video1 exists, but /dev/video0 does not)

### Fixed

- Fixed linking on macOS (`libSecurity`)
- Fix macOS version detection for macOS 11 and newer (fixes #456)
- `examples/labs/`: Fix building of Qt examples by migrating to Qt 5
- Fixed the kernel center of CV-related image filters (was off-center before)

### Removed

- Remove obsolete support for Travis CI/Ubuntu 14.04
- `contrib/convert-include-guards.py`: Remove (+ change some remaining include guards)
- Removed support scripts and documentation for building in Pocket C.H.I.P
- Removed support for MSVC versions older than 2022
- Removed `libusb_dynamic_crt.patch`
- Removed SWIG-based language bindings (use the existing `ctypes`-based Python bindings instead) (Fixes #338)
- Removed support for macOS iSight exposure locking / calibration; PS3EYEDriver replaces it (Fixes #53)
- Removed support for the proprietary CL Eye Driver + Registry settings on Windows
- Removed `psmove_tracker_get_frame()` (replaced with `psmove_tracker_opencv_get_frame()`)
- Removed legacy OpenGL examples and glfw3 (only used for the OpenGL examples)
- Removed support for the `PSMOVE_TRACKER_COLOR` environment variable


## [4.0.12] - 2020-12-19

- Fix flipping of camera image on Linux (by @nitsch)
- Package SWIG-generated .cs files (by @nitsch)
- Fix macOS build in Travis CI
- Only link libusb if sixpair is to be used (Fixes #406)
- `install_dependencies.sh`: Use Python 3
- Various clean-ups, packaging improvements, etc...


## [4.0.11] - 2020-09-25

- Bindings
 - Install SWIG in AppVeyor builds
 - Generally improve SWIG-based binding building
 - Fix packaging of bindings on Windows
 - Re-enable building SWIG-based bindings on macOS
 - Fixes to Java and Csharp bindings (by Rafael Lago)
 - Bindings: Add support for Java 8 and above (by Rafael Lago)
 - Package processing bindings in release (Fixes #421)
- Tooling
 - Add function for freeing memory allocated by the lib (helps with multiple C runtimes)
 - Free internally allocated memory using the new API
- Windows/MSVC
 - Enable sixpair on Windows (by Egor Pugin)
 - Added variables for using with MSVC (by Rafael Lago)
 - Fix for native loading on windows
- macOS
 - Add tracker libraries to package (Fixes #422)


## [4.0.10] - 2020-05-03

- `batterycheck.py`: Add support for "charging" indication (8bcc80d)
- Navigation Controller support and documentation (#403)
- Improve pairing of ZCM2 on Raspberry Pi devices (#401, by @adangert)


## [4.0.9] - 2020-02-24

- PSEye on Linux: Fix auto-exposure setting (Fixes #150, workaround by @peoro)
- Fix crash on missing camera on Linux (by @nitsch)
- Disable building OpenCV with Eigen (by @nitsch)
- Integrate ZCM2 PIN agent into pairing tool (by @nitsch)
- Document Bluetooth limitation on macOS (by @nitsch)


## [4.0.8] - 2019-09-16

This release further improves support for the (MicroUSB/PS4) ZCM2 model of the
PS Move Motion Controller. Minor bug fixes are also included, as is a fix for
recent Windows 10 versions (1903) and the codebase has been upgraded to make
use of OpenCV 3. Thanks to @nitsch, @dquam, @Zangetsu38, @adangert and @rpavlik
for contributing to this release.


## [4.0.7] - 2018-10-234

This release improves support for the PS4 Move CECH-ZCM2J controller, thanks to
Guido Sanchez, Derek Quam and Alexander Nitsch for the updates (#373).


## [4.0.6] - 2018-08-30

- Update `examples/python/ep_whack.py` to be Python 3 compatible (by Maksim Surguy, #347)
- macOS build script: Allow spaces in the installation path (by Tim Arterbury, #348)
- OpenCV: Require OpenCV 2.x in the build scripts for now (by Alexander Nitsch, #359)
- `CMakeLists.txt`: Use `swig_add_library` instead of `swig_add_module` (#365)
- Initial support for the PS4 Move CECH-ZCM2J (in cooperation with Alexander Nitsch, #361, #362, #353)
- Calibration: Update dumped information (by Alexander Nitsch, #366)
- Versioning: Take `PSMOVE_CURRENT_VERSION` from CMake (#360, #364)
- Updated documentation (#351)


## [4.0.5] - 2017-12-30

- Fix building of Java bindings on Windows via CMake (#343)
- Improvements to the Python bindings (#337)
- Add support for specifying the Python version to use for bindings
- Add documentation for building and testing the Python bindings
- Make the Python examples more robust and report missing controllers
- Pin OpenCV version to 2.4, as the 3.x branch isn't as compatible (#339)
- Link against v4l2 on Linux for the tracker (#77)


## [4.0.4] - 2017-09-14

This release adds support for easily pairing with multiple controllers on
Linux, removes support for the old BlueZ 4 version (BlueZ 5 is the only
Bluetooth stack supported on Linux now) and improves runtime detection support
for the Pocket C.H.I.P ARM-based Linux computer.

On the macOS front, we now require the pairing tool to be ran as root user or
with sudo, this avoids having to query the password when the Bluetooth
configuration needs to be updated. For macOS 10.12.6, using more than two
controllers is supported by using an external Bluetooth 4.0 USB dongle.

For Windows, this release adds support for building both 64- and 32-bit
variants of the library and uses a custom version of hidapi that allows for
proper pairing of Bluetooth controllers. Visual Studio 2017 is now also
supported as compiler in addition to VS2013, VS2015 and MinGW (both 32- and
64-bit).

The static libraries are not built anymore, users are recommended to use the
shared library.


## [4.0.1] - 2016-12-27

This release fixes various Bluetooth pairing / configuration issues on Linux
and adds support for the Pocket C.H.I.P portable Linux computer.


## [4.0.0] - 2016-12-26

This release adds the `psmoveapi.h` header for a new high-level, event-based
API to access controllers without having to interface with the low-level
details.  There's also new ctypes-based Python 3 bindings for PS Move API, and
we've cleaned up the build process to easily build and deploy releases in
Travis CI and AppVeyor.


## Older Releases

For older changes, consult the Git History for this project.
