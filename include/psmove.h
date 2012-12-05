
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

#ifdef _WIN32
#  define ADDCALL __cdecl
#  if defined(BUILDING_STATIC_LIBRARY)
#    define ADDAPI
#  elif defined(USING_STATIC_LIBRARY)
#    define ADDAPI
#  elif defined(BUILDING_SHARED_LIBRARY)
#    define ADDAPI __declspec(dllexport)
#  else /* using shared library */
#    define ADDAPI __declspec(dllimport)
#  endif
#else
#  define ADDAPI
#  define ADDCALL
#endif

/*! Connection type for controllers.
 * Controllers can be connected via USB or via Bluetooth. The USB connection is
 * required when you want to pair the controller and for saving the calibration
 * blob. The Bluetooth connection is required when you want to read buttons and
 * calibration values. LED and rumble settings can be done with both Bluetooth
 * and USB.
 *
 * Used by psmove_connection_type().
 **/
enum PSMove_Connection_Type {
    Conn_Bluetooth, /*!< The controller is connected via Bluetooth */
    Conn_USB, /*!< The controller is connected via USB */
    Conn_Unknown, /*!< Unknown connection type / other error */
};

/*! Button flags.
 * The values of each button when pressed. The values returned
 * by the button-related functions return an integer. You can
 * use the logical-and operator (&) to check if a button is
 * pressed (in case of psmove_get_buttons()) or if a button
 * state has changed (in case of psmove_get_button_events()).
 *
 * Used by psmove_get_buttons() and psmove_get_button_events().
 **/
enum PSMove_Button {
    Btn_TRIANGLE = 1 << 0x04, /*!< Green triangle */
    Btn_CIRCLE = 1 << 0x05, /*!< Red circle */
    Btn_CROSS = 1 << 0x06, /*!< Blue cross */
    Btn_SQUARE = 1 << 0x07, /*!< Pink square */

    Btn_SELECT = 1 << 0x08, /*!< Select button, left side */
    Btn_START = 1 << 0x0B, /*!< Start button, right side */

    Btn_MOVE = 1 << 0x13, /*!< Move button, big front button */
    Btn_T = 1 << 0x14, /*!< Trigger, on the back */
    Btn_PS = 1 << 0x10, /*!< PS button, front center */

#if 0
    /* Not used for now - only on Sixaxis/DS3 or nav controller */
    Btn_L2 = 1 << 0x00,
    Btn_R2 = 1 << 0x01,
    Btn_L1 = 1 << 0x02,
    Btn_R1 = 1 << 0x03,
    Btn_L3 = 1 << 0x09,
    Btn_R3 = 1 << 0x0A,
    Btn_UP = 1 << 0x0C,
    Btn_RIGHT = 1 << 0x0D,
    Btn_DOWN = 1 << 0x0E,
    Btn_LEFT = 1 << 0x0F,
#endif
};


/*! Frame of an input report.
 * Each input report sent by the PS Move Controller contains two readings for
 * the accelerometer and the gyroscope. The first one is the older one, and the
 * second one is the most recent one. If you need 120 Hz updates, you can
 * process both frames for each update. If you only need the latest reading,
 * use the second frame and ignore the first frame.
 *
 * Used by psmove_get_accelerometer_frame() and psmove_get_gyroscope_frame().
 **/
enum PSMove_Frame {
    Frame_FirstHalf = 0, /*!< The older frame */
    Frame_SecondHalf, /*!< The most recent frame */
};

/*! Battery charge level.
 * Charge level of the battery. Charging is indicated when the controller is
 * connected via USB, or when the controller is sitting in the charging dock.
 * In all other cases (Bluetooth, not in charging dock), the charge level is
 * indicated.
 *
 * Used by psmove_get_battery().
 **/
enum PSMove_Battery_Level {
    Batt_MIN = 0x00, /*!< Battery is almost empty (< 20%) */
    Batt_20Percent = 0x01, /*!< Battery has at least 20% remaining */
    Batt_40Percent = 0x02, /*!< Battery has at least 40% remaining */
    Batt_60Percent = 0x03, /*!< Battery has at least 60% remaining */
    Batt_80Percent = 0x04, /*!< Battery has at least 80% remaining */
    Batt_MAX = 0x05, /*!< Battery is fully charged (not on charger) */
    Batt_CHARGING = 0xEE, /*!< Battery is currently being charged */
    Batt_CHARGING_DONE = 0xEF, /*!< Battery is fully charged (on charger) */
};

/*! LED update result, returned by psmove_update_leds() */
enum PSMove_Update_Result {
    Update_Failed = 0, /*!< Could not update LEDs */
    Update_Success, /*!< LEDs successfully updated */
    Update_Ignored, /*!< LEDs don't need updating, see psmove_set_rate_limiting() */
};

/*! Boolean type. Use them instead of 0 and 1 to improve code readability. */
enum PSMove_Bool {
    PSMove_False = 0, /*!< False, Failure, Disabled (depending on context) */
    PSMove_True = 1, /*!< True, Success, Enabled (depending on context) */
};

/*! Remote configuration options, for psmove_set_remote_config() */
enum PSMove_RemoteConfig {
    PSMove_LocalAndRemote = 0, /*!< Use both local (hidapi) and remote (moved) devices */
    PSMove_OnlyLocal = 1, /*!< Use only local (hidapi) devices, ignore remote devices */
    PSMove_OnlyRemote = 2, /*!< Use only remote (moved) devices, ignore local devices */
};

#ifndef SWIG
struct _PSMove;
typedef struct _PSMove PSMove; /*!< Handle to a PS Move Controller.
                                    Obtained via psmove_connect_by_id() */
#endif

/**
 * \brief Enable or disable the usage of local or remote devices
 *
 * By default, both local (hidapi) and remote (moved) controllers will be
 * used. If your application only wants to use locally-connected devices,
 * and ignore any remote controllers, call this function with
 * \ref PSMove_OnlyLocal - to use only remotely-connected devices, use
 * \ref PSMove_OnlyRemote instead.
 *
 * This function must be called before any other PS Move API functions are
 * used, as it changes the behavior of counting and connecting to devices.
 *
 * \param config \ref PSMove_LocalAndRemote, \ref PSMove_OnlyLocal or
 *               \ref PSMove_OnlyRemote
 **/
ADDAPI void
ADDCALL psmove_set_remote_config(enum PSMove_RemoteConfig config);

/**
 * \brief Get the number of available controllers
 *
 * \return Number of controllers available (USB + Bluetooth + Remote)
 **/
ADDAPI int
ADDCALL psmove_count_connected();

/**
 * \brief Connect to the default PS Move controller
 *
 * This is a convenience function, having the same effect as:
 *
 * \code psmove_connect_by_id(0) \endcode
 *
 * \return A new \ref PSMove handle, or \c NULL on error
 **/
ADDAPI PSMove *
ADDCALL psmove_connect();

/**
 * \brief Connect to a specific PS Move controller
 *
 * This will connect to a controller based on its index. The controllers
 * available are usually:
 *
 *  1. The locally-connected controllers (USB, Bluetooth)
 *  2. The remotely-connected controllers (exported via \c moved)
 *
 * The order of controllers can be different on each application start,
 * so use psmove_get_serial() to identify the controllers if more than
 * one is connected, and you need some fixed ordering. The global ordering
 * (first local controllers, then remote controllers) is fixed, and will
 * be guaranteed by the library.
 *
 * \param id Zero-based index of the controller to connect to
 *           (0 .. psmove_count_connected() - 1)
 *
 * \return A new \ref PSMove handle, or \c NULL on error
 **/
ADDAPI PSMove *
ADDCALL psmove_connect_by_id(int id);

/**
 * \brief Get the connection type of a PS Move controller
 *
 * For now, controllers connected via USB can't use psmove_poll() and
 * all related features (sensor and button reading, etc..). Because of
 * this, you might want to check if the controllers are connected via
 * Bluetooth before using psmove_poll() and friends.
 *
 * \param move A valid \ref PSMove handle
 *
 * \return \ref Conn_Bluetooth if the controller is connected via Bluetooth
 * \return \ref Conn_USB if the controller is connected via USB
 * \return \ref Conn_Unknown on error
 **/
ADDAPI enum PSMove_Connection_Type
ADDCALL psmove_connection_type(PSMove *move);

/**
 * \brief Check if the controller is remote (\c moved) or local.
 *
 * This can be used to determine to which machine the controller is
 * connected to, and can be helpful in debugging, or if you need to
 * handle remote controllers differently from local controllers.
 *
 * \param move A valid \ref PSMove handle
 *
 * \return \ref PSMove_False if the controller is connected locally
 * \return \ref PSMove_True if the controller is connected remotely
 **/
ADDAPI enum PSMove_Bool
ADDCALL psmove_is_remote(PSMove *move);

/**
 * \brief Get the serial number (Bluetooth MAC address) of a controller.
 *
 * The serial number is usually the Bluetooth MAC address of a
 * PS Move controller. This can be used to identify different
 * controllers when multiple controllers are connected, and is
 * especially helpful if your application needs to identify
 * controllers or guarantee a special ordering.
 *
 * The resulting value has the format:
 *
 * \code "aa:bb:cc:dd:ee:ff" \endcode
 *
 * \param move A valid \ref PSMove handle
 *
 * \return The serial number of the controller. The caller must
 *         free() the result when it is not needed anymore.
 **/
ADDAPI char *
ADDCALL psmove_get_serial(PSMove *move);

/**
 * \brief Pair a controller connected via USB with the computer.
 *
 * This function assumes that psmove_connection_type() returns
 * \ref Conn_USB for the given controller. This will set the
 * target Bluetooth host address of the controller to this
 * computer's default Bluetooth adapter address. This function
 * has been implemented and tested with the following operating
 * systems and Bluetooth stacks:
 *
 *  * Linux 2.6 (Bluez)
 *  * Mac OS X >= 10.6
 *  * Windows 7 (Microsoft Bluetooth stack)
 *
 * \attention On Windows, this function does not work with 3rd
 * party stacks like Bluesoleil. Use psmove_pair_custom() and
 * supply the Bluetooth host adapter address manually. It is
 * recommended to only use the Microsoft Bluetooth stack for
 * Windows with the PS Move API to avoid problems.
 *
 * If your computer doesn't have USB host mode (e.g. because it
 * is a mobile device), you can use psmove_pair_custom() on a
 * different computer and specify the Bluetooth address of the
 * mobile device instead. For most use cases, you can use the
 * \c psmovepair command-line utility.
 *
 * \param move A valid \ref PSMove handle
 *
 * \return \ref PSMove_True if the pairing was successful
 * \return \ref PSMove_False if the pairing failed
 **/
ADDAPI enum PSMove_Bool
ADDCALL psmove_pair(PSMove *move);

/**
 * \brief Pair a controller connected via USB to a specific address.
 *
 * This function behaves the same as psmove_pair(), but allows you to
 * specify a custom Bluetooth host address.
 *
 * \param move A valid \ref PSMove handle
 * \param btaddr_string The host address in the format \c "aa:bb:cc:dd:ee:ff"
 *
 * \return \ref PSMove_True if the pairing was successful
 * \return \ref PSMove_False if the pairing failed
 **/
ADDAPI enum PSMove_Bool
ADDCALL psmove_pair_custom(PSMove *move, const char *btaddr_string);

/**
 * \brief Enable or disable LED update rate limiting.
 *
 * If LED update rate limiting is enabled, psmove_update_leds() will make
 * ignore extraneous updates (and return \ref Update_Ignored) if the update
 * rate is too high, or if the color hasn't changed and the timeout has not
 * been hit.
 *
 * By default, rate limiting is enabled.
 *
 * \warning If rate limiting is disabled, the read performance might
 *          be decreased, especially on Linux.
 *
 * \param move A valid \ref PSMove handle
 * \param enabled \ref PSMove_True to enable rate limiting,
 *                \ref PSMove_False to disable
 **/
ADDAPI void
ADDCALL psmove_set_rate_limiting(PSMove *move, enum PSMove_Bool enabled);

/**
 * \brief Set the RGB LEDs on the PS Move controller.
 *
 * This sets the RGB values of the LEDs on the Move controller. Usage examples:
 *
 * \code
 *    psmove_set_leds(move, 255, 255, 255);  // white
 *    psmove_set_leds(move, 255, 0, 0);      // red
 *    psmove_set_leds(move, 0, 0, 0);        // black (off)
 * \endcode
 *
 * This function will only update the library-internal state of the controller.
 * To really update the LEDs (write the changes out to the controller), you
 * have to call psmove_update_leds() after calling this function.
 *
 * \param move A valid \ref PSMove handle
 * \param r The red value (0..255)
 * \param g The green value (0..255)
 * \param b The blue value (0..255)
 **/
ADDAPI void
ADDCALL psmove_set_leds(PSMove *move, unsigned char r, unsigned char g,
        unsigned char b);

/**
 * \brief Set the rumble intensity of the PS Move controller.
 *
 * This sets the rumble (vibration motor) intensity of the
 * Move controller. Usage example:
 *
 * \code
 *   psmove_set_rumble(move, 255);  // strong rumble
 *   psmove_set_rumble(move, 128);  // medium rumble
 *   psmove_set_rumble(move, 0);    // rumble off
 * \endcode
 *
 * This function will only update the library-internal state of the controller.
 * To really update the rumble intensity (write the changes out to the
 * controller), you have to call psmove_update_leds() (the rumble value is sent
 * together with the LED updates, that's why you have to call it even for
 * rumble updates) after calling this function.
 *
 * \param move A valid \ref PSMove handle
 * \param rumble The rumble intensity (0..255)
 **/
ADDAPI void
ADDCALL psmove_set_rumble(PSMove *move, unsigned char rumble);

/**
 * \brief Send LED and rumble values to the controller.
 *
 * This writes the LED and rumble changes to the controller. You have to call
 * this function regularly, or the controller will switch off the LEDs and
 * rumble automatically (after about 4-5 seconds). When rate limiting is
 * enabled, you can just call this function in your main loop, and the LEDs
 * will stay on properly (with extraneous updates being ignored to not flood
 * the controller with updates).
 *
 * When rate limiting (see psmove_set_rate_limiting()) is disabled, you have
 * to make sure to not call this function not more often then e.g. every
 * 80 ms to avoid flooding the controller with updates.
 *
 * \param move A valid \ref PSMove handle
 *
 * \return \ref Update_Success on success
 * \return \ref Update_Ignored if the change was ignored (see psmove_set_rate_limiting())
 * \return \ref Update_Failed (= \c 0) on error
 **/
ADDAPI enum PSMove_Update_Result
ADDCALL psmove_update_leds(PSMove *move);

/**
 * \brief Read new sensor/button data from the controller.
 *
 * For most sensor and button functions, you have to call this function
 * to read new updates from the controller.
 *
 * How to detect dropped frames:
 *
 * \code
 *     int seq_old = 0;
 *     while (1) {
 *         int seq = psmove_poll(move);
 *         if ((seq_old > 0) && ((seq_old % 16) != (seq - 1))) {
 *             // dropped frames
 *         }
 *         seq_old = seq;
 *     }
 * \endcode
 *
 * In practice, you usually use this function in a main loop and guard
 * all your sensor/button updating functions with it:
 *
 * \code
 *     while (1) {
 *         if (psmove_poll(move)) {
 *             unsigned int pressed, released;
 *             psmove_get_button_events(move, &pressed, &released);
 *             // process button events
 *         }
 *
 *         // update the application state
 *         // draw the current frame
 *     }
 * \endcode
 *
 * \return a positive sequence number (1..16) if new data is
 *         available
 * \return \c 0 if no (new) data is available or an error occurred
 *
 * \param move A valid \ref PSMove handle
 **/
ADDAPI int
ADDCALL psmove_poll(PSMove *move);

/**
 * \brief Get the current button states from the controller.
 *
 * The status of the buttons is described as a bitfield, with a bit
 * in the result being \c 1 when the corresponding \ref PSMove_Button
 * is pressed.
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * Example usage:
 *
 * \code
 *     if (psmove_poll(move)) {
 *         unsigned int buttons = psmove_get_buttons(move);
 *         if (buttons & Btn_PS) {
 *             printf("The PS button is currently pressed.\n");
 *         }
 *     }
 * \endcode
 *
 * \param move A valid \ref PSMove handle
 *
 * \return A bit field of \ref PSMove_Button states
 **/
ADDAPI unsigned int
ADDCALL psmove_get_buttons(PSMove *move);


/**
 * \brief Get new button events since the last call to this fuction.
 *
 * This is an advanced version of psmove_get_buttons() that takes care
 * of tracking the previous button states and comparing the previous
 * states with the current states to generate two bitfields, which is
 * usually more suitable for event-driven applications:
 *
 *  * \c pressed - all buttons that have been pressed since the last call
 *  * \c released - all buttons that have been released since the last call
 *
 * Example usage:
 *
 * \code
 *     if (psmove_poll(move)) {
 *         unsigned int pressed, released;
 *         psmove_get_button_events(move, &pressed, &released);
 *
 *         if (pressed & Btn_MOVE) {
 *             printf("The Move button has been pressed now.\n");
 *         } else if (released & Btn_MOVE) {
 *             printf("The Move button has been released now.\n");
 *         }
 *     }
 * \endcode
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * \param move A valid \ref PSMove handle
 * \param pressed Pointer to store a bitfield of new press events \c NULL
 * \param released Pointer to store a bitfield of new release events \c NULL
 **/
ADDAPI void
ADDCALL psmove_get_button_events(PSMove *move, unsigned int *pressed,
        unsigned int *released);

/**
 * \brief Get the battery charge level of the controller.
 *
 * This function retrieves the charge level of the controller or
 * the charging state (if the controller is currently being charged).
 *
 * See \ref PSMove_Battery_Level for details on the result values.
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * \param move A valid \ref PSMove handle
 * \return A \ref PSMove_Battery_Level (\ref Batt_CHARGING when charging)
 **/
ADDAPI enum PSMove_Battery_Level
ADDCALL psmove_get_battery(PSMove *move);

/**
 * \brief Get the current raw temperature reading of the controller.
 *
 * This gets the raw sensor value of the internal temperature sensor.
 *
 * \bug Right now, the value range of the temperature sensor is now
 *      known, so you have to experiment with the values yourself.
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * \param move A valid \ref PSMove handle
 *
 * \return The raw temperature sensor reading
 **/
ADDAPI int
ADDCALL psmove_get_temperature(PSMove *move);

/**
 * \brief Get the value of the PS Move analog trigger.
 *
 * Get the current value of the PS Move analog trigger. The trigger
 * is also exposed as digital button using psmove_get_buttons() in
 * combination with \ref Btn_T.
 *
 * Usage example:
 *
 * \code
 *     // Control the red LED brightness via the trigger
 *     while (1) {
 *         if (psmove_poll()) {
 *             unsigned char value = psmove_get_trigger(move);
 *             psmove_set_leds(move, value, 0, 0);
 *             psmove_update_leds(move);
 *         }
 *     }
 * \endcode
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * \param move A valid \ref PSMove handle
 *
 * \return 0 if the trigger is not pressed
 * \return 1-254 when the trigger is partially pressed
 * \return 255 if the trigger is fully pressed
 **/
ADDAPI unsigned char
ADDCALL psmove_get_trigger(PSMove *move);

/**
 * \brief Get the raw accelerometer reading from the PS Move.
 *
 * This function reads the raw (uncalibrated) sensor values from
 * the controller. To read calibrated sensor values, use
 * psmove_get_accelerometer_frame().
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * \param move A valid \ref PSMove handle
 * \param ax Pointer to store the raw X axis reading, or \c NULL
 * \param ay Pointer to store the raw Y axis reading, or \c NULL
 * \param az Pointer to store the raw Z axis reading, or \c NULL
 **/
ADDAPI void
ADDCALL psmove_get_accelerometer(PSMove *move, int *ax, int *ay, int *az);

/**
 * \brief Get the raw gyroscope reading from the PS Move.
 *
 * This function reads the raw (uncalibrated) sensor values from
 * the controller. To read calibrated sensor values, use
 * psmove_get_gyroscope_frame().
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * \param move A valid \ref PSMove handle
 * \param gx Pointer to store the raw X axis reading, or \c NULL
 * \param gy Pointer to store the raw Y axis reading, or \c NULL
 * \param gz Pointer to store the raw Z axis reading, or \c NULL
 **/
ADDAPI void
ADDCALL psmove_get_gyroscope(PSMove *move, int *gx, int *gy, int *gz);

/**
 * \brief Get the raw magnetometer reading from the PS Move.
 *
 * This function reads the raw sensor values from the controller,
 * pointing to magnetic north.
 *
 * The result value range is -2048..+2047. The magnetometer is located
 * roughly below the glowing orb - you can glitch the values with a
 * strong kitchen magnet by moving it around the bottom ring of the orb.
 * You can detect if a magnet is nearby by checking if any two values
 * stay at zero for several frames.
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * \param move A valid \ref PSMove handle
 * \param mx Pointer to store the raw X axis reading, or \c NULL
 * \param my Pointer to store the raw Y axis reading, or \c NULL
 * \param mz Pointer to store the raw Z axis reading, or \c NULL
 **/
ADDAPI void
ADDCALL psmove_get_magnetometer(PSMove *move, int *mx, int *my, int *mz);

/**
 * \brief Get the calibrated accelerometer values (in g) from the controller.
 *
 * Assuming that psmove_has_calibration() returns \ref PSMove_True, this
 * function will give you the calibrated accelerometer values in g. To get
 * the raw accelerometer readings, use psmove_get_accelerometer().
 *
 * Usage example:
 *
 * \code
 *     if (psmove_poll(move)) {
 *         float ay;
 *         psmove_get_accelerometer_frame(move, Frame_SecondHalf,
 *                 NULL, &ay, NULL);
 *
 *         if (ay > 0.5) {
 *             printf("Controller is pointing up.\n");
 *         } else if (ay < -0.5) {
 *             printf("Controller is pointing down.\n");
 *         }
 *     }
 * \endcode
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * \param move A valid \ref PSMove handle
 * \param frame \ref Frame_FirstHalf or \ref Frame_SecondHalf (see \ref PSMove_Frame)
 * \param ax Pointer to store the X axis reading, or \c NULL
 * \param ay Pointer to store the Y axis reading, or \c NULL
 * \param az Pointer to store the Z axis reading, or \c NULL
 **/
ADDAPI void
ADDCALL psmove_get_accelerometer_frame(PSMove *move, enum PSMove_Frame frame,
        float *ax, float *ay, float *az);

/**
 * \brief Get the calibrated gyroscope values (in rad/s) from the controller.
 *
 * Assuming that psmove_has_calibration() returns \ref PSMove_True, this
 * function will give you the calibrated gyroscope values in rad/s. To get
 * the raw gyroscope readings, use psmove_get_gyroscope().
 *
 * Usage example:
 *
 * \code
 *     if (psmove_poll(move)) {
 *         float gz;
 *         psmove_get_gyroscope_frame(move, Frame_SecondHalf,
 *                 NULL, NULL, &gz);
 *
 *         // Convert rad/s to RPM
 *         gz = gz * 60 / (2*M_PI);
 *
 *         printf("Rotation: %.2f RPM\n", gz);
 *     }
 * \endcode
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * \param move A valid \ref PSMove handle
 * \param frame \ref Frame_FirstHalf or \ref Frame_SecondHalf (see \ref PSMove_Frame)
 * \param gx Pointer to store the X axis reading, or \c NULL
 * \param gy Pointer to store the Y axis reading, or \c NULL
 * \param gz Pointer to store the Z axis reading, or \c NULL
 **/
ADDAPI void
ADDCALL psmove_get_gyroscope_frame(PSMove *move, enum PSMove_Frame frame,
        float *gx, float *gy, float *gz);

/**
 * \brief Get the normalized magnetometer vector from the controller.
 *
 * The normalized magnetometer vector is a three-axis vector where each
 * component is in the range [-1,+1], including both endpoints. The range
 * will be dynamically determined based on the highest (and lowest) value
 * observed during runtime. To get the raw magnetometer readings, use
 * psmove_get_magnetometer().
 *
 * You need to call psmove_poll() first to read new data from the
 * controller.
 *
 * \param move A valid \ref PSMove handle
 * \param mx Pointer to store the X axis reading, or \c NULL
 * \param my Pointer to store the Y axis reading, or \c NULL
 * \param mz Pointer to store the Z axis reading, or \c NULL
 **/
ADDAPI void
ADDCALL psmove_get_magnetometer_vector(PSMove *move,
        float *mx, float *my, float *mz);

/**
 * \brief Check if calibration is available on this controller.
 *
 * For psmove_get_accelerometer_frame() and psmove_get_gyroscope_frame()
 * to work, the calibration data has to be availble. This usually happens
 * at pairing time via USB. The calibration files are stored in the PS
 * Move API data directory (see psmove_util_get_data_dir()) and can be
 * copied between machines (e.g. from the machine you do your pairing to
 * the machine where you run the API on, which is especially important for
 * mobile devices, where USB host mode might not be supported).
 *
 * If no calibration is available, the two functions returning calibrated
 * values will return uncalibrated values. Also, the orientation features
 * will not work without calibration.
 *
 * \param move A valid \ref PSMove handle
 *
 * \return \ref PSMove_True if calibration is supported, \ref PSMove_False otherwise
 **/
ADDAPI enum PSMove_Bool
ADDCALL psmove_has_calibration(PSMove *move);

/**
 * \brief Dump the calibration information to stdout.
 *
 * This is mostly useful for developers wanting to analyze the
 * calibration data of a given PS Move controller for debugging
 * or development purposes.
 *
 * The current calibration information (if available) will be printed to \c
 * stdout, including an interpretation of raw values where available.
 *
 * \param move A valid \ref PSMove handle
 **/
ADDAPI void
ADDCALL psmove_dump_calibration(PSMove *move);

/**
 * \brief Enable or disable orientation tracking.
 *
 * This will enable orientation tracking and update the internal orientation
 * quaternion (which can be retrieved using psmove_get_orientation()) when
 * psmove_poll() is called.
 *
 * In addition to enabling the orientation tracking features, calibration data
 * and an orientation algorithm (usually built-in) has to be used, too. You can
 * use psmove_has_orientation() after enabling orientation tracking to check if
 * orientation features can be used.
 *
 * \param move A valid \ref PSMove handle
 * \param enabled \ref PSMove_True to enable orientation tracking, \ref PSMove_False to disable
 **/
ADDAPI void
ADDCALL psmove_enable_orientation(PSMove *move, enum PSMove_Bool enabled);

/**
 * \brief Check if orientation tracking is available for this controller.
 *
 * The orientation tracking feature depends on the availability of an
 * orientation tracking algorithm (usually built-in) and the calibration
 * data availability (as determined by psmove_has_calibration()). In addition
 * to that (because orientation tracking is somewhat computationally
 * intensive, especially on embedded systems), you have to enable the
 * orientation tracking manually via psmove_enable_orientation()).
 *
 * If this function returns \ref PSMove_False, the orientation features
 * will not work - check for missing calibration data and make sure that
 * you have called psmove_enable_orientation() first.
 *
 * \param move A valid \ref PSMove handle
 *
 * \return \ref PSMove_True if calibration is supported, \ref PSMove_False otherwise
 **/
ADDAPI enum PSMove_Bool
ADDCALL psmove_has_orientation(PSMove *move);

/**
 * \brief Get the current orientation as quaternion.
 *
 * This will get the current 3D rotation of the PS Move controller as
 * quaternion. You will have to call psmove_poll() regularly in order to have
 * good quality orientation tracking.
 *
 * In order for the orientation tracking to work, you have to enable tracking
 * first using psmove_enable_orientation().  In addition to enabling tracking,
 * an orientation algorithm has to be present, and calibration data has to be
 * available. You can use psmove_has_orientation() to check if all
 * preconditions are fulfilled to do orientation tracking.
 *
 * \param move A valid \ref PSMove handle
 * \param w A pointer to store the w part of the orientation quaternion
 * \param x A pointer to store the x part of the orientation quaternion
 * \param y A pointer to store the y part of the orientation quaternion
 * \param z A pointer to store the z part of the orientation quaternion
 **/
ADDAPI void
ADDCALL psmove_get_orientation(PSMove *move,
        float *w, float *x, float *y, float *z);

/**
 * \brief Reset the current orientation quaternion.
 *
 * This will set the current 3D rotation of the PS Move controller as
 * quaternion. You can use this function to re-adjust the orientation of the
 * controller when it points towards the camera.
 *
 * \param move A valid \ref PSMove handle
 **/
ADDAPI void
ADDCALL psmove_reset_orientation(PSMove *move);


/**
 * \brief Reset the magnetometer calibration state.
 *
 * This will reset the magnetometer calibration data, so they can be
 * re-adjusted dynamically. Used by the calibration utility.
 *
 * \ref move A valid \ref PSMove handle
 **/
ADDAPI void
ADDCALL psmove_reset_magnetometer_calibration(PSMove *move);

/**
 * \brief Save the magnetometer calibration values.
 *
 * This will save the magnetometer calibration data to persistent storage.
 * If a calibration already exists, this will overwrite the old values.
 *
 * \param move A valid \ref PSMove handle
 **/
ADDAPI void
ADDCALL psmove_save_magnetometer_calibration(PSMove *move);

/**
 * \brief Return the raw magnetometer calibration range.
 *
 * The magnetometer calibration is dynamic at runtime - this function returns
 * the raw range of magnetometer calibration. The user should rotate the
 * controller in all directions to find the response range of the controller
 * (this will be dynamically adjusted).
 *
 * \param move A valid \ref PSMove handle
 *
 * \return The smallest raw sensor range difference of all three axes
 **/
ADDAPI int
ADDCALL psmove_get_magnetometer_calibration_range(PSMove *move);

/**
 * \brief Disconnect from the PS Move and release resources.
 *
 * This will disconnect from the controller and release any resources allocated
 * for handling the controller. Please note that this does not disconnect the
 * controller from the system (as the Bluetooth stack of the operating system
 * usually keeps the HID connection alive), but will rather disconnect the
 * application from the controller.
 *
 * To really disconnect the controller from your computer, you can either press
 * the PS button for ~ 10 seconds or use the Bluetooth application of your
 * operating system to disconnect the controller.
 *
 * \param move A valid \ref PSMove handle (which will be invalid after this call)
 **/
ADDAPI void
ADDCALL psmove_disconnect(PSMove *move);

/**
 * \brief Reinitialize the library.
 *
 * Required for detecting new and removed controllers (at least on Mac OS X).
 * Make sure to disconnect all controllers (using psmove_disconnect) before
 * calling this, otherwise it won't work.
 *
 * You do not need to call this function at application startup.
 *
 * \bug It should be possible to auto-detect newly-connected controllers
 *      without having to rely on this function.
 **/
ADDAPI void
ADDCALL psmove_reinit();

/**
 * \brief Get milliseconds since first library use.
 *
 * This function is used throughout the library to take care of timing and
 * measurement. It implements a cross-platform way of getting the current
 * time, relative to library use.
 *
 * \return Time (in ms) since first library use.
 **/
ADDAPI long
ADDCALL psmove_util_get_ticks();

/**
 * \brief Get local save directory for settings.
 *
 * The local save directory is a PS Move API-specific directory where the
 * library and its components will store files such as calibration data,
 * tracker state and configuration files.
 *
 * \return The local save directory for settings.
 *         The returned value is reserved in static memory - it must not be freed!
 **/
ADDAPI const char *
ADDCALL psmove_util_get_data_dir();

/**
 * \brief Get a filename path in the local save directory.
 *
 * This is a convenience function wrapping psmove_util_get_data_dir()
 * and will give the absolute path of the given filename.
 *
 * The data directory will be created in case it doesn't exist yet.
 *
 * \param filename The basename of the file (e.g. \c myfile.txt)
 *
 * \return The absolute filename to the file. The caller must
 *         free() the result when it is not needed anymore.
 * \return On error, \c NULL is returned.
 **/
ADDAPI char *
ADDCALL psmove_util_get_file_path(const char *filename);

/**
 * \brief Get an integer from an environment variable
 *
 * Utility function used to get configuration from environment
 * variables.
 *
 * \param name The name of the environment variable
 *
 * \return The integer value of the environment variable, or -1 if
 *         the variable is not set or could not be parsed as integer.
 **/
ADDAPI int
ADDCALL psmove_util_get_env_int(const char *name);

/**
 * \brief Get a string from an environment variable
 *
 * Utility function used to get configuration from environment
 * variables.
 *
 * \param name The name of the environment variable
 *
 * \return The string value of the environment variable, or NULL if the
 *         variable is not set. The caller must free() the result when
 *         it is not needed anymore.
 **/
ADDAPI char *
ADDCALL psmove_util_get_env_string(const char *name);


#ifdef __cplusplus
}
#endif

#endif

