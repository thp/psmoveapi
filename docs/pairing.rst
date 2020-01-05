Pairing the Controller to your PC
=================================

The PS Move connects via two methods:

* **USB**: Used for Bluetooth pairing; can set rumble and LEDs, cannot read sensors
* **Bluetooth**: Used for wireless connectivity; can set rumble, LEDs and read sensors


Bluetooth pairing
-----------------

The ``psmove pair`` utility is used for Bluetooth pairing -- it will write the
Bluetooth host address of the computer it's running on to PS Move controllers
connected via USB.

It optionally takes a single command-line parameter that is an alternative
Bluetooth host address. For example, if you want to pair your PS Move controller
to your phone, but it does not have USB Host Mode, you can use this on your PC::

    ./psmove pair aa:bb:cc:dd:ee:ff

Where ``aa:bb:cc:dd:ee:ff`` is the Bluetooth host address of your phone. Note
that depending on the phone, you also need to run pairing code there.

Depending on the operating system, you might need to run the utility as
Administrator (Windows), enter your password (OS X) or run using ``sudo``
(Linux) to let the utility modify the system Bluetooth settings and whitelist
the PS Move for connection.

.. note::
   You only need to pair the controller to your PC once, from then on
   it will always try to connect to your PC. Only when you connect your
   controller to a PS3 or pair with another PC will you have to re-do
   the pairing process on your computer.


Connecting via Bluetooth
------------------------

Unplug the USB cable and press the PS button on the controller. The red status
LED will start blinking and should eventually remain lit to indicate a working
Bluetooth connection. If it continues blinking, it might not be paired via
Bluetooth, or - if you can see the connection on your computer - the battery
is low and needs charging via USB or a charger. To verify the connection,
check the Bluetooth devices list of your computer and see if there is an
entry for "Motion Controller".

On recent versions of OS X, a dialog might pop up asking for a PIN. Close it
and pair the controller again using ``psmove pair``. After that, it should
connect successfully.


Troubleshooting
---------------

Here are some advanced tips if you can't get pairing working out of the box:

Mac OS X
~~~~~~~~

If ``psmove pair`` doesn't work or you get a PIN prompt when you press the PS
button on your controller, follow these steps:

* Right after you run ``psmove pair`` write down the adress you find after
  "controller address:" in the form "aa:bb:cc:dd:ee:ff"
* Disable Bluetooth (or the modifications that follow won’t work)
* Wait for the Bluetooth process to quit (``pgrep blued``); repeat until nothing
  prints anymore (e.g. the process "blued" has quit) - this can take up to a minute
* Authorize the controller’s MAC address:
  ``sudo defaults write /Library/Preferences/com.apple.Bluetooth HIDDevices -array-add "<aa-bb-cc-dd-ee-ff>"``
  (where <aa-bb-cc-dd-ee-ff> is the MAC address you wrote down at 8.2 but with hyphens for separators)
* Enable Bluetooth again then press the PS button on the controller. The PIN request should
  not pop up this time and the Move should now appear in the bluetooth devices as "Motion Controller".

Starting with macOS Sierra, at most 2 Move controllers can be connected via Bluetooth at the same time when using the built-in Bluetooth adapter. This is due to some internal changes Apple made and did work on the same hardware prior to updating to Sierra. If you need more controllers at the same time, you can use an external Bluetooth adapter.


Ubuntu Linux
~~~~~~~~~~~~

If you wish to access the PSMove controller via USB or Bluetooth without
requiring root-level permissions then it is necessary to copy the
``contrib/99-psmove.rules`` file to ``/etc/udev/rules.d/``::

   sudo cp contrib/99-psmove.rules /etc/udev/rules.d/
   sudo udevadm trigger


Windows
~~~~~~~

You might have to try pairing multiple times for the Bluetooth connection to
work on Windows. Also, be sure to use the Microsoft Bluetooth Stack and do
not install any third party drivers (e.g. MotionInJoy) that would interfere
with proper operation of PS Move API on Windows.


Pocket C.H.I.P
~~~~~~~~~~~~~~

During testing, we found that pairing a PS Move via USB works fine in a Pocket
C.H.I.P with its battery. On a C.H.I.P (the board, without the battery, just
connected via a MicroUSB charger) we noticed that sometimes the PS Move seems
to draw too much power from USB, causing the C.H.I.P to reboot. In this case,
you can pair your controller with the C.H.I.P in the Pocket C.H.I.P shell and
then remove the C.H.I.P and just power it up and connect via Bluetooth only.

If you do not have a Pocket C.H.I.P, and just a C.H.I.P, you can pair the
controller on your workstation (Linux, macOS, Windows) and then register the
controller using ``psmove register``.
