============================================================
  PLAYSTATION MOVE BLUETOOTH PAIRING UTILITY - v. 0.3 (Beta)
============================================================

by Douglas Wilson (www.doougle.net)
Copenhagen Game Collective (www.cphgc.org)
Project site: http://www.cphgc.org/unimove

Last modified: 2011-07-20
	

------------------------------------------
  INTRODUCTION
------------------------------------------

This Pairing Utility is an open-source C# application that pairs
PlayStation Move controllers by Bluetooth to your computer.

It was built using Unity (http://unity3d.com) and is licensed under 
the GNU Lesser General Public License (v2.1 or later) (see COPYING). 
It is still under development.

For more information, see the UniMove README.


--------------------------------
  SYSTEM REQUIREMENTS
--------------------------------

The Pairing Utility runs only on OSX 10.5 and 10.6 (it has not 
been tested on OSX 10.4 or earlier, but I'd be curious to hear if 
that actually works).

The Utility requires a Bluetooth connection, and requires one or
more PlayStation Move controllers. You'll also need a mini-USB
wire for the initial connection (once the controller is 
paired, you'll be able to unplug it and use it wirelessly).


--------------------------------
  INSTALLATION & HOW TO USE
--------------------------------

There is no "installation" per se. Just double click the 
"Pairing Utility" application executable. 

Follow the directions in the application.

The process of "pairing" writes the Bluetooth address of your 
computer (or console) into the controller's memory. This means 
that you won't have to re-pair the controller, even if you restart 
your computer. HOWEVER, if you pair the controller to a new computer 
or to a Playstation 3, you will have to re-pair. 

And remember to turn on Bluetooth on your computer BEFORE 
starting the application!


--------------------------------
  SOURCE CODE
--------------------------------

The Unity files and source code have been included, but without
the music, sound effects, or fonts used in our release executable
(for permissions reasons).

To get the source code running, you'll need a copy of Unity Pro
(only the Pro version lets you use external libraries).

1. Start a new project, then go to: 
Assets --> Import Package --> Custom Package…

2. Find and select PairingUtility.package

3. In the "Importing package" window, press the "Import" button
in the bottom right

4. Viola! The assets (code, scene, plugin) should now be extracted and visible 
in your Project window. Double click the "Pairing Utility" scene to load it. 
You should now be able to press the Play button to run the utility.

We've also included the code in a separate directory, just in case.

For more information on using C# bindings outside of Unity,
see the UniMove README.


--------------------------------
  CREDITS & TECHNOLOGY
--------------------------------

Design and Programming by Douglas Wilson (www.doougle.net)

Sound and music by and Nicklas "Nifflas" Nygren (nifflas.ni2.se)
The tune comes from the B.U.T.T.O.N. soundtrack:
http://sites.fastspring.com/copenhagengameproductions/product/buttonep

The Pairing Utility was developed in C# and Unity Pro (unity3d.com)

The game is built on top of an open source C API by Thomas Perl: 
http://thp.io/2010/psmove/

Thanks to Patrick Jarnfelt and Thomas Perl for tech help!

For more credits, see the UniMove README.


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


--------------------------------
  VERSION HISTORY
--------------------------------

 0.1.0 (alpha) - 2011-05-30
----------------------------
Pre-alpha debut. Thanks to Brandon Boyer for testing!


 0.2.0 (alpha) - 2011-06-01
----------------------------
-- Re-wrote the directions
-- The application now loops, so you can pair multiple times 
without having to restart


 0.2.1 (alpha) - 2011-06-14
----------------------------
-- Finally built from proper version of Unity Pro


 0.3.0 (beta) - 2011-07-20
----------------------------
-- Public release along w/ our UniMove plugin!
-- Upgraded from alpha to beta status
-- Re-organized code to make it (somewhat) cleaner
