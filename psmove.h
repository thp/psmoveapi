
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2011, 2012 Thomas Perl <m@thp.io>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 **/

#ifndef __PSMOVE_H
#define __PSMOVE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  define ADDCALL __cdecl
#  ifdef BUILDING_SHARED_LIBRARY
#    define ADDAPI __declspec(dllexport)
#  else
#    define ADDAPI __declspec(dllimport)
#  endif
#else
#  define ADDAPI
#  define ADDCALL
#endif

enum PSMove_Connection_Type {
    Conn_Bluetooth,
    Conn_USB,
    Conn_Unknown,
};

enum PSMove_Button {
    Btn_L2 = 1 << 0x00,
    Btn_R2 = 1 << 0x01,
    Btn_L1 = 1 << 0x02,
    Btn_R1 = 1 << 0x03,
    Btn_TRIANGLE = 1 << 0x04,
    Btn_CIRCLE = 1 << 0x05,
    Btn_CROSS = 1 << 0x06,
    Btn_SQUARE = 1 << 0x07,
    Btn_SELECT = 1 << 0x08,
    Btn_L3 = 1 << 0x09,
    Btn_R3 = 1 << 0x0A,
    Btn_START = 1 << 0x0B,
    Btn_UP = 1 << 0x0C,
    Btn_RIGHT = 1 << 0x0D,
    Btn_DOWN = 1 << 0x0E,
    Btn_LEFT = 1 << 0x0F,
    Btn_PS = 1 << 0x10,

    Btn_MOVE = 1 << 0x13,
    Btn_T = 1 << 0x14,
};

enum PSMove_Battery_Level {
    Batt_MIN = 0x00,
    Batt_MAX = 0x05,
    Batt_CHARGING = 0xEE,
};

/* A Bluetooth address. */
typedef unsigned char PSMove_Data_BTAddr[6];

/* Opaque data type for the PS Move internal data */
struct _PSMove;
typedef struct _PSMove PSMove;

/**
 * Disable the connection to remote servers
 *
 * This can only be called at the beginning, and will disable connections to
 * any remote "moved" servers.
 **/
ADDAPI void
ADDCALL psmove_disable_remote();

/**
 * Disable local (hidapi-based) controllers
 *
 * This can only be called at the beginning, and will disable all local
 * controllers that are connected via hidapi.
 **/
ADDAPI void
ADDCALL psmove_disable_local();


/**
 * [PRIVATE API] Write raw data blob to device
 **/
ADDAPI void
ADDCALL _psmove_write_data(PSMove *move, unsigned char *data, int length);


/**
 * [PRIVATE API] Read raw data blob from device
 **/
ADDAPI void
ADDCALL _psmove_read_data(PSMove *move, unsigned char *data, int length);


/**
 * Reinitialize the library. Required for detecting new and removed
 * controllers (at least on Mac OS X). Make sure to disconnect all
 * controllers (using psmove_disconnect) before calling this!
 **/
ADDAPI void
ADDCALL psmove_reinit();

/**
 * Get the number of currently-connected PS Move controllers
 **/
ADDAPI int
ADDCALL psmove_count_connected();

/**
 * Connect to the PS Move controller id (zero-based index)
 * Returns: A newly-allocated PSMove structure or NULL on error
 **/
ADDAPI PSMove *
ADDCALL psmove_connect_by_id(int id);

/**
 * Connect to the default PS Move controller
 * Returns: A newly-allocated PSMove structure or NULL on error
 **/
ADDAPI PSMove *
ADDCALL psmove_connect();

/**
 * Determine the connection type of the controllerj
 * Returns: An enum PSMove_Connection_Type value
 **/
ADDAPI enum PSMove_Connection_Type
ADDCALL psmove_connection_type(PSMove *move);

/**
 * Read a Bluetooth address from string and write its
 * internal representation into a PSMove_Data_BTAddr.
 *
 * If dest is NULL, the data is not written (only verified).
 *
 * Will return nonzero on success, zero on error.
 **/
ADDAPI int
ADDCALL psmove_btaddr_from_string(const char *string, PSMove_Data_BTAddr *dest);

/**
 * Get the currently-set Host Bluetooth address that is used
 * to connect via Bluetooth when the PS button is pressed.
 *
 * addr might be NULL in which case the address and calibration
 * data are retrieved, but the Bluetooth address is discarded.
 **/
ADDAPI int
ADDCALL psmove_get_btaddr(PSMove *move, PSMove_Data_BTAddr *addr);

/**
 * Get the Bluetooth Mac address of the connected controller.
 *
 * XXX This is not implemented in the backend at the moment.
 **/
ADDAPI int
ADDCALL psmove_controller_btaddr(PSMove *move, PSMove_Data_BTAddr *addr);

/**
 * Get the serial number of the controller.
 *
 * On Mac OS X for Bluetooth controllers, this is the Bluetooth Mac address of
 * the controller in string format, e.g. "aa-bb-cc-dd-ee-ff"
 *
 * XXX What is the serial number on OS X via USB?
 * XXX What is the serial number on other platforms?
 **/
ADDAPI const char*
ADDCALL psmove_get_serial(PSMove *move);

/**
 * Check if the controller handle is a remote (moved) connection.
 *
 * Return 0 if "move" is local (USB/Bluetooth), 1 if it's remote (moved)
 **/
ADDAPI int
ADDCALL psmove_is_remote(PSMove *move);

/**
 * Set the Host Bluetooth address that is used to connect via
 * Bluetooth. You should set this to the local computer's
 * Bluetooth address when connected via USB, then disconnect
 * and press the PS button to connect the controller via BT.
 **/
ADDAPI int
ADDCALL psmove_set_btaddr(PSMove *move, PSMove_Data_BTAddr *addr);

/**
 * Set the Host Bluetooth address of the PS Move to this
 * computer's Bluetooth address. Only works via USB.
 *
 * Implemented for Linux (Bluez), Mac OS X and the default
 * Windows 7 Microsoft Bluetooth stack only.
 *
 * Windows note: Doesn't work with 3rd party stacks like Bluesoleil.
 * In this case, you can use psmove_pair_custom() (see below).
 *
 * Will return nonzero on success, zero on error.
 **/
ADDAPI int
ADDCALL psmove_pair(PSMove *move);

/**
 * Set the Host Bluetooth address of the PS Move to the
 * Bluetooth address given by btaddr_string (which should
 * contain a string in the format "AA:BB:CC:DD:EE:FF").
 *
 * Will return nonzero on success, zero on error.
 **/
ADDAPI int
ADDCALL psmove_pair_custom(PSMove *move, const char *btaddr_string);

/**
 * Set the LEDs of the PS Move controller. You need to
 * call PSMove_update_leds() to send the update to the
 * controller.
 **/
ADDAPI void
ADDCALL psmove_set_leds(PSMove *move, unsigned char r, unsigned char g,
        unsigned char b);

/**
 * Set the rumble value of the PS Move controller. You
 * need to call PSMove_update_leds() to send the update
 * to the controller.
 **/
ADDAPI void
ADDCALL psmove_set_rumble(PSMove *move, unsigned char rumble);

/**
 * Re-send the LED and Rumble status bits. This needs to
 * be done regularly to keep the LEDs and rumble turned on.
 *
 * Returns: Nonzero on success, zero on error
 **/
ADDAPI int
ADDCALL psmove_update_leds(PSMove *move);

/**
 * Polls the PS Move for new sensor/button data.
 * Returns a positive number (sequence number + 1) if new data is
 * available or zero if no data is available.
 **/
ADDAPI int
ADDCALL psmove_poll(PSMove *move);

/**
 * Get the current status of the PS Move buttons. You need to call
 * PSMove_poll() to read new data from the controller first.
 **/
ADDAPI unsigned int
ADDCALL psmove_get_buttons(PSMove *move);

/**
 * Get the battery level of the PS Move. You need to call
 * PSMove_poll() to read new data from the controller first.
 *
 * Return value range: Batt_MIN..Batt_MAX
 * Charging (via USB): Batt_CHARGING
 **/
ADDAPI unsigned char
ADDCALL psmove_get_battery(PSMove *move);

/**
 * Get the current temperature of the PS Move. You need to
 * call PSMove_poll() to read new data from the controller first.
 *
 * Return value range: FIXME
 **/
ADDAPI int
ADDCALL psmove_get_temperature(PSMove *move);

/**
 * Get the current value of the PS Move analog trigger. You need to
 * call PSMove_poll() to read new data from the controller first.
 **/
ADDAPI unsigned char
ADDCALL psmove_get_trigger(PSMove *move);

/**
 * Get the current accelerometer readings from the PS Move. You need
 * to call PSMove_poll() to read new data from the controller first.
 *
 * ax, ay and az should be pointers to integer locations that you want
 * to have filled with values. If you don't care about one of these
 * values, simply pass NULL and the field will be ignored..
 **/
ADDAPI void
ADDCALL psmove_get_accelerometer(PSMove *move, int *ax, int *ay, int *az);

/**
 * Same as PSMove_get_accelerometer(), but for the gyroscope.
 **/
ADDAPI void
ADDCALL psmove_get_gyroscope(PSMove *move, int *gx, int *gy, int *gz);

/**
 * Same as PSMove_get_accelerometer(), but for the magnetometer.
 * The result value range is -2048..+2047. The magnetometer is located
 * roughly below the glowing orb - you can glitch the values with a
 * strong kitchen magnet by moving it around the bottom ring of the orb.
 *
 * You can detect if a magnet is nearby by checking if any two values
 * stay at zero for several frames.
 **/
ADDAPI void
ADDCALL psmove_get_magnetometer(PSMove *move, int *mx, int *my, int *mz);

/**
 * Disconnect from the PS Move and free resources
 **/
ADDAPI void
ADDCALL psmove_disconnect(PSMove *move);

#ifdef __cplusplus
}
#endif

#endif

