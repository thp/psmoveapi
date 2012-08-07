==========================================
  UNIMOVE - v. 0.1.0 (Beta)
==========================================

by Patrick Jarnfelt and Douglas Wilson (www.doougle.net)
Copenhagen Game Collective (www.cphgc.org)
Project site: http://www.cphgc.org/unimove

Last modified: 2011-07-20


------------------------------------------
  INTRODUCTION
------------------------------------------

UniMove is an open-source Unity (http://unity3d.com) plugin and set of C# bindings 
that allows you to use PlayStation Move controllers with your Unity and/or C# application. 
Currently, the plugin/bindings only works on Mac OSX 10.5 and 10.6.

UniMove is based on top of Thomas Perl's super-nifty C API (http://thp.io/2010/psmove/) 
The plugin is licensed under the GNU Lesser General Public License (v2.1 or later) 
(see COPYING), and is still under development.

The goal, as Thomas Perl put it, is:

    	"... to implement a small and easy to use library to control the main functions 
	of the controller (rumble and RGB LEDs) and to read the sensor values and button 
	states from the controller. The broad availability of the controller, with its USB 
	and Bluetooth connectivity, makes it an ideal candidate for building your own Ambient 
	Orb-style ambient display."

Features include:

	-- Set the RGB LED color and rumble intensity (via USB and Bluetooth)
    	-- Read the status of the digital buttons (via Bluetooth only)
    	-- Read the status of the analog trigger (via Bluetooth only)
    	-- Read sensor values of the accelerometer and gyroscope (via Bluetooth only)

We have already used the plugin to develop of a number of games and prototypes, including 
our upcoming physical party game, Johann Sebastian Joust!: http://www.jsjoust.com


------------------------------------------
  INSTALLATION
------------------------------------------

1. Start a new project, then go to: 
Assets --> Import Package --> Custom Package…

2. Find and select UniMove.package

3. In the "Importing package" window, press the "Import" button in the bottom right

4. Viola! The assets (code, scene, plugin) should now be extracted and visible in your
Project window. Double click the "UniMoveTestScene" scene to load it. 
You should now be able to press the Play button to run the test program!

We've also included the code in a separate directory, just in case.


------------------------------------------
  FUTURE PLANS
------------------------------------------

Shorter-term TODOs:

	-- Provide hooks to the battery level and the temperature sensor 
	   (Thomas Perl is currently working on this in his C API)
	-- MonoDevelop .sln and example program (for those of you who don't have Unity Pro).

Longer-term TODOs:

	-- Provide support for the Navigation Controller and other PS3 input devices
    	-- Graphical utility program that offers developers an easy way to visualize 
	   accelerometer/gyroscope data, play with LED colors, etc.
    	-- Get the plugin/bindings working on Windows

Email us (code@cphgc.org) if you have any code to contribute, or if you want to get involved!

We could probably use some help getting the plugin to work w/ Windows (see below).


------------------------------------------
  FAQ
------------------------------------------

1. Is UniMove available for Windows?
------------------------------------
No, sadly. We've spent a long time trying to get it to work, but we just can't get the 
Bluetooth pairing working on Windows. We can actually get the USB functionality working, 
but that's quite limited. If you think you can help us, shoot us an email (code@cphgc.org).

We had some limited success experimenting with an entirely different API called 
MotioninJoy (http://www.motioninjoy.com), but it seems like it doesn't support multiple 
Move controllers... yet.


2. I can't afford a Unity Pro license! Can I still use UniMove?
---------------------------------------------------------------
Unfortunately, you do need a Unity Pro license in order to use plugins 
(i.e. external .dll and .bundle files).

However, you could use our C# bindings and make your own project using the MonoDevelop IDE. 
It's a little tricky, but definitely possible - we've actually done it ourselves. In the 
near future, we hope to release a .sln and short tutorial. Meanwhile, in our download we've 
included the .dylib file (the OSX equivalent of a .dll) that you'll need to get started. 
See our Unity code for an example of how to reference code from a .dylib/.dll in a C#.

Careful, it get be frustrating to get MonoDevelop to "see" external .dylib files. You may 
need to use a special .config file. For more info, read this very useful and extensive page 
about Mono and interop with native libraries. Also, OSX seems to be picky about how your 
reference the .dylib prefix and suffix in your C# code. Again, do read that reference page 
if you're going to try to use the C# bindings outside of Unity.

Finally, if all else fails, you could always just use Thomas Perl's lower-level API - 
available in C, Qt/C++, and Python.


3. Does UniMove work on OSX 10.4 or earlier?
--------------------------------------------
Actually we don't really know. We haven't been able to test that yet. We do suspect that 
we'd need to recompile the .bundle/.dylib, but we aren't sure.

Get in touch with us if you ever test that!


4. What is a .bundle file?
--------------------------
You'll notice that the plugin comes with a .bundle file. That's basically an OSX version of
a .dll. Usually, on OSX you'd use a .dylib, but Unity only takes .bundle or .dll files.

If you're going to try to use our bindings outside of Unity, you'll want to use the .dylib 
that we also included (see above, Question #2).


5. How do I connect the controllers via Bluetooth? And what is "pairing"?
-------------------------------------------------------------------------
This gets a tiny bit complicated. You'll need to both "pair" and "connect" the Move 
controllers. Each Move controller stores the hardware address of one target device 
(i.e. a computer or PS3). Setting that address - that is, tethering the controller to a 
specific computer - is called "pairing". To do this, you'll need to physically connect the 
controller by mini-USB cable. Once you pair a controller, you won't ever have to pair it 
again - or at least until you overwrite that address by pairing the controller to some 
other device.

We've included our own Pairing Utility (along with the source code). Feel free to package 
it together with your own application. You could also write your own pairing code/utility,
of course. See the Pairing Utility code to see how that's done. Thomas Perl's original C API 
actually provides functions for manually setting/getting the Bluetooth address, but we were 
just able to use his auto-magic psmove_pair() function.

Once a controller is paired, you still need to press the small circular "PS" button to 
finally "connect" it to OSX (you don't need the mini-USB cable for this step). If successful, 
you should see the controller show up in the OSX Bluetooth Preferences panel. Unlike the 
pairing process, you'll need to connect the controllers again every time you restart your 
computer.

Note: if a controller gets unexpectedly disconnected (i.e. it goes out of range), you'll 
sometimes need to manually remove it from the OSX Bluetooth Preferences panel before you can 
re-connect it.


6. How many controllers does UniMove support?
---------------------------------------------
6-7 controllers - apparently there's actually a hardware limit (7?) on the number of Bluetooth 
devices that can be connected at any one time.

However, we should note that we've found that older/slower computer can have trouble keeping 
up. This sluggishness seems to increase proportionally to the number of controllers that are 
connected. We're still not entirely sure why, and we'd be curious to hear if anyone has a 
smart workaround. Our own somewhat kludge-y workaround has been to increase, when needed, the 
frequency of the controllers' update loop. Our API offers a simple property where you can 
adjust this frequency. This is why our API doesn't use Unity's FixedUpdate() function - 
we figured that people might want to adjust the frequency of their controllers' update loop 
independently of the rest of their application.


--------------------------------
  CREDITS & TECHNOLOGY
--------------------------------

UniMove was developed by Patrick Jarnfelt and Douglas Wilson (www.doougle.net)

A very special thanks to Thomas Perl (http://thp.io), who authored the C code on top of which 
this project is based (and who gave us excellent and patient tech support!). Thomas' code, in 
turn, uses Alan Ott's multi-platform hidapi (http://www.signal11.us/oss/hidapi).

And thanks to the rest of the collective, as well as Brandon, Greg, Tiff, Adam, Marek, Nick, 
Derek, and Rune, for testing and feedback!


--------------------------------
  CONTACT
--------------------------------

Email: code@cphgc.org
Twitter: @cphgc / @doougle
Web: www.cphgc.org

For more news, follow us on our blog:
http://www.copenhagengamecollective.org/blog

We also recommend that you join our low-traffic email newsletter:
http://www.copenhagengamecollective.org/newsletter


------------------------------------------
  VERSION HISTORY
------------------------------------------

0.1.0 (Beta) - 2011-07-20
--------------------------
First public release! Give it a whirl and tell us what you think!

