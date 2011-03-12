/**
 * PS Move API
 * Copyright (c) 2011 Thomas Perl <thp.io/about>
 * All Rights Reserved
 **/

#ifndef __PSMOVE_H
#define __PSMOVE_H

#include "hidapi.h"

/* Vendor ID and Product ID of PS Move Controller */
#define PSMOVE_VID 0x054c
#define PSMOVE_PID 0x03d5

/* Buffer size for writing LEDs and reading sensor data */
#define PSMOVE_BUFFER_SIZE 49

/* Buffer size for calibration data */
#define PSMOVE_CALIBRATION_SIZE 49

/* Buffer size for the Bluetooth address get request */
#define PSMOVE_BTADDR_GET_SIZE 16

/* Buffer size for the Bluetooth address set request */
#define PSMOVE_BTADDR_SET_SIZE 23

enum PSMove_Connection_Type {
    PSMove_Bluetooth,
    PSMove_USB,
    PSMove_Unknown,
};

enum PSMove_Button {
    PSMove_L2 = 1 << 0x00,
    PSMove_R2 = 1 << 0x01,
    PSMove_L1 = 1 << 0x02,
    PSMove_R1 = 1 << 0x03,
    PSMove_TRIANGLE = 1 << 0x04,
    PSMove_CIRCLE = 1 << 0x05,
    PSMove_CROSS = 1 << 0x06,
    PSMove_SQUARE = 1 << 0x07,
    PSMove_SELECT = 1 << 0x08,
    PSMove_L3 = 1 << 0x09,
    PSMove_R3 = 1 << 0x0A,
    PSMove_START = 1 << 0x0B,
    PSMove_UP = 1 << 0x0C,
    PSMove_RIGHT = 1 << 0x0D,
    PSMove_DOWN = 1 << 0x0E,
    PSMove_LEFT = 1 << 0x0F,
    PSMove_PS = 1 << 0x10,

    PSMove_MOVE = 1 << 0x13,
    PSMove_T = 1 << 0x14,
};

enum PSMove_Request_Type {
    PSMove_Req_GetInput = 0x01,
    PSMove_Req_SetLEDs = 0x02,
    PSMove_Req_GetBTAddr = 0x04,
    PSMove_Req_SetBTAddr = 0x05,
    PSMove_Req_GetCalibration = 0x10,
};

typedef struct {
    unsigned char type; /* message type, must be PSMove_Req_SetLEDs */
    unsigned char _zero; /* must be zero */
    unsigned char r; /* red value, 0x00..0xff */
    unsigned char g; /* green value, 0x00..0xff */
    unsigned char b; /* blue value, 0x00..0xff */
    unsigned char rumble2; /* unknown, should be 0x00 for now */
    unsigned char rumble; /* rumble value, 0x00..0xff */
    unsigned char _padding[PSMOVE_BUFFER_SIZE-7]; /* must be zero */
} PSMove_Data_LEDs;

typedef struct {
    unsigned char type; /* message type, must be PSMove_Req_GetInput */
    unsigned char buttons1;
    unsigned char buttons2;
    unsigned char buttons3;
    unsigned char buttons4;
    unsigned char _unk5;
    unsigned char trigger; /* trigger value; 0..255 */
    unsigned char _unk7;
    unsigned char _unk8;
    unsigned char _unk9;
    unsigned char _unk10;
    unsigned char _unk11;
    unsigned char _unk12;
    unsigned char aXlow; /* low byte of accelerometer X value */
    unsigned char aXhigh; /* high byte of accelerometer X value */
    unsigned char aYlow;
    unsigned char aYhigh;
    unsigned char aZlow;
    unsigned char aZhigh;
    unsigned char aXlow2; /* low byte of accelerometer X value, 2nd frame */
    unsigned char aXhigh2; /* high byte of accelerometer X value, 2nd frame */
    unsigned char aYlow2;
    unsigned char aYhigh2;
    unsigned char aZlow2;
    unsigned char aZhigh2;
    unsigned char gXlow; /* low byte of gyro X value */
    unsigned char gXhigh; /* high byte of gyro X value */
    unsigned char gYlow;
    unsigned char gYhigh;
    unsigned char gZlow;
    unsigned char gZhigh;
    unsigned char gXlow2; /* low byte of gyro X value, 2nd frame */
    unsigned char gXhigh2; /* high byte of gyro X value, 2nd frame */
    unsigned char gYlow2;
    unsigned char gYhigh2;
    unsigned char gZlow2;
    unsigned char gZhigh2;
    unsigned char mag38; /* magnetometer data, weirdly aligned (see code) */
    unsigned char mag39;
    unsigned char mag40;
    unsigned char mag41;
    unsigned char mag42;
    unsigned char _padding[PSMOVE_BUFFER_SIZE-42]; /* unknown */
} PSMove_Data_Input;

/* A Bluetooth address. Most significant byte first. */
typedef unsigned char PSMove_Data_BTAddr[6];

typedef struct {
    /* The handle to the HIDAPI device */
    hid_device *handle;

    /* Various buffers for PS Move-related data */
    PSMove_Data_LEDs leds;
    PSMove_Data_Input input;
} PSMove;

/**
 * Connect to a PS Move controller
 * Returns: A newly-allocated PSMove structure or NULL on error
 **/
PSMove *
PSMove_connect();

/**
 * Determine the connection type of the controllerj
 * Returns: An enum PSMove_Connection_Type value
 **/
enum PSMove_Connection_Type
PSMove_connection_type(PSMove *move);

/**
 * Get the currently-set Host Bluetooth address that is used
 * to connect via Bluetooth when the PS button is pressed.
 *
 * addr might be NULL in which case the address and calibration
 * data re retrieved, but the Bluetooth address is discarded.
 **/
int
PSMove_get_btaddr(PSMove *move, PSMove_Data_BTAddr *addr);

/**
 * Set the Host Bluetooth address that is used to connect via
 * Bluetooth. You should set this to the local computer's
 * Bluetooth address when connected via USB, then disconnect
 * and press the PS button to connect the controller via BT.
 **/
int
PSMove_set_btaddr(PSMove *move, const PSMove_Data_BTAddr *addr);

/**
 * Set the LEDs of the PS Move controller. You need to
 * call PSMove_update_leds() to send the update to the
 * controller.
 **/
void
PSMove_set_leds(PSMove *move, unsigned char r, unsigned char g,
        unsigned char b);

/**
 * Set the rumble value of the PS Move controller. You
 * need to call PSMove_update_leds() to send the update
 * to the controller.
 **/
void
PSMove_set_rumble(PSMove *move, unsigned char rumble);

/**
 * Re-send the LED and Rumble status bits. This needs to
 * be done regularly to keep the LEDs and rumble turned on.
 *
 * Returns: Nonzero on success, zero on error
 **/
int
PSMove_update_leds(PSMove *move);

/**
 * Polls the PS Move for new sensor/button data.
 * Returns a positive number (sequence number + 1) if new data is
 * available or zero if no data is available.
 **/
int
PSMove_poll(PSMove *move);

/**
 * Get the current status of the PS Move buttons. You need to call
 * PSMove_poll() to read new data from the controller first.
 **/
unsigned int
PSMove_get_buttons(PSMove *move);

/**
 * Get the current value of the PS Move analog trigger. You need to
 * call PSMove_poll() to read new data from the controller first.
 **/
unsigned char
PSMove_get_trigger(PSMove *move);

/**
 * Get the current accelerometer readings from the PS Move. You need
 * to call PSMove_poll() to read new data from the controller first.
 *
 * ax, ay and az should be pointers to integer locations that you want
 * to have filled with values. If you don't care about one of these
 * values, simply pass NULL and the field will be ignored..
 **/
void
PSMove_get_accelerometer(PSMove *move, int *ax, int *ay, int *az);

/**
 * Same as PSMove_get_accelerometer(), but for the gyroscope.
 **/
void
PSMove_get_gyroscope(PSMove *move, int *gx, int *gy, int *gz);

/**
 * Same as PSMove_get_accelerometer(), but for the magnetometer.
 * See http://youtu.be/ltOQB_q1UTg for calibration instructions.
 * This is not really tested. YMMV.
 **/
void
PSMove_get_magnetometer(PSMove *move, int *mx, int *my, int *mz);

/**
 * Disconnect from the PS Move and free resources
 **/
void
PSMove_disconnect(PSMove *move);

#endif

