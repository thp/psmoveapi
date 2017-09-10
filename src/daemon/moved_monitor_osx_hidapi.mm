/**
 * These functions for querying information from IOHIDDeviceRef
 * have been copied from HIDAPI's macOS implementation.
 **/

/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.

 Alan Ott
 Signal 11 Software

 2010-07-03

 Copyright 2010, All Rights Reserved.

 At the discretion of the user of this library,
 this software may be licensed under the terms of the
 GNU General Public License v3, a BSD-Style license, or the
 original HIDAPI license as outlined in the LICENSE.txt,
 LICENSE-gpl3.txt, LICENSE-bsd.txt, and LICENSE-orig.txt
 files located at the root of the source distribution.
 These files may also be found in the public source
 code repository located at:
        http://github.com/signal11/hidapi .
********************************************************/

/* See Apple Technical Note TN2187 for details on IOHidManager. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#import <IOKit/hid/IOHIDManager.h>
#import <CoreFoundation/CoreFoundation.h>

static int32_t get_int_property(IOHIDDeviceRef device, CFStringRef key)
{
	CFTypeRef ref;
	int32_t value;

	ref = IOHIDDeviceGetProperty(device, key);
	if (ref) {
		if (CFGetTypeID(ref) == CFNumberGetTypeID()) {
			CFNumberGetValue((CFNumberRef) ref, kCFNumberSInt32Type, &value);
			return value;
		}
	}
	return 0;
}

static unsigned short get_vendor_id(IOHIDDeviceRef device)
{
	return get_int_property(device, CFSTR(kIOHIDVendorIDKey));
}

static unsigned short get_product_id(IOHIDDeviceRef device)
{
	return get_int_property(device, CFSTR(kIOHIDProductIDKey));
}

static int32_t get_location_id(IOHIDDeviceRef device)
{
	return get_int_property(device, CFSTR(kIOHIDLocationIDKey));
}

static int get_string_property(IOHIDDeviceRef device, CFStringRef prop, wchar_t *buf, size_t len)
{
	CFStringRef str;

	if (!len)
		return 0;

	str = (CFStringRef)IOHIDDeviceGetProperty(device, prop);

	buf[0] = 0;

	if (str) {
		CFIndex str_len = CFStringGetLength(str);
		CFRange range;
		CFIndex used_buf_len;
		CFIndex chars_copied;

		len --;

		range.location = 0;
		range.length = ((size_t)str_len > len)? len: (size_t)str_len;
		chars_copied = CFStringGetBytes(str,
			range,
			kCFStringEncodingUTF32LE,
			(char)'?',
			FALSE,
			(UInt8*)buf,
			len * sizeof(wchar_t),
			&used_buf_len);

		if (chars_copied == len)
			buf[len] = 0; /* len is decremented above */
		else
			buf[chars_copied] = 0;

		return 0;
	}
	else
		return -1;

}

static int get_string_property_utf8(IOHIDDeviceRef device, CFStringRef prop, char *buf, size_t len)
{
	CFStringRef str;
	if (!len)
		return 0;

	str = (CFStringRef)IOHIDDeviceGetProperty(device, prop);

	buf[0] = 0;

	if (str) {
		len--;

		CFIndex str_len = CFStringGetLength(str);
		CFRange range;
		range.location = 0;
		range.length = str_len;
		CFIndex used_buf_len;
		CFIndex chars_copied;
		chars_copied = CFStringGetBytes(str,
			range,
			kCFStringEncodingUTF8,
			(char)'?',
			FALSE,
			(UInt8*)buf,
			len,
			&used_buf_len);

		if (used_buf_len == len)
			buf[len] = 0; /* len is decremented above */
		else
			buf[used_buf_len] = 0;

		return used_buf_len;
	}
	else
		return 0;
}


static int get_serial_number(IOHIDDeviceRef device, wchar_t *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDSerialNumberKey), buf, len);
}


static int make_path(IOHIDDeviceRef device, char *buf, size_t len)
{
	int res;
	unsigned short vid, pid;
	char transport[32];
	int32_t location;

	buf[0] = '\0';

	res = get_string_property_utf8(
		device, CFSTR(kIOHIDTransportKey),
		transport, sizeof(transport));

	if (!res)
		return -1;

	location = get_location_id(device);
	vid = get_vendor_id(device);
	pid = get_product_id(device);

	res = snprintf(buf, len, "%s_%04hx_%04hx_%x",
                       transport, vid, pid, location);


	buf[len-1] = '\0';
	return res+1;
}
