Navigation Controller
=====================

While the "PS Move Navigation Controller" (CECH-ZCS1E) is marketed
together with the Move Controller, in reality it's just a special
variant of the SixAxis / DualShock 3 controller with its own PID.

* Vendor ID: **0x054c** (Sony Corp.)
* Product ID: **0x042f** (PlayStation Move navigation controller)

As such, the Navigation Controller does not need a special library
for reading, processing and interpreting input events. After pairing,
the controller can be used just like a normal gamepad device via
the operating system game controller APIs.


Pairing
-------

The ``psmove`` command-line utility now comes with a new sub-command
``pair-nav``, which will use ``sixpair`` to pair a Navigation Controller
(as well as a Sixaxis, DS3 and DS4) via USB. ``sixpair`` comes from the
`sixad`_ package, but has been modified to read the Bluetooth Host
address from the PS Move API libraries (for cross-platform support)::

    psmove pair-nav

Just like with the normal pairing function, one can pass an alternative
Bluetooth host address to the pairing tool::

    psmove pair-nav aa:bb:cc:dd:ee:ff

.. _`sixad`: https://github.com/RetroPie/sixad


Testing
-------

The easiest way to test the controller is via the ``jstest`` tool,
which comes in the ``joystick`` package on Debian-based systems::

    sudo apt install joystick

Assuming the Navigation Controller is the only joystick-style device
connected, you can test it with::

    jstest /dev/input/js0

There is an example file ``examples/c/test_navcon.cpp`` that shows
how to use SDL2 to read the navigation controller (this is also
built as a program if ``PSMOVE_BUILD_NAVCON_TEST`` is enabled in CMake)::

    test_navcon


Axes and Buttons
----------------

These values are also exposed in ``psmove.h`` as ``enum PSNav_Button``
and ``enum PSNav_Axis`` for your convenience. However, you are
expected to bring your own Joystick-reading library (e.g. SDL2).


 * Analog stick

   * Axis 0 (horizontal, "X")
   * Axis 1 (vertical, "Y")
   * Button 7 when pressed ("L3")

 * Shoulder button ("L1"): Button 4
 * Analog trigger ("L2"):

   * Axis 2 ("Z")
   * Button 5 when pressed (even just slightly)

 * Cross button: Button 0
 * Circle button: Button 1
 * D-Pad Up: Button 8
 * D-Pad Down: Button 9
 * D-Pad Left: Button 10
 * D-Pad Right: Button 11
 * PS button: Button 6


Caveats
-------

On Linux, all inputs work via both USB and Bluetooth. When connecting
via USB, you might need to press the PS button for the controller to
start reporting.

On macOS, inputs only work over Bluetooth, not via USB. Also, the
button assignment is different on macOS (but ``psmove.h`` contains
definitions for both platforms and will pick the right ones). As a
special case, the analog value of the trigger isn't available on
macOS at the moment.

This is not tested at all on Windows, contributions welcome.
