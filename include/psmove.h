
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

#include "psmove_config.h"

#include <stdlib.h>

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

enum PSMove_Sensor {
    Sensor_Accelerometer = 0,
    Sensor_Gyroscope,
};

enum PSMove_Frame {
    Frame_FirstHalf = 0,
    Frame_SecondHalf,
};

enum PSMove_Battery_Level {
    Batt_MIN = 0x00,
    Batt_MAX = 0x05,
    Batt_CHARGING = 0xEE,
};

/* Return values for psmove_update_leds */
#define PSMOVE_UPDATE_FAILED 0
#define PSMOVE_UPDATE_SUCCESS 1
#define PSMOVE_UPDATE_IGNORED 2

/* A Bluetooth address. */
typedef unsigned char PSMove_Data_BTAddr[6];

/* Opaque data type for the PS Move internal data */
struct _PSMove;
typedef struct _PSMove PSMove;

struct _PSMoveCalibration;
typedef struct _PSMoveCalibration PSMoveCalibration;

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
 * Formats the contents of addr to a newly-allocated string and
 * returns it. The caller has to free() the return value.
 **/
ADDAPI char *
ADDCALL psmove_btaddr_to_string(const PSMove_Data_BTAddr addr);

/**
 * Read the current Bluetooth addresses stored in the controller
 *
 * This only works via USB.
 *
 * If host is not NULL, the current host address will be stored there.
 * If controller is not NULL, the controller address will be stored there.
 **/
ADDAPI int
ADDCALL psmove_read_btaddrs(PSMove *move, PSMove_Data_BTAddr *host, PSMove_Data_BTAddr *controller);

/**
 * Get the currently-set Host Bluetooth address that is used
 * to connect via Bluetooth when the PS button is pressed.
 *
 * DEPRECATED - use psmove_read_btaddrs(move, addr, NULL) instead
 **/
ADDAPI int
ADDCALL psmove_get_btaddr(PSMove *move, PSMove_Data_BTAddr *addr);

/**
 * Get the Bluetooth Mac address of the connected controller.
 *
 * DEPRECATED - use psmove_read_btaddrs(move, NULL, addr) instead
 **/
ADDAPI int
ADDCALL psmove_controller_btaddr(PSMove *move, PSMove_Data_BTAddr *addr);

/**
 * Get the calibration data from a connected USB controller.
 *
 * The pointer *dest will be set to a newly-allocated byte array
 * of a certain size (which will be saved in *size) and the caller
 * has to free this field with free()
 **/
ADDAPI int
ADDCALL psmove_get_calibration_blob(PSMove *move, char **dest, size_t *size);

/**
 * Get the serial number of the controller.
 *
 * This is only defined for Bluetooth controllers, and contains
 * the Bluetooth Mac address of the controller as a string, e.g.
 * "aa:bb:cc:dd:ee:ff".
 *
 * The serial number is owned by the move handle - the caller MUST NOT
 * free the returned string. It will be freed on psmove_disconnect().
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
 * Return values:
 *   PSMOVE_UPDATE_SUCCESS ........ success
 *   PSMOVE_UPDATE_IGNORED ........ ignored (LEDs/rumble unchanged)
 *   PSMOVE_UPDATE_FAILED (= 0) ... error
 **/
ADDAPI int
ADDCALL psmove_update_leds(PSMove *move);

/**
 * Enable or disable LED update rate limiting
 *
 * If enabled is 1, then psmove_update_leds will ignore extraneous updates
 * if the update rate is too high. If enabled is 0, all LED updates will be
 * sent (when the LED or rumble value has changed), which might worsen the
 * performance of reading sensor values, especially on Linux.
 **/
ADDAPI void
ADDCALL psmove_set_rate_limiting(PSMove *move, unsigned char enabled);

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
 * Get new button events since the last call to this fuction.
 *
 * move ...... A valid PSMove * instance
 * pressed ... Pointer for storing a bitfield of new press events (or NULL)
 * released .. Pointer for storing a bitfield of new release events (or NULL)
 *
 * Must be called after PSMove_poll() to get the latest states. It should
 * only be called at one location in your application. This is a shortcut
 * for storing and comparing the result of psmove_get_buttons() manually.
 **/
ADDAPI void
ADDCALL psmove_get_button_events(PSMove *move, unsigned int *pressed,
        unsigned int *released);

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
 * Get a half-frame from the accelerometer or gyroscope from the
 * PS Move after using psmove_poll() previously.
 *
 * sensor must be Sensor_Accelerometer or Sensor_Accelerometer.
 *
 * frame must be Frame_FirstHalf or Frame_SecondHalf.
 *
 * x, y and z can point to integer locations that will be filled
 * with the readings. If any are NULL, the fields will be ignored.
 *
 * DEPRECATED - Use psmove_get_accelerometer_frame() or
 * psmove_get_gyroscope_frame() instead - these functions return the
 * half-frame in calibrated form, which is usually what you want when
 * using the half-frames.
 **/
ADDAPI void
ADDCALL psmove_get_half_frame(PSMove *move, enum PSMove_Sensor sensor,
        enum PSMove_Frame frame, int *x, int *y, int *z);

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
 * Get the calibrated accelerometer values (in g)
 *
 * move ......... a valid PSMove * instance
 * frame ........ Frame_FirstHalf or Frame_SecondHalf
 * ax, ay, az ... pointers for the result value (or NULL to ignore)
 *
 * You need to call psmove_poll() first to read the data from the controller.
 **/
ADDAPI void
ADDCALL psmove_get_accelerometer_frame(PSMove *move, enum PSMove_Frame frame,
        float *ax, float *ay, float *az);

/**
 * Get the calibrated gyroscope values (in rad/s)
 *
 * move ......... a valid PSMove * instance
 * frame ........ Frame_FirstHalf or Frame_SecondHalf
 * ax, ay, az ... pointers for the result value (or NULL to ignore)
 *
 * You need to call psmove_poll() first to read the data from the controller.
 **/
ADDAPI void
ADDCALL psmove_get_gyroscope_frame(PSMove *move, enum PSMove_Frame frame,
        float *gx, float *gy, float *gz);

/**
 * Check if the move controller has support for calibration
 *
 * move ... a valid PSMove * instance
 *
 * Returns nonzero if calibration is supported, zero otherwise.
 **/
ADDAPI int
ADDCALL psmove_has_calibration(PSMove *move);

/**
 * Dump the calibration information to stdout (for debugging)
 *
 * move ... a valid PSMove * instance
 **/
ADDAPI void
ADDCALL psmove_dump_calibration(PSMove *move);

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
 * Enable or disable orientation tracking
 *
 * move ...... a valid PSMove * instance
 * enabled ... 1 to enable orientation tracking, 0 to disable
 **/
ADDAPI void
ADDCALL psmove_enable_orientation(PSMove *move, int enabled);

/**
 * Check of orientation tracking is enabled and available
 *
 * move ... a valid PSMove * instance
 *
 * Returns nonzero if orientation tracking is enabled, zero otherwise
 **/
ADDAPI int
ADDCALL psmove_has_orientation(PSMove *move);

/**
 * Get the current orientation as quaternion
 *
 * move ............. a valid PSMove * instance
 * q0, q1, q2, q3 ... pointers to store the quaternion result
 *
 * Make sure to call psmove_poll() regularly. This only works if
 * orientation tracking has been enabled previously (using the function
 * psmove_enable_orientation()).
 **/
ADDAPI void
ADDCALL psmove_get_orientation(PSMove *move,
        float *q0, float *q1, float *q2, float *q3);

/**
 * (Re-)Set the current orientation quaternion
 *
 * move ............. a valid PSMove * instance
 * q0, q1, q2, q3 ... the quaternion to (re-)set the orientation to
 **/
ADDAPI void
ADDCALL psmove_set_orientation(PSMove *move,
        float q0, float q1, float q2, float q3);


/**
 * Disconnect from the PS Move and free resources
 **/
ADDAPI void
ADDCALL psmove_disconnect(PSMove *move);

/**
 * Utility function: Get milliseconds since first library use
 **/
ADDAPI long
ADDCALL psmove_util_get_ticks();

/**
 * Utility function: Get local save directory for settings
 *
 * The returned value is reserved in static memory. It must not be free()d.
 **/
ADDAPI const char *
ADDCALL psmove_util_get_data_dir();

/**
 * Utility function: Get filename in PS Move data directory
 *
 * The data directory will be created in case it doesn't exist yet.
 * Returns NULL if the data directory cannot be created.
 *
 * filename ... The basename of the file inside the data dir
 *
 * The returned value must be free()d after use.
 **/
ADDAPI char *
ADDCALL psmove_util_get_file_path(const char *filename);


#ifdef __cplusplus
}
#endif

#endif

