Tracking and Camera
===================

This page contains information about the camera integration, especially
as it relates to tracking and the different variants for different OSes.

PSEYEDriver
-----------

This is the recommended and default way of interfacing with the PS Eye
camera on Windows and macOS. On Linux, the kernel has a built-in driver
for the PS3 Eye camera, which is used via OpenCV/V4L2 by default.

On Windows, you need to install `Zadig`_ (WinUSB driver) for the
"USBCamera-B4.09.24.1" (or similar) device so that the PS3EYEDriver
can access the camera using ``libusb``. This has been tested with the
"WinUSB (v6.1.7600.16385)" driver and Zadig 2.7.

.. _Zadig: https://zadig.akeo.ie/
