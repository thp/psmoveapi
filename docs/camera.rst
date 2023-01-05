Tracking and Camera
===================

This page contains information about the camera integration, especially
as it relates to tracking and the different variants for different OSes.

PS3 Camera
----------

This camera is always supported on Linux, and supported on macOS and Windows
when using the PS3EYEDriver camera control driver.

On Linux, the kernel includes built-in support for the camera using the
``ov534`` driver, so the V4L2 camera control driver also support the PS3 Eye.

The camera needs a USB 2.0 port.

On Windows, you need to install `Zadig`_ (WinUSB driver) for the
"USBCamera-B4.09.24.1" (or similar) device so that the PS3EYEDriver
can access the camera using ``libusb``. This has been tested with the
"WinUSB (v6.1.7600.16385)" driver and Zadig 2.7.

.. _Zadig: https://zadig.akeo.ie/

- Tested models: SLEH-00448
- No firmware upload necessary

Supported resolutions:

- **640x480 @ 60 Hz** (default)
- 320x240 @ 187 Hz


PS4 Camera
----------

This camera is currently supported on Linux when using the V4L2 camera control
driver, and macOS when using the AVFoundation camera control driver.

You need to upload a firmware binary blob using the ``psmove camera-firmware`` command.

The camera needs a USB 3.0 port.

If you have a PSVR kit, you can order the `Camera Adaptor`_ (to convert
PS4 AUX to USB 3.0) from Sony for free. You only need to enter the serial
number of your PSVR kit during the ordering process, no need for a PS5.
If you only have the camera but no PSVR, you can look to buy the
adaptor online. The official Sony one is CFI-ZAA1.

.. _Camera Adaptor: https://camera-adaptor.support.playstation.com/

The official PSVR camera adaptor for PS5 (CFI-ZAA1) will be detected
automatically. If you are using a homemade PS4 AUX-to-USB adapter, use
``psmove camera-firmware --force-ps4`` to detect any OV580 in boot mode
as PS4 (otherwise it's assumed that the camera is a PS5 camera, as there
is no way yet to distinguish the PS4 and PS5 cameras from the USB
descriptor in boot mode only, which is probably why the CFI-ZAA1 appears
as USB parent device during enumeration).

After the firmware upload the camera is supported using
the ``uvcvideo`` kernel driver in OpenCV/V4L2 (Linux)
and ``UVCService`` on OpenCV/AVFoundation (macOS).

- Tested models: CUH-ZEY2 ("round model v2")
- Tested firmware SHA-1: ``fe86162309518a0ffe267075a2fcf728c5856b3e``
- Firmware search path: ``/etc/psmoveapi/camera-firmware-ps4.bin``, ``$HOME/.psmoveapi/camera-firmware-ps4.bin``

Supported resolutions:

- **1280x800 @ 60 Hz** (default)
- 640x400 @ 120 Hz
- 320x192 @ 240 Hz


PS5 Camera
----------

This camera is currently supported on Linux when using the V4L2 camera control
driver, and macOS when using the AVFoundation camera control driver.

You need to upload a firmware binary blob using the ``psmove camera-firmware`` command.

The camera needs a USB 3.0 port.

After the firmware upload the camera is supported using
the ``uvcvideo`` kernel driver in OpenCV/V4L2 (Linux)
and ``UVCService`` on OpenCV/AVFoundation (macOS).

- Tested models: CFI-ZEY1
- Tested firmware SHA-1: ``0fa4da31a12b662a9a80abc8b84932770df8f7e1``
- Firmware search path: ``/etc/psmoveapi/camera-firmware-ps5.bin``, ``$HOME/.psmoveapi/camera-firmware-ps5.bin``

Supported resolutions:

- 1920x1080 @ 30 Hz
- **1280x800 @ 60 Hz** (default)
- 960x520 @ 60 Hz
- 640x376 @ 120 Hz
- 448x256 @ 120 Hz
- 320x184 @ 240 Hz
