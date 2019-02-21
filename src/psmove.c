
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

#include "psmove.h"
#include "psmove_port.h"
#include "psmove_private.h"
#include "psmove_calibration.h"
#include "psmove_orientation.h"
#include "math/psmove_vector.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <wchar.h>
#include <sys/stat.h>
#include <math.h>
#include <limits.h>

#include "daemon/moved_client.h"
#include "hidapi.h"

/* Begin private definitions */

/* Buffer size for writing LEDs and reading sensor data */
#define PSMOVE_BUFFER_SIZE 9

/* Buffer size for the Bluetooth address get request */
/* NOTE: On Windows, reading Feature Request 0x04 sometimes fails for ZCM2
 *       controllers because hidapi occasionally returns a garbage buffer of
 *       much larger size. Increasing the buffer size in the request seems to
 *       reliably fix this. Note that this only happens for Bluetooth
 *       connections, not USB.
 */
#define PSMOVE_BTADDR_GET_SIZE            16
#define PSMOVE_WIN32_ZCM2_BTADDR_GET_SIZE 20
#define PSMOVE_MAX_BTADDR_GET_SIZE        PSMOVE_WIN32_ZCM2_BTADDR_GET_SIZE

/* Buffer size for the Bluetooth address set request */
#define PSMOVE_BTADDR_SET_SIZE 23

/* Buffer size for sending/retrieving a request to an extension device */
#define PSMOVE_EXT_DEVICE_REPORT_SIZE 49

/* Maximum milliseconds to inhibit further updates to LEDs if not changed */
#define PSMOVE_MAX_LED_INHIBIT_MS 4000

/* Minimum time (in milliseconds) between two LED updates (rate limiting) */
#define PSMOVE_MIN_LED_UPDATE_WAIT_MS 120


enum PSMove_Request_Type {
    PSMove_Req_GetInput = 0x01,
    PSMove_Req_SetLEDs = 0x06,
    PSMove_Req_SetLEDPWMFrequency = 0x03,
    PSMove_Req_GetBTAddr = 0x04,
    PSMove_Req_SetBTAddr = 0x05,
    PSMove_Req_GetCalibration = 0x10,
    PSMove_Req_SetAuthChallenge = 0xA0,
    PSMove_Req_GetAuthResponse = 0xA1,
    PSMove_Req_GetExtDeviceInfo = 0xE0,
    PSMove_Req_SetExtDeviceInfo = 0xE0,
    PSMove_Req_SetDFUMode = 0xF2,
    PSMove_Req_GetFirmwareInfo = 0xF9,

    /**
     * Permanently set LEDs via USB
     *
     * Writing USB report 0xFA controls the LEDs. But unlike BT report
     * 0x02 this one keeps the sphere glowing as long as the USB cable
     * is connected, i.e. no refresh updates need to be send. Not sure
     * why that one exists. Might be useful for debugging though.
     *
     * TODO: Can't get this to work, but I could imagine it be useful for
     * things like notification apps that don't have to be running all the
     * time. Maybe it only works in specific controller firmware versions?
     *
     * http://lists.ims.tuwien.ac.at/pipermail/psmove/2013-March/000335.html
     * https://github.com/thp/psmoveapi/issues/55
     **/
    PSMove_Req_SetLEDsPermanentUSB = 0xFA,
};

enum PSMove_Device_Type {
    PSMove_HIDAPI = 0x01,
    PSMove_MOVED = 0x02,
};

enum PSMove_Sensor {
    Sensor_Accelerometer = 0,
    Sensor_Gyroscope,
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

/* Decode 12-bit signed value (assuming two's complement) */
#define TWELVE_BIT_SIGNED(x) (((x) & 0x800)?(-(((~(x)) & 0xFFF) + 1)):(x))

/* Decode 16-bit signed value from data pointer and offset */
static inline int
psmove_decode_16bit(char *data, int offset)
{
    unsigned char low = data[offset] & 0xFF;
    unsigned char high = (data[offset+1]) & 0xFF;
    return (low | (high << 8)) - 0x8000;
}

static inline int
psmove_decode_16bit_twos_complement(char *data, int offset)
{
    unsigned char low = data[offset] & 0xFF;
    unsigned char high = (data[offset+1]) & 0xFF;
    int value = (low | (high << 8));
    return (value & 0x8000) ? (-(~value & 0xFFFF) + 1) : value;
}

#define NUM_PSMOVE_PIDS \
    ((sizeof(PSMOVE_PIDS) / sizeof(PSMOVE_PIDS[0])) - 1)

static int
PSMOVE_PIDS[] = {
    PSMOVE_PID,
    PSMOVE_PS4_PID,
    0,
};

typedef struct {
    unsigned char type; /* message type, must be PSMove_Req_GetInput */
    unsigned char buttons1;
    unsigned char buttons2;
    unsigned char buttons3;
    unsigned char buttons4;
    unsigned char trigger; /* trigger value; 0..255 */
    unsigned char trigger2; /* trigger value, 2nd frame */
    unsigned char _unk7;
    unsigned char _unk8;
    unsigned char _unk9;
    unsigned char _unk10;
    unsigned char timehigh; /* high byte of timestamp */
    unsigned char battery; /* battery level; 0x05 = max, 0xEE = USB charging */
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
    unsigned char temphigh; /* temperature (bits 12-5) */
    unsigned char templow_mXhigh; /* temp (bits 4-1); magneto X (bits 12-9) (magneto only in ZCM1) */
} PSMove_Data_Input_Common;

typedef struct {
    PSMove_Data_Input_Common common;

    unsigned char mXlow; /* magnetometer X (bits 8-1) */
    unsigned char mYhigh; /* magnetometer Y (bits 12-5) */
    unsigned char mYlow_mZhigh; /* magnetometer: Y (bits 4-1), Z (bits 12-9) */
    unsigned char mZlow; /* magnetometer Z (bits 8-1) */
    unsigned char timelow; /* low byte of timestamp */
    unsigned char extdata[PSMOVE_EXT_DATA_BUF_SIZE]; /* external device data (EXT port) */
} PSMove_ZCM1_Data_Input;

typedef struct {
    PSMove_Data_Input_Common common;

    unsigned char timehigh2; /* same as timestamp at offsets 0x0B */
    unsigned char timelow; /* same as timestamp at offsets 0x2B */
    unsigned char _unk41;
    unsigned char _unk42;
    unsigned char timelow2;
} PSMove_ZCM2_Data_Input;

typedef union {
    PSMove_Data_Input_Common common;
    PSMove_ZCM1_Data_Input zcm1;
    PSMove_ZCM2_Data_Input zcm2;
} PSMove_Data_Input;

struct _PSMove {
    /* Device type (hidapi-based or moved-based */
    enum PSMove_Device_Type type;

    /* The handle to the HIDAPI device */
    hid_device *handle;

    hid_device *handle_addr; // Only used by _WIN32. Needed by Win 8.1 to get BT address.

    /* The handle to the moved client */
    moved_client *client;
    int remote_id;

    /* Index (at connection time) - not exposed yet */
    int id;

    /* Various buffers for PS Move-related data */
    PSMove_Data_LEDs leds;
    PSMove_Data_Input input;

    /* Controller hardware model */
    enum PSMove_Model_Type model;

    /* Save location for the serial number */
    char *serial_number;

    /* Device path of the controller */
    char *device_path;

    char *device_path_addr;  // Only used by _WIN32. Needed by Win 8.1 to get BT address.

    /* Nonzero if the value of the LEDs or rumble has changed */
    unsigned char leds_dirty;

    /* Nonzero if LED update rate limiting is enabled */
    enum PSMove_Bool leds_rate_limiting;

    /* Milliseconds timestamp of last LEDs update (psmove_util_get_ticks) */
    long last_leds_update;

    /* Previous values of buttons (psmove_get_button_events) */
    int last_buttons;

    PSMoveCalibration *calibration;
    PSMoveOrientation *orientation;

    /* Is orientation tracking currently enabled? */
    enum PSMove_Bool orientation_enabled;

	/* The direction of the magnetic field found during calibration */
	PSMove_3AxisVector magnetometer_calibration_direction;

    /* Minimum and maximum magnetometer values observed this session */
    PSMove_3AxisVector magnetometer_min;
    PSMove_3AxisVector magnetometer_max;

    enum PSMove_Connection_Type connection_type;
};

enum PSMove_Bool
psmove_load_magnetometer_calibration(PSMove *move);

/* End private definitions */

static moved_client_list *clients;
static int psmove_local_disabled = 0;
static int psmove_remote_disabled = 0;

/* Number of valid, open PSMove* handles "in the wild" */
static int psmove_num_open_handles = 0;



/* Previously public functions, now private: */

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
 **/
void
psmove_get_half_frame(PSMove *move, enum PSMove_Sensor sensor,
        enum PSMove_Frame frame, int *x, int *y, int *z);


/* Start implementation of the API */

enum PSMove_Bool
psmove_init(enum PSMove_Version version)
{
    // Compile-time version of the library is PSMOVE_CURRENT_VERSION
    // Compile-time version of the user code is the value of "version"

    // The requested version must be equal or less than the version implemented,
    // but it's okay if the requested version is less than the implemented version,
    // as we (try to) be backwards compatible with older API users
    if (version <= PSMOVE_CURRENT_VERSION) {
        return PSMove_True;
    } else {
        return PSMove_False;
    }
}

void
psmove_set_remote_config(enum PSMove_RemoteConfig config)
{
    psmove_remote_disabled = (config == PSMove_OnlyLocal);
    psmove_local_disabled = (config == PSMove_OnlyRemote);
}

void
_psmove_write_data(PSMove *move, unsigned char *data, int length)
{
    if (memcmp(&(move->leds), data, length) != 0) {
        memcpy(&(move->leds), data, length);
        move->leds_dirty = 1;
    }
    psmove_update_leds(move);
}

void
_psmove_read_data(PSMove *move, unsigned char *data, int length)
{
    assert(data != NULL);

    int32_t res = psmove_poll(move);
    assert(res <= 0xFF);

    // Put the read result at the beginning of the buffer
    assert(length >= sizeof(int32_t));
    *((int32_t *)data) = res;
    data += sizeof(int32_t);
    length -= sizeof(int32_t);

    switch (move->model) {
        case Model_ZCM1:
            assert(length >= sizeof(move->input.zcm1));
            memcpy(data, &(move->input.zcm1), sizeof(move->input.zcm1));
            break;
        case Model_ZCM2:
            assert(length >= sizeof(move->input.zcm2));
            memcpy(data, &(move->input.zcm2), sizeof(move->input.zcm2));
            break;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            break;
    }
}

enum PSMove_Bool
psmove_is_remote(PSMove *move)
{
    return move->type == PSMove_MOVED;
}

void
psmove_reinit()
{
    if (psmove_num_open_handles != 0) {
        psmove_CRITICAL("reinit called with open handles "
                "(forgot psmove_disconnect?)");
        exit(0);
    }

    if (clients != NULL) {
        moved_client_list_destroy(clients);
        clients = NULL;
    }

    if(!psmove_local_disabled)
        hid_exit();
}

static int
_psmove_count_connected_by_pid(int pid)
{
    struct hid_device_info *devs, *cur_dev;
    int count = 0;

    devs = hid_enumerate(PSMOVE_VID, pid);
    cur_dev = devs;
    while (cur_dev) {
#ifdef _WIN32
        /**
         * Windows Quirk: Each dev is enumerated 3 times.
         * The one with "&col01#" in the path is the one we will get most of our data from. Only count this one.
         * The one with "&col02#" in the path is the one we will get the bluetooth address from.
         **/
        if (strstr(cur_dev->path, "&col01#") == NULL) {
            count--;
        }
#endif
        count++;
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);

    return count;
}

int
psmove_count_connected_hidapi()
{
    int count = 0;

    if (psmove_local_disabled) {
        return 0;
    }

    int *pid = PSMOVE_PIDS;
    while (*pid) {
        count += _psmove_count_connected_by_pid(*pid++);
    }

    return count;
}

int
psmove_count_connected_moved(moved_client *client)
{
    psmove_return_val_if_fail(client != NULL, 0);
    if (moved_client_send(client, MOVED_REQ_COUNT_CONNECTED, 0, NULL, 0)) {
        return client->response_buf.count_connected.count;
    }

    return 0;
}

int
psmove_count_connected()
{
    int count = psmove_count_connected_hidapi();

    if (clients == NULL && !psmove_remote_disabled) {
        clients = moved_client_list_open();
    }

    moved_client_list *cur;
    for (cur=clients; cur != NULL; cur=cur->next) {
        count += psmove_count_connected_moved(cur->client);
    }

    return count;
}

PSMove *
psmove_connect_internal(const wchar_t *serial, const char *path, int id, unsigned short pid)
{
    char *tmp;

    PSMove *move = (PSMove*)calloc(1, sizeof(PSMove));
    move->type = PSMove_HIDAPI;
    move->connection_type = Conn_Unknown;

    /* Make sure the first LEDs update will go through (+ init get_ticks) */
    move->last_leds_update = psmove_util_get_ticks() - PSMOVE_MAX_LED_INHIBIT_MS;

#ifdef _WIN32
    /* Windows Quirk: USB devices have "0" as serial, BT devices their addr */
    if (serial != NULL && wcslen(serial) > 1) {
        move->connection_type = Conn_Bluetooth;
    } else {
        move->connection_type = Conn_USB;
    }
    serial = NULL;  // Set the serial to NULL, even if BT device, to use psmove_get_serial below.

    /**
     * In Windows, the device is enumerated 3 times, and each has slightly different behaviour.
     * The devices' paths differ slightly (col01, col02, and col03).
     * The device with col01 is the one we want to use for data.
     * The device with col02 is the one we want to use for the bluetooth address.
     * More testing remains to determine which device is best for which feature reports.
     **/

    /**
     * We know this function (psmove_connect_internal) will only be called with the col01 path.
     * We first copy that path then modify it to col02. That will be the path for our addr device.
     * Connect to the addr device first, then connect to the main device.
     **/
    move->device_path_addr = strdup(path);
    char *p;
    psmove_return_val_if_fail((p = strstr(move->device_path_addr, "&col01#")) != NULL, NULL);
    p[5] = '2';
    psmove_return_val_if_fail((p = strstr(move->device_path_addr, "&0000#")) != NULL, NULL);
    p[4] = '1';
    move->handle_addr = hid_open_path(move->device_path_addr);
    hid_set_nonblocking(move->handle_addr, 1);

    move->device_path = strdup(path);
    move->handle = hid_open_path(move->device_path);

#else
    /* If not in Windows then we can rely on having only one device. */
    if (path != NULL) {
        move->device_path = strdup(path);
    }
    if ((serial == NULL || wcslen(serial) == 0) && path != NULL) {
        move->handle = hid_open_path(path);
    } else {
        move->handle = hid_open(PSMOVE_VID, pid, serial);
    }

#endif

    if (!move->handle) {
        free(move);
        return NULL;
    }

    /* Use Non-Blocking I/O */
    hid_set_nonblocking(move->handle, 1);

    /* Message type for LED set requests */
    move->leds.type = PSMove_Req_SetLEDs;

    move->model = (pid == PSMOVE_PS4_PID) ? Model_ZCM2 : Model_ZCM1;

    /* Remember the ID/index */
    move->id = id;

    /* Remember the serial number */
    move->serial_number = (char*)calloc(PSMOVE_MAX_SERIAL_LENGTH, sizeof(char));
    if (serial != NULL) {
        wcstombs(move->serial_number, serial, PSMOVE_MAX_SERIAL_LENGTH);
    } else {
        move->serial_number = psmove_get_serial(move);
    }

    // Recently disconnected controllers might still show up in hidapi (especially Windows).
    if (!move->serial_number) {
        if (move->handle) {
            hid_close(move->handle);
        }
        if (move->handle_addr) {  // _WIN32 only
            hid_close(move->handle_addr);
        }
        free(move);
        return NULL;
    } else if (move->connection_type == Conn_Unknown) {
        if (strlen(move->serial_number) == 0) {
            move->connection_type = Conn_USB;
        } else {
            move->connection_type = Conn_Bluetooth;
        }
    }

    /**
     * Normalize "aa-bb-cc-dd-ee-ff" (OS X format) into "aa:bb:cc:dd:ee:ff"
     * Also normalize "AA:BB:CC:DD:EE:FF" into "aa:bb:cc:dd:ee:ff" (lowercase)
     **/
    tmp = move->serial_number;
    while (*tmp != '\0') {
        if (*tmp == '-') {
            *tmp = ':';
        }

        *tmp = (char)tolower(*tmp);
        tmp++;
    }

    /* Bookkeeping of open handles (for psmove_reinit) */
    psmove_num_open_handles++;

    move->calibration = psmove_calibration_new(move);
    move->orientation = psmove_orientation_new(move);

    switch (move->model) {
        case Model_ZCM1:
            /* Load magnetometer calibration data */
            psmove_load_magnetometer_calibration(move);
            break;
        case Model_ZCM2:
            /* No magnetometer on the ZCM2 */
            psmove_reset_magnetometer_calibration(move);
            break;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            break;
    }

    return move;
}

const char *
_psmove_get_device_path(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, NULL);

    return move->device_path;
}

enum PSMove_Bool
_psmove_set_auth_challenge(PSMove *move, PSMove_Data_AuthChallenge *challenge)
{
    unsigned char buf[sizeof(PSMove_Data_AuthChallenge) + 1];
    int res;

    psmove_return_val_if_fail(move != NULL, PSMove_False);

    memset(buf, 0, sizeof(buf));
    buf[0] = PSMove_Req_SetAuthChallenge;

    /* Copy challenge data into send buffer */
    memcpy(buf + 1, challenge, sizeof(buf) - 1);

    res = hid_send_feature_report(move->handle, buf, sizeof(buf));

    return (res == sizeof(buf));
}

PSMove_Data_AuthResponse *
_psmove_get_auth_response(PSMove *move)
{
    unsigned char buf[sizeof(PSMove_Data_AuthResponse) + 1];
    int res;

    psmove_return_val_if_fail(move != NULL, NULL);

    memset(buf, 0, sizeof(buf));
    buf[0] = PSMove_Req_GetAuthResponse;
    res = hid_get_feature_report(move->handle, buf, sizeof(buf));

    psmove_return_val_if_fail(res == sizeof(buf), NULL);

    /* Copy response data into output buffer */
    PSMove_Data_AuthResponse *output_buf = malloc(sizeof(PSMove_Data_AuthResponse));
    memcpy(*output_buf, buf + 1, sizeof(*output_buf));

    return output_buf;
}

PSMove_Firmware_Info *
_psmove_get_firmware_info(PSMove *move)
{
    unsigned char buf[14];
    int res;
    int expected_res = sizeof(buf) - 1;
    unsigned char *p = buf;

    psmove_return_val_if_fail(move != NULL, NULL);

    memset(buf, 0, sizeof(buf));
    buf[0] = PSMove_Req_GetFirmwareInfo;
    res = hid_get_feature_report(move->handle, buf, sizeof(buf));

    /**
     * The Bluetooth report contains the Report ID as additional first byte
     * while the USB report does not. So we need to check the current connection
     * type in order to determine the correct offset for reading from the report
     * buffer.
     *
     * XXX: This also happens for me on macOS with USB, so check the first
     * byte, and if it's the Report ID as first byte, assume we need to remove
     * it from the response.
     **/

    if (psmove_connection_type(move) == Conn_Bluetooth || (res > 0 && buf[0] == PSMove_Req_GetFirmwareInfo)) {
        --res;
        ++p;
    }

    psmove_return_val_if_fail(res >= expected_res, NULL);

    PSMove_Firmware_Info *info = malloc(sizeof(PSMove_Firmware_Info));

    /* NOTE: Each field in the report is stored in Big-Endian byte order */
    info->version    = (p[0] << 8) | p[1];
    info->revision   = (p[2] << 8) | p[3];
    info->bt_version = (p[4] << 8) | p[5];

    /* Copy unknown trailing bytes into info struct */
    memcpy(info->_unknown, p + 6, sizeof(info->_unknown));

    return info;
}

enum PSMove_Bool
_psmove_set_operation_mode(PSMove *move, enum PSMove_Operation_Mode mode)
{
    unsigned char buf[10];
    int res;
    char mode_magic_val;

    psmove_return_val_if_fail(move != NULL, PSMove_False);

    /* We currently support setting STDFU or BTDFU mode only */
    psmove_return_val_if_fail(mode == Mode_STDFU || mode == Mode_BTDFU, PSMove_False);

    switch (mode) {
        case Mode_STDFU:
            mode_magic_val = 0x42;
            break;
        case Mode_BTDFU:
            mode_magic_val = 0x43;
            break;
        default:
            mode_magic_val = 0;
            break;
    }

    memset(buf, 0, sizeof(buf));
    buf[0] = PSMove_Req_SetDFUMode;
    buf[1] = mode_magic_val;
    res = hid_send_feature_report(move->handle, buf, sizeof(buf));

    return (res == sizeof(buf));
}

PSMove *
psmove_connect_remote_by_id(int id, moved_client *client, int remote_id)
{
    PSMove *move = (PSMove*)calloc(1, sizeof(PSMove));
    move->type = PSMove_MOVED;

    // By default, all moved-provided controllers are considered Bluetooth
    move->connection_type = Conn_Bluetooth;

    move->client = client;
    move->remote_id = remote_id;

    /* Message type for LED set requests */
    move->leds.type = PSMove_Req_SetLEDs;

    // TODO: Add support for other models
    move->model = Model_ZCM1;

    /* Remember the ID/index */
    move->id = id;

    /* Remember the serial number */
    if (moved_client_send(move->client, MOVED_REQ_GET_SERIAL, (char)move->remote_id, NULL, 0)) {
        /* Retrieve the serial number from the remote host */
        move->serial_number = _psmove_btaddr_to_string(*((PSMove_Data_BTAddr *)
                move->client->response_buf.get_serial.btaddr));
    } else {
        /* No serial number -- FATAL? */
        psmove_WARNING("Cannot retrieve serial number");
        move->serial_number = (char*)calloc(PSMOVE_MAX_SERIAL_LENGTH, sizeof(char));
    }

    move->calibration = psmove_calibration_new(move);
    // XXX: Copy calibration remotely if possible

    move->orientation = psmove_orientation_new(move);

    /* Load magnetometer calibration data */
    psmove_load_magnetometer_calibration(move);

    /* Bookkeeping of open handles (for psmove_reinit) */
    psmove_num_open_handles++;

    return move;
}

static int
compare_hid_device_info_ptr(const void *a, const void *b)
{
    struct hid_device_info *dev_a = *(struct hid_device_info **)a;
    struct hid_device_info *dev_b = *(struct hid_device_info **)b;

    if ((dev_a->serial_number != NULL) && (wcslen(dev_a->serial_number) != 0) &&
        (dev_b->serial_number != NULL) && (wcslen(dev_b->serial_number) != 0)) {
        return wcscmp(dev_a->serial_number, dev_b->serial_number);
    }

    if (dev_a->path != NULL && dev_b->path != NULL) {
        return strcmp(dev_a->path, dev_b->path);
    }

    psmove_WARNING("Cannot compare serial number or path when sorting devices");
    return 0;
}

static struct hid_device_info *move_hid_devices[NUM_PSMOVE_PIDS];

PSMove *
psmove_connect_by_id(int id)
{
    int hidapi_count = psmove_count_connected_hidapi();

    if (id >= hidapi_count) {
        /* XXX: check remote controllers */
        if (clients == NULL && !psmove_remote_disabled) {
            clients = moved_client_list_open();
        }

        int offset = hidapi_count;

        moved_client_list *cur;
        for (cur=clients; cur != NULL; cur=cur->next) {
            int count = psmove_count_connected_moved(cur->client);
            if ((id - offset) < count) {
                int remote_id = id - offset;
                return psmove_connect_remote_by_id(id, cur->client, remote_id);
            }
            offset += count;
        }

        return NULL;
    }

    struct hid_device_info *devs, *cur_dev;
    PSMove *move = NULL;

    // TODO: Implement handling of multiple PIDs in a cleaner way. Ideally, we
    //       would just build a *single* list of hid_device_info structs.

    // enumerate matching HID devices
    for (unsigned int i = 0; i < NUM_PSMOVE_PIDS; i++) {
        psmove_DEBUG("Enumerating HID devices with PID 0x%04X\n", PSMOVE_PIDS[i]);

        // NOTE: hidapi returns NULL for PIDs that were not found
        move_hid_devices[i] = hid_enumerate(PSMOVE_VID, PSMOVE_PIDS[i]);
    }

    // Count available devices
    int available = 0;
    int i;
    for (i = 0; i < NUM_PSMOVE_PIDS; i++) {
        for (cur_dev = move_hid_devices[i]; cur_dev != NULL; cur_dev = cur_dev->next, available++);
    }
    psmove_DEBUG("Matching HID devices: %d\n", available);

    // Sort list of devices to have stable ordering of devices
    int n = 0;
    struct hid_device_info **devs_sorted = calloc(available, sizeof(struct hid_device_info *));
    for (i = 0; i < NUM_PSMOVE_PIDS; i++) {
        cur_dev = move_hid_devices[i];
        while (cur_dev && (n < available)) {
            devs_sorted[n] = cur_dev;
            cur_dev = cur_dev->next;
            n++;
        }
    }
    qsort((void *)devs_sorted, available, sizeof(struct hid_device_info *), compare_hid_device_info_ptr);

#if defined(PSMOVE_DEBUG)
    for (i=0; i<available; i++) {
        cur_dev = devs_sorted[i];
        char tmp[64];
        wcstombs(tmp, cur_dev->serial_number, sizeof(tmp));
        printf("devs_sorted[%d]: (handle=%p, serial=%s, path=%s)\n", i, cur_dev, tmp, cur_dev->path);
    }
#endif /* defined(PSMOVE_DEBUG) */

#ifdef _WIN32
    int count = 0;
    for (i=0; i<available; i++) {
        cur_dev = devs_sorted[i];

        if (strstr(cur_dev->path, "&col01#") != NULL) {
            if (count == id) {
                move = psmove_connect_internal(cur_dev->serial_number, cur_dev->path, id, cur_dev->product_id);
                break;
            }
        } else {
            count--;
        }

        count++;
    }
#else
    if (id < available) {
        cur_dev = devs_sorted[id];
        move = psmove_connect_internal(cur_dev->serial_number, cur_dev->path, id, cur_dev->product_id);
    }
#endif

    free(devs_sorted);

    // free HID device enumerations
    for (i = 0; i < NUM_PSMOVE_PIDS; i++) {
        devs = move_hid_devices[i];
        if (devs) {
            hid_free_enumeration(devs);
        }
    }

    return move;
}


PSMove *
psmove_connect()
{
    return psmove_connect_by_id(0);
}

int
_psmove_read_btaddrs(PSMove *move, PSMove_Data_BTAddr *host, PSMove_Data_BTAddr *controller)
{
    unsigned char btg[PSMOVE_MAX_BTADDR_GET_SIZE];
    size_t report_size = PSMOVE_BTADDR_GET_SIZE;
    int res;

    assert(report_size <= sizeof(btg));

    psmove_return_val_if_fail(move != NULL, 0);

#ifdef _WIN32
    // fix Windows quirk for ZCM2
    // (see definition of PSMOVE_WIN32_ZCM2_BTADDR_GET_SIZE)
    if ((move->model == Model_ZCM2) && (move->connection_type == Conn_Bluetooth)) {
        report_size = PSMOVE_WIN32_ZCM2_BTADDR_GET_SIZE;
    }
#endif

    if (move->type == PSMove_MOVED) {
        psmove_CRITICAL("Not implemented in MOVED mode");
        return 0;
    }

    /* Get Bluetooth address */
    memset(btg, 0, report_size);
    btg[0] = PSMove_Req_GetBTAddr;

    /* _WIN32 only has move->handle_addr for getting bluetooth address. */
    if (move->handle_addr) {
        res = hid_get_feature_report(move->handle_addr, btg, sizeof(btg));
    } else {
        res = hid_get_feature_report(move->handle, btg, sizeof(btg));
    }

    if (res == report_size) {
        if (controller != NULL) {
            memcpy(*controller, btg+1, 6);
        }

        char *current_host = _psmove_btaddr_to_string(btg+10);
        psmove_DEBUG("Current host: %s\n", current_host);
        free(current_host);

        if (host != NULL) {
            memcpy(*host, btg+10, 6);
        }

        /* Success! */
        return 1;
    }

    return 0;
}

int
_psmove_get_zcm1_calibration_blob(PSMove *move, char **dest, size_t *size)
{
    psmove_return_val_if_fail(move != NULL, 0);
    psmove_return_val_if_fail(dest != NULL, 0);
    psmove_return_val_if_fail(size != NULL, 0);

    unsigned char calibration[PSMOVE_ZCM1_CALIBRATION_BLOB_SIZE];

    unsigned char cal[PSMOVE_CALIBRATION_SIZE];
    int res;
    int x;

    int dest_offset;
    int src_offset;

    for (x=0; x<3; x++) {
        memset(cal, 0, sizeof(cal));
        cal[0] = PSMove_Req_GetCalibration;
        res = hid_get_feature_report(move->handle, cal, sizeof(cal));
#if defined(__linux)
        if(res == -1) {
            psmove_WARNING("hid_get_feature_report failed, kernel issue? see %s\n",
                "https://github.com/thp/psmoveapi/issues/108");
        }
#endif
        psmove_return_val_if_fail(res == PSMOVE_CALIBRATION_SIZE, 0);

        if (cal[1] == 0x00) {
            /* First block */
            dest_offset = 0;
            src_offset = 0;
        } else if (cal[1] == 0x01) {
            /* Second block */
            dest_offset = PSMOVE_CALIBRATION_SIZE;
            src_offset = 2;
        } else if (cal[1] == 0x82) {
            /* Third block */
            dest_offset = 2*PSMOVE_CALIBRATION_SIZE - 2;
            src_offset = 2;
        } else {
            return 0;
        }

        memcpy(calibration+dest_offset, cal+src_offset,
                sizeof(cal)-src_offset);
    }

    *dest = (char*)malloc(sizeof(calibration));
    memcpy(*dest, calibration, sizeof(calibration));
    *size = sizeof(calibration);

    return 1;
}

int
_psmove_get_zcm2_calibration_blob(PSMove *move, char **dest, size_t *size)
{
    psmove_return_val_if_fail(move != NULL, 0);
    psmove_return_val_if_fail(dest != NULL, 0);
    psmove_return_val_if_fail(size != NULL, 0);

    unsigned char calibration[PSMOVE_ZCM2_CALIBRATION_BLOB_SIZE];

    unsigned char cal[PSMOVE_CALIBRATION_SIZE];
    int res;
    int x;

    int dest_offset;
    int src_offset;

    for (x=0; x<2; x++) {
        memset(cal, 0, sizeof(cal));
        cal[0] = PSMove_Req_GetCalibration;
        res = hid_get_feature_report(move->handle, cal, sizeof(cal));
#if defined(__linux)
        if(res == -1) {
            psmove_WARNING("hid_get_feature_report failed, kernel issue? see %s\n",
                "https://github.com/thp/psmoveapi/issues/108");
        }
#endif
        psmove_return_val_if_fail(res == PSMOVE_CALIBRATION_SIZE, 0);

        if (cal[1] == 0x00) {
            /* First block */
            dest_offset = 0;
            src_offset = 0;
        } else if (cal[1] == 0x81) {
            /* Second block */
            dest_offset = PSMOVE_CALIBRATION_SIZE;
            src_offset = 2;
        } else {
            return 0;
        }

        memcpy(calibration+dest_offset, cal+src_offset,
                sizeof(cal)-src_offset);
    }

    *dest = (char*)malloc(sizeof(calibration));
    memcpy(*dest, calibration, sizeof(calibration));
    *size = sizeof(calibration);

    return 1;
}

char *
psmove_get_serial(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, NULL);
    psmove_return_val_if_fail(move->serial_number != NULL, NULL);

    if (strlen(move->serial_number) == 0) {
        PSMove_Data_BTAddr btaddr;
        if (!_psmove_read_btaddrs(move, NULL, &btaddr)) {
            return NULL;
        }

        return _psmove_btaddr_to_string(btaddr);
    }

    return strdup(move->serial_number);
}

enum PSMove_Model_Type
psmove_get_model(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, Model_ZCM1);
    return move->model;
}

int
_psmove_set_btaddr(PSMove *move, PSMove_Data_BTAddr *addr)
{
    unsigned char bts[PSMOVE_BTADDR_SET_SIZE];
    int res;
    int i;

    psmove_return_val_if_fail(move != NULL, 0);
    psmove_return_val_if_fail(addr != NULL, 0);

    /* Get calibration data */
    memset(bts, 0, sizeof(bts));
    bts[0] = PSMove_Req_SetBTAddr;

    /* Copy 6 bytes from addr into bts[1]..bts[6] */
    for (i=0; i<6; i++) {
        bts[1+i] = (*addr)[i];
    }
    /* _WIN32 only has move->handle_addr for getting bluetooth address. */
    if (move->handle_addr) {
        res = hid_send_feature_report(move->handle_addr, bts, sizeof(bts));
    } else {
        res = hid_send_feature_report(move->handle, bts, sizeof(bts));
    }

    return (res == sizeof(bts));
}

enum PSMove_Bool
psmove_pair(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, PSMove_False);

    PSMove_Data_BTAddr btaddr;
    PSMove_Data_BTAddr current_host;

    if (!_psmove_read_btaddrs(move, &current_host, NULL)) {
        return PSMove_False;
    }

    char *host = psmove_port_get_host_bluetooth_address();
    if (host == NULL) {
        fprintf(stderr, "WARNING: Can't determine Bluetooth address.\n"
                "Make sure Bluetooth is turned on.\n");
    }

    psmove_return_val_if_fail(host != NULL, PSMove_False);
    if (!_psmove_btaddr_from_string(host, &btaddr)) {
        return PSMove_False;
    }

    if (memcmp(current_host, btaddr, sizeof(PSMove_Data_BTAddr)) != 0) {
        if (!_psmove_set_btaddr(move, &btaddr)) {
            return PSMove_False;
        }
    } else {
        psmove_DEBUG("Already paired.\n");
    }

    char *addr = psmove_get_serial(move);

    enum PSMove_Bool result = psmove_port_register_psmove(addr, host, move->model);

    free(addr);
    free(host);

    return result;
}

enum PSMove_Bool
psmove_host_pair_custom(const char *addr)
{
    // NOTE: We assume Move Motion controller model ZCM1 to be compatible with
    //       earlier version of the library that only supported that model.
    return psmove_host_pair_custom_model(addr, Model_ZCM1);
}

enum PSMove_Bool
psmove_host_pair_custom_model(const char *addr, enum PSMove_Model_Type model)
{
    char *host = psmove_port_get_host_bluetooth_address();

    psmove_return_val_if_fail(host != NULL, PSMove_False);

    enum PSMove_Bool result = psmove_port_register_psmove(addr, host, model);

    free(host);

    return result;
}

enum PSMove_Bool
psmove_pair_custom(PSMove *move, const char *new_host_string)
{
    psmove_return_val_if_fail(move != NULL, 0);

    PSMove_Data_BTAddr new_host;
    PSMove_Data_BTAddr current_host;

    if (!_psmove_read_btaddrs(move, &current_host, NULL)) {
        return PSMove_False;
    }

    if (!_psmove_btaddr_from_string(new_host_string, &new_host)) {
        return PSMove_False;
    }

    if (memcmp(current_host, new_host, sizeof(PSMove_Data_BTAddr)) != 0) {
        if (!_psmove_set_btaddr(move, &new_host)) {
            return PSMove_False;
        }
    } else {
        psmove_DEBUG("Already paired.\n");
    }

    char *addr = psmove_get_serial(move);
    char *host = _psmove_btaddr_to_string(new_host);

    enum PSMove_Bool result = psmove_port_register_psmove(addr, host, move->model);

    free(addr);
    free(host);

    return result;
}

enum PSMove_Connection_Type
psmove_connection_type(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, Conn_Unknown);

    return move->connection_type;
}

int
_psmove_btaddr_from_string(const char *string, PSMove_Data_BTAddr *dest)
{
    int i, value;
    PSMove_Data_BTAddr tmp = {0};

    /* strlen("00:11:22:33:44:55") == 17 */
    psmove_return_val_if_fail(strlen(string) == 17, 0);

    for (i=0; i<6; i++) {
        value = strtol(string + i*3, NULL, 16);
        psmove_return_val_if_fail(value >= 0x00 && value <= 0xFF, 0);
        tmp[5-i] = (unsigned char)value;
    }

    if (dest != NULL) {
        memcpy(*dest, tmp, sizeof(PSMove_Data_BTAddr));
    }

    return 1;
}

char *
_psmove_btaddr_to_string(const PSMove_Data_BTAddr addr)
{
    int size = 18; /* strlen("aa:bb:cc:dd:ee:ff") + 1 */
    char *result = (char*)malloc(size);

    snprintf(result, size, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char) addr[5], (unsigned char) addr[4],
            (unsigned char) addr[3], (unsigned char) addr[2],
            (unsigned char) addr[1], (unsigned char) addr[0]);

    return result;
}

void
psmove_set_leds(PSMove *move, unsigned char r, unsigned char g,
        unsigned char b)
{
    psmove_return_if_fail(move != NULL);

    if (move->leds.r == r && move->leds.g == g && move->leds.b == b) {
        /**
         * Avoid extraneous LED updates - this does not set the "leds_dirty"
         * flag below, so we don't have to send an update the next time.
         **/
        return;
    }

    move->leds.r = r;
    move->leds.g = g;
    move->leds.b = b;
    move->leds_dirty = 1;
}

enum PSMove_Bool
psmove_set_led_pwm_frequency(PSMove *move, unsigned long freq)
{
    unsigned char buf[7];
    int res;

    psmove_return_val_if_fail(move != NULL, PSMove_False);

    if (freq < 733 || freq > 24e6) {
        psmove_WARNING("Frequency can only assume values between 733 and 24e6.");
        return PSMove_False;
    }

    memset(buf, 0, sizeof(buf));
    buf[0] = PSMove_Req_SetLEDPWMFrequency;
    buf[1] = 0x41;  /* magic value, report is ignored otherwise */
    buf[2] = 0;     /* command byte, values 1..4 are internal frequency presets */
    /* The 32-bit frequency value must be stored in Little-Endian byte order */
    buf[3] =  freq        & 0xFF;
    buf[4] = (freq >>  8) & 0xFF;
    buf[5] = (freq >> 16) & 0xFF;
    buf[6] = (freq >> 24) & 0xFF;

    res = hid_send_feature_report(move->handle, buf, sizeof(buf));

    return (res == sizeof(buf));
}

void
psmove_set_rumble(PSMove *move, unsigned char rumble)
{
    psmove_return_if_fail(move != NULL);

    if (move->leds.rumble == rumble) {
        /* Avoid extraneous updates (see psmove_set_leds above) */
        return;
    }

    move->leds.rumble2 = 0x00;
    move->leds.rumble = rumble;
    move->leds_dirty = 1;
}

enum PSMove_Update_Result
psmove_update_leds(PSMove *move)
{
    long timediff_ms;

    psmove_return_val_if_fail(move != NULL, 0);

    timediff_ms = (psmove_util_get_ticks() - move->last_leds_update);

    if (move->leds_rate_limiting &&
            move->leds_dirty && timediff_ms < PSMOVE_MIN_LED_UPDATE_WAIT_MS) {
        /* Rate limiting (too many updates) */
        return Update_Ignored;
    } else if (!move->leds_dirty && timediff_ms < PSMOVE_MAX_LED_INHIBIT_MS) {
        /* Unchanged LEDs value (no need to update yet) */
        return Update_Ignored;
    }

    move->leds_dirty = 0;
    move->last_leds_update = psmove_util_get_ticks();

    switch (move->type) {
        case PSMove_HIDAPI:
        	// NOTE: On Linux, hid_write returns 0 for a Bluetooth-connected
        	//       controller, but not for USB. On Windows, hid_write always
        	//       returns the size of the longest HID report which, in this
        	//       case, does not match the output report size. And Mac
        	//       probably adds another behaviour altogether.
        	//
        	//       So instead of handling every different case here, we just
        	//       settle for the lowest common denominator and flag write
        	//       errors.

            if (hid_write(move->handle, (unsigned char*)(&(move->leds)),
                    sizeof(move->leds)) >= 0) {
                return Update_Success;
            } else {
                return Update_Failed;
            }
            break;
        case PSMove_MOVED:
            if (moved_client_send(move->client, MOVED_REQ_SET_LEDS, move->remote_id, (uint8_t *)&move->leds, sizeof(move->leds))) {
                return Update_Success;
            } else {
                return Update_Failed;
            }
            break;
        default:
            psmove_CRITICAL("Unknown device type");
            return 0;
            break;
    }
}

void
psmove_set_rate_limiting(PSMove *move, enum PSMove_Bool enabled)
{
    psmove_return_if_fail(move != NULL);
    move->leds_rate_limiting = enabled;
}

int
psmove_poll(PSMove *move)
{
    int res = 0;

    psmove_return_val_if_fail(move != NULL, 0);

    /* store old sequence number before reading */
    int oldseq = (move->input.common.buttons4 & 0x0F);

    switch (move->type) {
        case PSMove_HIDAPI:
            switch (move->model) {
                case Model_ZCM1:
                    res = hid_read(move->handle, (unsigned char*)(&(move->input.zcm1)), sizeof(move->input.zcm1));
                    break;
                case Model_ZCM2:
                    res = hid_read(move->handle, (unsigned char*)(&(move->input.zcm2)), sizeof(move->input.zcm2));
                    break;
                default:
                    psmove_CRITICAL("Unknown PS Move model");
                    break;
            }
            break;
        case PSMove_MOVED:
            if (moved_client_send(move->client, MOVED_REQ_READ_INPUT, (char)move->remote_id, NULL, 0)) {
                /**
                 * The input buffer is stored at offset 1 (the first byte
                 * contains the return value of the remote psmove_poll())
                 **/

                size_t input_data_size = 0;
                switch (move->model) {
                    case Model_ZCM1:
                        input_data_size = sizeof(move->input.zcm1);
                        memcpy(&(move->input.zcm1), move->client->response_buf.read_input.data, input_data_size);
                        break;
                    case Model_ZCM2:
                        input_data_size = sizeof(move->input.zcm2);
                        memcpy(&(move->input.zcm2), move->client->response_buf.read_input.data, input_data_size);
                        break;
                    default:
                        psmove_CRITICAL("Unknown PS Move model");
                        break;
                }

                if (move->client->response_buf.read_input.poll_return_value != 0) {
                    res = input_data_size;
                }
            }
            break;
        default:
            psmove_CRITICAL("Unknown device type");
    }

    if ((move->model == Model_ZCM1 && res == sizeof(move->input.zcm1)) ||
        (move->model == Model_ZCM2 && res == sizeof(move->input.zcm2))) {
        /* Sanity check: The first byte should be PSMove_Req_GetInput */
        psmove_return_val_if_fail(move->input.common.type == PSMove_Req_GetInput, 0);

        /**
         * buttons4's 4 least significant bits contain the sequence number,
         * so we add 1 to signal "success" and add the sequence number for
         * consumers to utilize the data
         **/
        int seq = (move->input.common.buttons4 & 0x0F);
        if (seq != ((oldseq + 1) % 16)) {
            psmove_DEBUG("Warning: Dropped frames (seq %d -> %d)\n",
                    oldseq, seq);
        }

        if (move->orientation_enabled) {
            psmove_orientation_update(move->orientation);
        }

        return 1 + seq;
    }

    return 0;
}

enum PSMove_Bool
psmove_get_ext_data(PSMove *move, PSMove_Ext_Data *data)
{
    psmove_return_val_if_fail(move != NULL, PSMove_False);
    psmove_return_val_if_fail(data != NULL, PSMove_False);

    switch (move->model) {
        case Model_ZCM1:
            assert(sizeof(*data) >= sizeof(move->input.zcm1.extdata));
            memcpy(data, move->input.zcm1.extdata, sizeof(move->input.zcm1.extdata));
            return PSMove_True;
        case Model_ZCM2:
            // EXT data not supported on ZCM2
            return PSMove_False;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            return PSMove_False;
    }
}

unsigned int
psmove_get_buttons(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);

    /**
     * See also: enum PSMove_Button in include/psmove.h
     *
     * Source:
     * https://github.com/nitsch/moveonpc/wiki/Input-report
     *
     *           21  1    1  00000   0 <- bit (read top to bottom)
     *   ........09..6....1..87654...0
     *           ^^  ^    ^  ^^^^^
     *           ||  |    |  |||||
     *           ||  |    |  |22222222 <- input report byte 2
     *           ||  |11111111         <- input report byte 1
     *           ||  3                 <- bit 0 of input report byte 3
     *           44                    <- bits 6-7 of input report byte 4
     *
     * Input report byte 1:
     *  xxxx4xx0
     *  ^^^^-^^- L3 (bit 1), R3 (bit 2), UP (bit 4), RIGHT (bit 5),
     *           DOWN (bit 6), LEFT (bit 7) on sixaxis (not exposed yet)
     *         ^- select
     *      ^- start
     *
     * Input report byte 2:
     *  7654xxxx
     *      ^^^^- L2 (bit 0), R2 (bit 1), L1 (bit 2),
     *            R1 (bit 3) on sixaxis (not exposed yet)
     *     ^- triangle
     *    ^- circle
     *   ^- cross
     *  ^- square
     *
     * Input report byte 3:
     *  xxxxxxx0
     *         ^- ps button
     *
     * Input report byte 4:
     *  76xxxxxx
     *      ^^^^- input sequence number (see psmove_poll())
     *   ^- move
     *  ^- trigger
     *
     **/

    return ((move->input.common.buttons2) |
            (move->input.common.buttons1 << 8) |
            ((move->input.common.buttons3 & 0x01) << 16) |
            ((move->input.common.buttons4 & 0xF0) << 13 /* 13 = 17 - 4 */));
}

void
psmove_get_button_events(PSMove *move, unsigned int *pressed,
        unsigned int *released)
{
    psmove_return_if_fail(move != NULL);

    unsigned int buttons = psmove_get_buttons(move);

    if (pressed) {
        *pressed = buttons & ~(move->last_buttons);
    }

    if (released) {
        *released = move->last_buttons & ~buttons;
    }

    move->last_buttons = buttons;
}

enum PSMove_Bool
psmove_is_ext_connected(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, PSMove_False);

    if (move->model != Model_ZCM1) {
        return PSMove_False;
    }

    if ((move->input.common.buttons4 & 0x10) != 0) {
        return PSMove_True;
    }

    return PSMove_False;
}

enum PSMove_Bool
psmove_get_ext_device_info(PSMove *move, PSMove_Ext_Device_Info *ext)
{
    unsigned char send_buf[PSMOVE_EXT_DEVICE_REPORT_SIZE];
    unsigned char recv_buf[PSMOVE_EXT_DEVICE_REPORT_SIZE];
    int res;

    psmove_return_val_if_fail(move != NULL, PSMove_False);
    psmove_return_val_if_fail(ext != NULL, PSMove_False);

    if (move->model != Model_ZCM1) {
        return PSMove_False;
    }

    /* Send setup Report for the following read operation */
    memset(send_buf, 0, sizeof(send_buf));
    send_buf[0] = PSMove_Req_SetExtDeviceInfo;
    send_buf[1] = 1;    /* read flag */
    send_buf[2] = 0xA0; /* target extension device's I²C slave address */
    send_buf[3] = 0;    /* offset for retrieving data */
    send_buf[4] = 0xFF; /* number of bytes to retrieve */
    res = hid_send_feature_report(move->handle, send_buf, sizeof(send_buf));

    if (res != sizeof(send_buf)) {
        psmove_DEBUG("Sending Feature Report for read setup failed\n");
        return PSMove_False;
    }

    /* Send actual read Report */
    memset(recv_buf, 0, sizeof(recv_buf));
    recv_buf[0] = PSMove_Req_GetExtDeviceInfo;
    res = hid_get_feature_report(move->handle, recv_buf, sizeof(recv_buf));

    if (res != sizeof(recv_buf)) {
        psmove_DEBUG("Sending Feature Report for actual read failed\n");
        return PSMove_False;
    }

    memset(ext, 0, sizeof(PSMove_Ext_Device_Info));

    /* Copy extension device ID */
    ext->dev_id = (recv_buf[9] << 8) | recv_buf[10];

    /* Copy device info following the device ID into EXT info struct */
    assert(sizeof(ext->dev_info) <= sizeof(recv_buf) - 11);
    memcpy(ext->dev_info, recv_buf + 11, sizeof(ext->dev_info));

    return PSMove_True;
}

enum PSMove_Bool
psmove_send_ext_data(PSMove *move, const unsigned char *data, unsigned char length)
{
    unsigned char send_buf[PSMOVE_EXT_DEVICE_REPORT_SIZE];
    int res;

    psmove_return_val_if_fail(move != NULL, PSMove_False);
    psmove_return_val_if_fail(data != NULL, PSMove_False);
    psmove_return_val_if_fail(length > 0,   PSMove_False);

    if (move->model != Model_ZCM1) {
        return PSMove_False;
    }

    if (length > sizeof(send_buf) - 9) {
        psmove_DEBUG("Data too large for send buffer\n");
        return PSMove_False;
    }

    /* Send Feature Report */
    memset(send_buf, 0, sizeof(send_buf));
    send_buf[0] = PSMove_Req_SetExtDeviceInfo;
    send_buf[1] = 0;          /* read flag */
    send_buf[2] = 0xA0;       /* target extension device's I²C slave address */
    send_buf[3] = data[0];    /* control byte */
    send_buf[4] = length - 1; /* payload size */
    memcpy(send_buf + 9, data + 1, length - 1);
    res = hid_send_feature_report(move->handle, send_buf, sizeof(send_buf));

    if (res != sizeof(send_buf)) {
        psmove_DEBUG("Sending Feature Report failed\n");
        return PSMove_False;
    }

    return PSMove_True;
}

enum PSMove_Battery_Level
psmove_get_battery(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);

    return move->input.common.battery;
}

int
psmove_get_temperature(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);

    /**
     * On the Move Motion Controller's PCB there is a voltage divider which
     * contains a thermistor. An ADC reads the voltage across the thermistor
     * (which changes its resistance with the temperature) and reports the
     * raw value (plus an offset) as the value we see in the Input Report.
     *
     * The offset can be changed if the controller is running in Debug mode,
     * but it seems to default to 0.
     **/

    return ((move->input.common.temphigh << 4) |
            ((move->input.common.templow_mXhigh & 0xF0) >> 4));
}

float
_psmove_temperature_to_celsius(int raw_value)
{
    /**
     * The Move uses this table in Debug mode. Even though the resulting values
     * are not labeled "degree Celsius" in the Debug output, measurements
     * indicate that it is close enough.
     **/
    static int const temperature_lookup[80] = {
        0x1F6, 0x211, 0x22C, 0x249, 0x266, 0x284, 0x2A4, 0x2C4,
        0x2E5, 0x308, 0x32B, 0x34F, 0x374, 0x399, 0x3C0, 0x3E8,
        0x410, 0x439, 0x463, 0x48D, 0x4B8, 0x4E4, 0x510, 0x53D,
        0x56A, 0x598, 0x5C6, 0x5F4, 0x623, 0x651, 0x680, 0x6AF,
        0x6DE, 0x70D, 0x73C, 0x76B, 0x79A, 0x7C9, 0x7F7, 0x825,
        0x853, 0x880, 0x8AD, 0x8D9, 0x905, 0x930, 0x95B, 0x985,
        0x9AF, 0x9D8, 0xA00, 0xA28, 0xA4F, 0xA75, 0xA9B, 0xAC0,
        0xAE4, 0xB07, 0xB2A, 0xB4B, 0xB6D, 0xB8D, 0xBAD, 0xBCB,
        0xBEA, 0xC07, 0xC24, 0xC40, 0xC5B, 0xC75, 0xC8F, 0xCA8,
        0xCC1, 0xCD8, 0xCF0, 0xD06, 0xD1C, 0xD31, 0xD46, 0xD5A,
    };

    int i;

    for (i = 0; i < 80; i++) {
        if (temperature_lookup[i] > raw_value) {
            return (float)(i - 10);
        }
    }

    return 70.0f;
}

float
psmove_get_temperature_in_celsius(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0.0);

    int raw_value = psmove_get_temperature(move);

    return _psmove_temperature_to_celsius(raw_value);
}

unsigned char
psmove_get_trigger(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);

    switch (move->model) {
        case Model_ZCM1:
            return (move->input.common.trigger + move->input.common.trigger2) / 2;
        case Model_ZCM2:
            return move->input.common.trigger;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            return 0;
    }
}

struct Vector3Int {
    int x, y, z;
};

void
psmove_get_half_frame(PSMove *move, enum PSMove_Sensor sensor,
        enum PSMove_Frame frame, int *x, int *y, int *z)
{
    psmove_return_if_fail(move != NULL);
    psmove_return_if_fail(sensor == Sensor_Accelerometer || sensor == Sensor_Gyroscope);
    psmove_return_if_fail(frame == Frame_FirstHalf || frame == Frame_SecondHalf);

    struct Vector3Int result = {0, 0, 0};

    int base = 0;

    switch (move->model) {
        case Model_ZCM1:
            switch (sensor) {
                case Sensor_Accelerometer:
                    base = offsetof(PSMove_Data_Input_Common, aXlow);
                    break;
                case Sensor_Gyroscope:
                    base = offsetof(PSMove_Data_Input_Common, gXlow);
                    break;
                default:
                    psmove_WARNING("Unknown sensor type");
                    return;
            }

            if (frame == Frame_SecondHalf) {
                base += 6;
            }

            result.x = psmove_decode_16bit((void*)&move->input.common, base + 0);
            result.y = psmove_decode_16bit((void*)&move->input.common, base + 2);
            result.z = psmove_decode_16bit((void*)&move->input.common, base + 4);
            break;
        case Model_ZCM2:
            switch (sensor) {
                case Sensor_Accelerometer:
                    base = offsetof(PSMove_Data_Input_Common, aXlow);
                    break;
                case Sensor_Gyroscope:
                    base = offsetof(PSMove_Data_Input_Common, gXlow);
                    break;
                default:
                    psmove_WARNING("Unknown sensor type");
                    return;
            }

            // NOTE: Only one frame on the ZCM2

            result.x = psmove_decode_16bit_twos_complement((void*)&move->input.common, base + 0);
            result.y = psmove_decode_16bit_twos_complement((void*)&move->input.common, base + 2);
            result.z = psmove_decode_16bit_twos_complement((void*)&move->input.common, base + 4);
            break;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            break;
    }

    if (x) {
        *x = result.x;
    }

    if (y) {
        *y = result.y;
    }

    if (z) {
        *z = result.z;
    }
}

void
psmove_get_accelerometer(PSMove *move, int *ax, int *ay, int *az)
{
    psmove_return_if_fail(move != NULL);

    struct Vector3Int result = {0, 0, 0};

    switch (move->model) {
        case Model_ZCM1:
            result.x = ((move->input.common.aXlow + move->input.common.aXlow2) +
                   ((move->input.common.aXhigh + move->input.common.aXhigh2) << 8)) / 2 - 0x8000;

            result.y = ((move->input.common.aYlow + move->input.common.aYlow2) +
                   ((move->input.common.aYhigh + move->input.common.aYhigh2) << 8)) / 2 - 0x8000;

            result.z = ((move->input.common.aZlow + move->input.common.aZlow2) +
                   ((move->input.common.aZhigh + move->input.common.aZhigh2) << 8)) / 2 - 0x8000;
            break;
        case Model_ZCM2:
            result.x = (int16_t) (move->input.common.aXlow + (move->input.common.aXhigh << 8));
            result.y = (int16_t) (move->input.common.aYlow + (move->input.common.aYhigh << 8));
            result.z = (int16_t) (move->input.common.aZlow + (move->input.common.aZhigh << 8));
            break;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            break;
    }

    if (ax) {
        *ax = result.x;
    }

    if (ay) {
        *ay = result.y;
    }

    if (az) {
        *az = result.z;
    }
}

void
psmove_get_gyroscope(PSMove *move, int *gx, int *gy, int *gz)
{
    psmove_return_if_fail(move != NULL);

    struct Vector3Int result = {0, 0, 0};

    switch (move->model) {
        case Model_ZCM1:
            result.x = ((move->input.common.gXlow + move->input.common.gXlow2) +
                   ((move->input.common.gXhigh + move->input.common.gXhigh2) << 8)) / 2 - 0x8000;

            result.y = ((move->input.common.gYlow + move->input.common.gYlow2) +
                   ((move->input.common.gYhigh + move->input.common.gYhigh2) << 8)) / 2 - 0x8000;

            result.z = ((move->input.common.gZlow + move->input.common.gZlow2) +
                   ((move->input.common.gZhigh + move->input.common.gZhigh2) << 8)) / 2 - 0x8000;
            break;
        case Model_ZCM2:
            result.x = (int16_t) (move->input.common.gXlow + (move->input.common.gXhigh << 8));
            result.y = (int16_t) (move->input.common.gYlow + (move->input.common.gYhigh << 8));
            result.z = (int16_t) (move->input.common.gZlow + (move->input.common.gZhigh << 8));
            break;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            break;
    }

    if (gx) {
        *gx = result.x;
    }

    if (gy) {
        *gy = result.y;
    }

    if (gz) {
        *gz = result.z;
    }
}

void
psmove_get_accelerometer_frame(PSMove *move, enum PSMove_Frame frame,
        float *ax, float *ay, float *az)
{
    int raw_input[3];

    psmove_return_if_fail(move != NULL);
    psmove_return_if_fail(frame == Frame_FirstHalf ||
            frame == Frame_SecondHalf);

    psmove_get_half_frame(move, Sensor_Accelerometer, frame,
            raw_input, raw_input+1, raw_input+2);

    psmove_calibration_map_accelerometer(move->calibration, raw_input,
            ax, ay, az);
}

void
psmove_get_gyroscope_frame(PSMove *move, enum PSMove_Frame frame,
        float *gx, float *gy, float *gz)
{
    int raw_input[3];

    psmove_return_if_fail(move != NULL);
    psmove_return_if_fail(frame == Frame_FirstHalf ||
            frame == Frame_SecondHalf);

    psmove_get_half_frame(move, Sensor_Gyroscope, frame,
            raw_input, raw_input+1, raw_input+2);

    psmove_calibration_map_gyroscope(move->calibration, raw_input,
            gx, gy, gz);
}

void
psmove_get_magnetometer_3axisvector(PSMove *move, PSMove_3AxisVector *out_m)
{
    psmove_return_if_fail(move != NULL);

	PSMove_3AxisVector m;
	int mx, my, mz;
    psmove_get_magnetometer(move, &mx, &my, &mz);
	m= psmove_3axisvector_xyz((float)mx, (float)my, (float)mz);

    /* Update minimum and max components */
	move->magnetometer_min = psmove_3axisvector_min_vector(&move->magnetometer_min, &m);
	move->magnetometer_max = psmove_3axisvector_max_vector(&move->magnetometer_max, &m);

    /* Map [min..max] to [-1..+1] */
	if (out_m)
	{
		PSMove_3AxisVector range = psmove_3axisvector_subtract(&move->magnetometer_max, &move->magnetometer_min);
		PSMove_3AxisVector offset = psmove_3axisvector_subtract(&m, &move->magnetometer_min);
		
		// 2*(raw-move->magnetometer_min)/(move->magnetometer_max - move->magnetometer_min) - <1,1,1>
		*out_m = psmove_3axisvector_divide_by_vector_with_default(&offset, &range, k_psmove_vector_zero);
		*out_m = psmove_3axisvector_scale(out_m, 2.f);
		*out_m = psmove_3axisvector_subtract(out_m, k_psmove_vector_one);

		// The magnetometer y-axis is flipped compared to the accelerometer and gyro.
		// Flip it back around to get it into the same space.
		out_m->y = -out_m->y;	
	}
}

void
psmove_get_magnetometer_vector(PSMove *move,
        float *mx, float *my, float *mz)
{
	psmove_return_if_fail(move != NULL);

	PSMove_3AxisVector m;

	psmove_get_magnetometer_3axisvector(move, &m);
	
	if (mx)
	{
		*mx= m.x;
	}
	
	if (my)
	{
		*my= m.y;
	}

	if (mz)
	{
		*mz= m.z;
	}
}

enum PSMove_Bool
psmove_has_calibration(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);
    return psmove_calibration_supported(move->calibration);
}

void
psmove_dump_calibration(PSMove *move)
{
    psmove_return_if_fail(move != NULL);
    psmove_calibration_dump(move->calibration);
}


void
psmove_get_magnetometer(PSMove *move, int *mx, int *my, int *mz)
{
    psmove_return_if_fail(move != NULL);

    struct Vector3Int result = {0, 0, 0};

    switch (move->model) {
        case Model_ZCM1:
            result.x = TWELVE_BIT_SIGNED(((move->input.common.templow_mXhigh & 0x0F) << 8) |
                    move->input.zcm1.mXlow);

            result.y = TWELVE_BIT_SIGNED((move->input.zcm1.mYhigh << 4) |
                   (move->input.zcm1.mYlow_mZhigh & 0xF0) >> 4);

            result.z = TWELVE_BIT_SIGNED(((move->input.zcm1.mYlow_mZhigh & 0x0F) << 8) |
                    move->input.zcm1.mZlow);
            break;
        case Model_ZCM2:
            // NOTE: This model does not have magnetometers
            break;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            break;
    }

    if (mx) {
        *mx = result.x;
    }

    if (my) {
        *my = result.y;
    }

    if (mz) {
        *mz = result.z;
    }
}


void
psmove_enable_orientation(PSMove *move, enum PSMove_Bool enabled)
{
    psmove_return_if_fail(move != NULL);

    // Reset orientation to current controller position
    psmove_reset_orientation(move);

    move->orientation_enabled = enabled;
}

enum PSMove_Bool
psmove_has_orientation(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, PSMove_False);
    psmove_return_val_if_fail(move->orientation != NULL, PSMove_False);

    return move->orientation_enabled;
}

void
psmove_get_orientation(PSMove *move,
        float *w, float *x, float *y, float *z)
{
    psmove_return_if_fail(move != NULL);
    psmove_return_if_fail(move->orientation != NULL);

    psmove_orientation_get_quaternion(move->orientation, w, x, y, z);
}

void
psmove_reset_orientation(PSMove *move)
{
    psmove_return_if_fail(move != NULL);
    psmove_return_if_fail(move->orientation != NULL);

    psmove_orientation_reset_quaternion(move->orientation);
}

void
psmove_reset_magnetometer_calibration(PSMove *move)
{
    psmove_return_if_fail(move != NULL);

	move->magnetometer_calibration_direction = *k_psmove_vector_zero;
    move->magnetometer_min.x =
        move->magnetometer_min.y =
        move->magnetometer_min.z = FLT_MAX;
    move->magnetometer_max.x =
        move->magnetometer_max.y =
        move->magnetometer_max.z = FLT_MIN;
}

char *
psmove_get_magnetometer_calibration_filename(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, NULL);

    char filename[PATH_MAX];

    char *serial = psmove_get_serial(move);
    psmove_return_val_if_fail(serial != NULL, NULL);

    int i;
    for (i=0; i<strlen(serial); i++) {
        if (serial[i] == ':') {
            serial[i] = '_';
        }
    }
    snprintf(filename, PATH_MAX, "%s.magnetometer.csv", serial);
    free(serial);

    char *filepath = psmove_util_get_file_path(filename);
    return filepath;
}

void
psmove_save_magnetometer_calibration(PSMove *move)
{
    psmove_return_if_fail(move != NULL);
    char *filename = psmove_get_magnetometer_calibration_filename(move);
    FILE *fp = fopen(filename, "w");
    free(filename);
    psmove_return_if_fail(fp != NULL);

	fprintf(fp, "mx,my,mz\n");
	fprintf(fp, "%f,%f,%f\n", 
		move->magnetometer_calibration_direction.x,
		move->magnetometer_calibration_direction.y, 
		move->magnetometer_calibration_direction.z);
    fprintf(fp, "axis,min,max\n");
    fprintf(fp, "x,%f,%f\n", move->magnetometer_min.x, move->magnetometer_max.x);
    fprintf(fp, "y,%f,%f\n", move->magnetometer_min.y, move->magnetometer_max.y);
    fprintf(fp, "z,%f,%f\n", move->magnetometer_min.z, move->magnetometer_max.z);

    fclose(fp);
}

enum PSMove_Bool
psmove_load_magnetometer_calibration(PSMove *move)
{
	enum PSMove_Bool success = PSMove_False;
    
	if (move == NULL) {
        return success;
    }

    psmove_reset_magnetometer_calibration(move);
    char *filename = psmove_get_magnetometer_calibration_filename(move);
    FILE *fp = fopen(filename, "r");
    free(filename);

	if (fp == NULL) {
        char *addr = psmove_get_serial(move);
        psmove_WARNING("Magnetometer in %s not yet calibrated.\n", addr);
        free(addr);
		goto finish;
    }

	char s_mx[3], s_my[3], s_mz[3];
	float mx, my, mz;
	char s_axis[5], s_min[4], s_max[4];
    char c_axis;
    float f_min, f_max;
    int result;

	result = fscanf(fp, "%2s,%2s,%2s\n", s_mx, s_my, s_mz);
	psmove_goto_if_fail(result == 3, finish);
	psmove_goto_if_fail(strcmp(s_mx, "mx") == 0, finish);
	psmove_goto_if_fail(strcmp(s_my, "my") == 0, finish);
	psmove_goto_if_fail(strcmp(s_mz, "mz") == 0, finish);

	result = fscanf(fp, "%f,%f,%f\n", &mx, &my, &mz);
	psmove_goto_if_fail(result == 3, finish);
	move->magnetometer_calibration_direction = psmove_3axisvector_xyz(mx, my, mz);

	result = fscanf(fp, "%4s,%3s,%3s\n", s_axis, s_min, s_max);
    psmove_goto_if_fail(result == 3, finish);
    psmove_goto_if_fail(strcmp(s_axis, "axis") == 0, finish);
    psmove_goto_if_fail(strcmp(s_min, "min") == 0, finish);
    psmove_goto_if_fail(strcmp(s_max, "max") == 0, finish);

    result = fscanf(fp, "%c,%f,%f\n", &c_axis, &f_min, &f_max);
    psmove_goto_if_fail(result == 3, finish);
    psmove_goto_if_fail(c_axis == 'x', finish);
    move->magnetometer_min.x = f_min;
    move->magnetometer_max.x = f_max;

    result = fscanf(fp, "%c,%f,%f\n", &c_axis, &f_min, &f_max);
    psmove_goto_if_fail(result == 3, finish);
    psmove_goto_if_fail(c_axis == 'y', finish);
    move->magnetometer_min.y = f_min;
    move->magnetometer_max.y = f_max;

    result = fscanf(fp, "%c,%f,%f\n", &c_axis, &f_min, &f_max);
    psmove_goto_if_fail(result == 3, finish);
    psmove_goto_if_fail(c_axis == 'z', finish);
    move->magnetometer_min.z = f_min;
    move->magnetometer_max.z = f_max;

	success = PSMove_True;

finish:
	if (success == PSMove_False)
	{
		psmove_reset_magnetometer_calibration(move);
	}
    
    if (fp != NULL)
    {
        fclose(fp);
    }

	return success;
}

float
psmove_get_magnetometer_calibration_range(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);

	PSMove_3AxisVector diff = psmove_3axisvector_subtract(&move->magnetometer_max, &move->magnetometer_min);

    return psmove_3axisvector_min_component(&diff);
}

void
psmove_set_orientation_fusion_type(PSMove *move, enum PSMoveOrientation_Fusion_Type fusion_type)
{
    psmove_return_if_fail(move != NULL);
    psmove_return_if_fail(move->orientation != NULL);

	psmove_orientation_set_fusion_type(move->orientation, fusion_type);
}

void
psmove_set_calibration_pose(PSMove *move, enum PSMove_CalibrationPose_Type calibration_pose)
{
    switch (calibration_pose)
    {
    case CalibrationPose_Upright:
        psmove_set_calibration_transform(move, k_psmove_identity_pose_upright);
        break;
    case CalibrationPose_LyingFlat:
        psmove_set_calibration_transform(move, k_psmove_identity_pose_laying_flat);
        break;
    }
}

void
psmove_set_calibration_transform(PSMove *move, const PSMove_3AxisTransform *transform)
{
    psmove_return_if_fail(move != NULL);
    psmove_return_if_fail(move->orientation != NULL);

	psmove_orientation_set_calibration_transform(move->orientation, transform);
}

void
psmove_get_identity_gravity_calibration_direction(PSMove *move, PSMove_3AxisVector *out_a)
{
	*out_a= psmove_3axisvector_xyz(0.f, 1.f, 0.f);
}

void
psmove_get_transformed_gravity_calibration_direction(PSMove *move, PSMove_3AxisVector *out_a)
{
	if (move != NULL)
	{
		*out_a= psmove_orientation_get_gravity_calibration_direction(move->orientation);
	}
	else
	{
		psmove_get_identity_gravity_calibration_direction(move, out_a);
	}
}

void
psmove_get_identity_magnetometer_calibration_direction(PSMove *move, PSMove_3AxisVector *out_m)
{
	psmove_return_if_fail(move != NULL);
	psmove_return_if_fail(out_m != NULL);

	*out_m= move->magnetometer_calibration_direction;
}

void
psmove_get_transformed_magnetometer_calibration_direction(PSMove *move, PSMove_3AxisVector *out_m)
{
	if (move != NULL)
	{
		*out_m= psmove_orientation_get_magnetometer_calibration_direction(move->orientation);
	}
	else
	{
		psmove_get_identity_magnetometer_calibration_direction(move, out_m);
	}
}

void
psmove_set_magnetometer_calibration_direction(PSMove *move, PSMove_3AxisVector *m)
{
	psmove_return_if_fail(move != NULL);

	move->magnetometer_calibration_direction = *m;
}

void
psmove_set_sensor_data_basis(PSMove *move, enum PSMove_SensorDataBasis_Type basis_type)
{
    switch (basis_type)
    {
    case SensorDataBasis_Native:
        psmove_set_sensor_data_transform(move, k_psmove_sensor_transform_identity);
        break;
    case SensorDataBasis_OpenGL:
        psmove_set_sensor_data_transform(move, k_psmove_sensor_transform_opengl);
        break;
    }
}

void
psmove_set_sensor_data_transform(PSMove *move, const PSMove_3AxisTransform *transform)
{
    psmove_return_if_fail(move != NULL);
    psmove_return_if_fail(move->orientation != NULL);

	psmove_orientation_set_sensor_data_transform(move->orientation, transform);
}

void
psmove_get_transformed_magnetometer_direction(PSMove *move, PSMove_3AxisVector *out_m)
{
	psmove_return_if_fail(move != NULL);

	*out_m= psmove_orientation_get_magnetometer_normalized_vector(move->orientation);
}

void
psmove_get_transformed_accelerometer_frame_3axisvector(PSMove *move, enum PSMove_Frame frame, PSMove_3AxisVector *out_a)
{
	psmove_return_if_fail(move != NULL);

	*out_a= psmove_orientation_get_accelerometer_vector(move->orientation, frame);
}

void
psmove_get_transformed_accelerometer_frame_direction(PSMove *move, enum PSMove_Frame frame, PSMove_3AxisVector *out_a)
{
	psmove_return_if_fail(move != NULL);

	*out_a= psmove_orientation_get_accelerometer_normalized_vector(move->orientation, frame);
}

void
psmove_get_transformed_gyroscope_frame_3axisvector(PSMove *move, enum PSMove_Frame frame, PSMove_3AxisVector *out_w)
{
	psmove_return_if_fail(move != NULL);

	*out_w= psmove_orientation_get_gyroscope_vector(move->orientation, frame);
}

void
psmove_disconnect(PSMove *move)
{
    psmove_return_if_fail(move != NULL);

    switch (move->type) {
        case PSMove_HIDAPI:
            hid_close(move->handle);
            if (move->handle_addr) {// _WIN32 only
                hid_close(move->handle_addr);
            }
            break;
        case PSMove_MOVED:
            // XXX: Close connection?
            break;
    }

    if (move->orientation) {
        psmove_orientation_free(move->orientation);
    }

    if (move->calibration) {
        psmove_calibration_free(move->calibration);
    }

    free(move->serial_number);
    free(move->device_path);
    if (move->device_path_addr) { // _WIN32 only
        free(move->device_path_addr);
    }
    free(move);

    /* Bookkeeping of open handles (for psmove_reinit) */
    psmove_return_if_fail(psmove_num_open_handles > 0);
    psmove_num_open_handles--;
}

long
psmove_util_get_ticks()
{
    return psmove_port_get_time_ms();
}

const char *
psmove_util_get_data_dir()
{
    static char dir[PATH_MAX];

    if (strlen(dir) == 0) {
        snprintf(dir, sizeof(dir), "%s" PATH_SEP ".psmoveapi", getenv(ENV_USER_HOME));
    }

    return dir;
}

char *
psmove_util_get_file_path(const char *filename)
{
    const char *parent = psmove_util_get_data_dir();
    char *result;
    struct stat st;

#ifndef _WIN32
    // if run as root, use system-wide data directory
    if (geteuid() == 0) {
        parent = PSMOVE_SYSTEM_DATA_DIR;
    }
#endif

    if (stat(filename, &st) == 0) {
        // File exists in the current working directory, prefer that
        // to the file in the default data / configuration directory
        return strdup(filename);
    }

    if (stat(parent, &st) != 0) {
#ifdef _WIN32
        psmove_return_val_if_fail(mkdir(parent) == 0, NULL);
#else
        psmove_return_val_if_fail(mkdir(parent, 0777) == 0, NULL);
#endif
    }

    result = malloc(strlen(parent) + 1 + strlen(filename) + 1);
    strcpy(result, parent);
    strcat(result, PATH_SEP);
    strcat(result, filename);

    return result;
}

char *
psmove_util_get_system_file_path(const char *filename)
{
    char *result;
    size_t len = strlen(PSMOVE_SYSTEM_DATA_DIR) + 1 + strlen(filename) + 1;

    result = malloc(len);
    if (result == NULL) {
        return NULL;
    }

    snprintf(result, len, "%s%s%s", PSMOVE_SYSTEM_DATA_DIR, PATH_SEP, filename);

    return result;
}

int
psmove_util_get_env_int(const char *name)
{
    char *env = getenv(name);

    if (env) {
        char *end;
        long result = strtol(env, &end, 10);

        if (*end == '\0' && *env != '\0') {
            return result;
        }
    }

    return -1;
}

char *
psmove_util_get_env_string(const char *name)
{
    char *env = getenv(name);

    if (env) {
        return strdup(env);
    }

    return NULL;
}

char *
_psmove_normalize_btaddr(const char *addr, int lowercase, char separator)
{
    size_t count = strlen(addr);

    if (count != 17) {
        psmove_WARNING("Invalid address: '%s'\n", addr);
        return NULL;
    }

    char *result = malloc(count + 1);
    int i;

    for (i=0; i<strlen(addr); i++) {
        if (addr[i] >= 'A' && addr[i] <= 'F' && i % 3 != 2) {
            if (lowercase) {
                result[i] = (char)tolower(addr[i]);
            } else {
                result[i] = addr[i];
            }
        } else if (addr[i] >= '0' && addr[i] <= '9' && i % 3 != 2) {
            result[i] = addr[i];
        } else if (addr[i] >= 'a' && addr[i] <= 'f' && i % 3 != 2) {
            if (lowercase) {
                result[i] = addr[i];
            } else {
                result[i] = (char)toupper(addr[i]);
            }
        } else if ((addr[i] == ':' || addr[i] == '-') && i % 3 == 2) {
            result[i] = separator;
        } else {
            psmove_WARNING("Invalid character at pos %d: '%c'\n", i, addr[i]);
            free(result);
            return NULL;
        }
    }

    result[count] = '\0';
    return result;
}

void
_psmove_wait_for_button(PSMove *move, int button)
{
    /* Wait for press */
    while ((psmove_get_buttons(move) & button) == 0) {
        psmove_poll(move);
        psmove_update_leds(move);
    }

    /* Wait for release */
    while ((psmove_get_buttons(move) & button) != 0) {
        psmove_poll(move);
        psmove_update_leds(move);
    }
}

void
psmove_util_sleep_ms(uint32_t ms)
{
    psmove_port_sleep_ms(ms);
}
