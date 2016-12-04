This directory contains some patches that are needed to build PSMoveAPI for Windows using Visual Studio:

#### libusb_dynamic_crt.patch
The libusb project links against the static CRT by default. PSMoveAPI links against the dynamic CRT.
Mixing these two together in the same program usually leads to problems, so this patch patches libusb to also link against the dynamic CRT.
