This directory contains some patches that are needed to build PSMoveAPI for Windows using Visual Studio:

#### libusb_dynamic_crt.patch
The libusb project links against the static CRT by default. PSMoveAPI links against the dynamic CRT.
Mixing these two together in the same program usually leads to problems, so this patch patches libusb to also link against the dynamic CRT.

#### sdl_vs2015_libs
The version of SDL2 that we're using does not currently work with VS2015.
There is a newer version of SDL2 available that does, but the way they've chosen to fix the VS2015 issues (/NODEFAULTLIB and other shenanigans) 
usually leads to more problems down the line.

This patch fixes SDL2 in the correct way for VS2015 (ie. just adding some extra libs to link against). Once SDL2 mainline has been properly fixed, we can remove this patch.