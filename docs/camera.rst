Tracking and Camera
===================

This page contains information about the camera integration, especially
as it relates to tracking and the different variants for different OSes.

PSEYEDriver
-----------

This is the recommended way of interfacing with the PS Eye camera on
Windows and OS X.


CL Eye Driver
-------------

For some situations, on Windows this might give better performance than
the PS Eye Driver. You just install the CL Eye Driver normally and then
use the camera via OpenCV.


iSight exposure locking
-----------------------

On Mac OS X, we have problems getting the PS Eye to work reliably with OpenCV.
Because of that, it makes sense to use other cameras like the iSight camera
built into most (all?) MacBook (Pro) computers and iMacs. Unfortunately, there
is no API for controlling the exposure directly. It is possible to lock the
exposure to a given value so that the exposure isn't continously corrected.

How it works normally (continuous autoexposure):

 1. The camera device determines a good first exposure when started
 2. While frames are captured, the exposure is continuously modified
    (e.g. when you hold the controller near the iSight, the exposure is
    automatically lowered to accommodate the brighter camera image)

How it works with locked exposure:

 1. The camera device determines a good first exposure when started
 2. The exposure set after opening the device will stay the same

So what we need to do is the following:

 1. Light up the controller to a very bright color
 2. Move the controller directly in front of the camera (the sphere
    can actually touch the iSight camera and "block its sight")
 3. Open the camera device (the first exposure is determined here)
 4. As soon as the camera device is opened (and the exposure set),
    lock the exposure so that it doesn't change anymore
 5. Move the controller away from the camera, proceed with normal
    initialization procedure (blinking calibration, etc..)

The call to locking has been integrated into the tracker library, the
exposure will be automatically locked when the tracker library is started.

The exposure seems to be locked until the next reboot in my experiments,
but the exposure is determined every time the camera device is opened (but
will stay locked after opening the device).

Because of this, the calibration procedure on Mac OS X is a bit different
to other platforms, because we have to light up the controller, put it in
front of the iSight, open the camera device, wait for a good exposure, and
only then continue with normal calibration. This has been implemented in
``examples/c/test_tracker.c`` - the console output will tell you what to do:

 1. Start the application
 2. Move the controller that lights up close to the iSight
 3. Press the Move button on this controller
 4. Keep the controller close to the iSight while the camera is opened
 5. Move the controller away from the iSight and press the Move button

Thanks to Raphaël de Courville who had the initial idea for the exposure
locking, did some experimentation with it and provided an first draft of the
implementation that is now integrated in simplified form in the library.
