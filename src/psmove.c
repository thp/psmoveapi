
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
#include "psmove_private.h"
#include "psmove_calibration.h"
#include "psmove_orientation.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <math.h>
#include <limits.h>

/* OS-specific includes, for getting the Bluetooth address */
#ifdef __APPLE__
#  include "platform/psmove_osxsupport.h"
#  include <sys/syslimits.h>
#  include <sys/stat.h>
#endif

#ifdef __linux
#  include <bluetooth/bluetooth.h>
#  include <bluetooth/hci.h>
#  include <bluetooth/hci_lib.h>
#  include <sys/ioctl.h>
#  include <linux/limits.h>
#  include <pthread.h>
#  include <unistd.h>
#  define PSMOVE_USE_PTHREADS
#  include "platform/psmove_linuxsupport.h"
#endif

#ifdef _WIN32
#  include <windows.h>
#  include <bthsdpdef.h>
#  include <bluetoothapis.h>
#  include "platform/psmove_winsupport.h"
#ifndef PATH_MAX
#  define PATH_MAX MAX_PATH
#endif
#  define ENV_USER_HOME "APPDATA"
#  define PATH_SEP "\\"
#else
#  define ENV_USER_HOME "HOME"
#  define PATH_SEP "/"
#endif

#include "daemon/moved_client.h"
#include "hidapi.h"


/* Begin private definitions */

/* Buffer size for writing LEDs and reading sensor data */
#define PSMOVE_BUFFER_SIZE 49

/* Buffer size for the Bluetooth address get request */
#define PSMOVE_BTADDR_GET_SIZE 16

/* Buffer size for the Bluetooth address set request */
#define PSMOVE_BTADDR_SET_SIZE 23

/* Buffer size for sending/retrieving a request to an extension device */
#define PSMOVE_EXT_DEVICE_REPORT_SIZE 49

/* Maximum length of the serial string */
#define PSMOVE_MAX_SERIAL_LENGTH 255

/* Maximum milliseconds to inhibit further updates to LEDs if not changed */
#define PSMOVE_MAX_LED_INHIBIT_MS 4000

/* Minimum time (in milliseconds) between two LED updates (rate limiting) */
#define PSMOVE_MIN_LED_UPDATE_WAIT_MS 120


enum PSMove_Request_Type {
    PSMove_Req_GetInput = 0x01,
    PSMove_Req_SetLEDs = 0x02,
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
    unsigned char templow_mXhigh; /* temp (bits 4-1); magneto X (bits 12-9) */
    unsigned char mXlow; /* magnetometer X (bits 8-1) */
    unsigned char mYhigh; /* magnetometer Y (bits 12-5) */
    unsigned char mYlow_mZhigh; /* magnetometer: Y (bits 4-1), Z (bits 12-9) */
    unsigned char mZlow; /* magnetometer Z (bits 8-1) */
    unsigned char timelow; /* low byte of timestamp */
    unsigned char extdata[PSMOVE_EXT_DATA_BUF_SIZE]; /* external device data (EXT port) */
} PSMove_Data_Input;

typedef struct {
    int x;
    int y;
    int z;
} PSMove_3AxisVector;

int
psmove_3axisvector_min(PSMove_3AxisVector vector)
{
    if (vector.x < vector.y && vector.x < vector.z) {
        return vector.x;
    } else if (vector.y < vector.z) {
        return vector.y;
    } else {
        return vector.z;
    }
}

int
psmove_3axisvector_max(PSMove_3AxisVector vector)
{
    if (vector.x > vector.y && vector.x > vector.z) {
        return vector.x;
    } else if (vector.y > vector.z) {
        return vector.y;
    } else {
        return vector.z;
    }
}

struct _PSMove {
    /* Device type (hidapi-based or moved-based */
    enum PSMove_Device_Type type;

    /* The handle to the HIDAPI device */
    hid_device *handle;

    /* The handle to the moved client */
    moved_client *client;
    int remote_id;

    /* Index (at connection time) - not exposed yet */
    int id;

    /* Various buffers for PS Move-related data */
    PSMove_Data_LEDs leds;
    PSMove_Data_Input input;

    /* Save location for the serial number */
    char *serial_number;

    /* Device path of the controller */
    char *device_path;

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


    /** vx vy vz are in world-coordinates! */
    float vx, vy, vz;
    /** x y z are in world-coordinates! */
    float px, py, pz;




    /* Is orientation tracking currently enabled? */
    enum PSMove_Bool orientation_enabled;

    /* Minimum and maximum magnetometer values observed this session */
    PSMove_3AxisVector magnetometer_min;
    PSMove_3AxisVector magnetometer_max;

#ifdef PSMOVE_USE_PTHREADS
    /* Write thread for updating LEDs in the background */
    pthread_t led_write_thread;
    pthread_mutex_t led_write_mutex;
    pthread_cond_t led_write_new_data;
    unsigned char led_write_thread_write_queued;
    unsigned char led_write_last_result;
#endif
HANDLE dead_reckoning_thread;
unsigned int dead_reckoning_running;


#ifdef _WIN32
    /* Nonzero if this device is a Bluetooth device (on Windows only) */
    unsigned char is_bluetooth;
#endif
};

void
psmove_load_magnetometer_calibration(PSMove *move);

/* End private definitions */

static moved_client_list *clients;
static int psmove_local_disabled = 0;
static int psmove_remote_disabled = 0;

/* Number of valid, open PSMove* handles "in the wild" */
static int psmove_num_open_handles = 0;

/* Private functionality needed by the Linux version */
#if defined(__linux)

int _psmove_linux_bt_dev_info(int s, int dev_id, long arg)
{
    struct hci_dev_info di = { .dev_id = dev_id };
    unsigned char *btaddr = (void*)arg;
    int i;

    if (ioctl(s, HCIGETDEVINFO, (void *) &di) == 0) {
        for (i=0; i<6; i++) {
            btaddr[i] = di.bdaddr.b[i];
        }
    }

    return 0;
}

#endif /* defined(__linux) */


#if defined(PSMOVE_USE_PTHREADS)

void *
_psmove_led_write_thread_proc(void *data)
{
    PSMove *move = (PSMove*)data;
    PSMove_Data_LEDs leds;

    while (1) {
        pthread_mutex_lock(&(move->led_write_mutex));
        pthread_cond_wait(&(move->led_write_new_data),
                &(move->led_write_mutex));
        pthread_mutex_unlock(&(move->led_write_mutex));

        if (!move->led_write_thread_write_queued) {
            break;
        }

        do {
            /* Create a local copy of the LED state */
            memcpy(&leds, &(move->leds), sizeof(leds));
            move->led_write_thread_write_queued = 0;

            long started = psmove_util_get_ticks();

            int res = hid_write(move->handle, (unsigned char*)(&leds),
                    sizeof(leds));
            if (res== sizeof(leds)) {
                move->led_write_last_result = Update_Success;
            } else {
                psmove_DEBUG("Threaded LED update failed (wrote %d)\n", res);
                move->led_write_last_result = Update_Failed;
            }

            psmove_DEBUG("hid_write(%d) = %ld ms\n",
                    move->id,
                    psmove_util_get_ticks() - started);

            pthread_yield();
        } while (memcmp(&leds, &(move->leds), sizeof(leds)) != 0);
    }

    return NULL;
}

#endif /* defined(PSMOVE_USE_PTHREADS) */



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

/**
 * Set the Host Bluetooth address that is used to connect via
 * Bluetooth. You should set this to the local computer's
 * Bluetooth address when connected via USB, then disconnect
 * and press the PS button to connect the controller via BT.
 **/
int
psmove_set_btaddr(PSMove *move, PSMove_Data_BTAddr *addr);


/* Start implementation of the API */

enum PSMove_Bool
psmove_init(enum PSMove_Version version)
{
    /**
     * You could do initialization here. Be sure to always check
     * if initialization has already been carried out, as this
     * might be called multiple times, even after succeeding once.
     **/

    /* For now, assume future versions will be backwards-compatible */
    if (version >= PSMOVE_CURRENT_VERSION) {
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
    assert(length >= (sizeof(move->input) + 1));

    data[0] = psmove_poll(move);
    memcpy(data+1, &(move->input), sizeof(move->input));
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

int
psmove_count_connected_hidapi()
{
    struct hid_device_info *devs, *cur_dev;
    int count = 0;

    if (psmove_local_disabled) {
        return 0;
    }

    devs = hid_enumerate(PSMOVE_VID, PSMOVE_PID);
    cur_dev = devs;
    while (cur_dev) {
#ifdef _WIN32
        /**
         * Windows Quirk: Ignore extraneous devices (each dev is enumerated
         * 3 times, count only the one with "&col02#" in the path)
         *
         * We use col02 for enumeration, and col01 for connecting. We want to
         * have col02 here, because after connecting to col01, it disappears.
         **/
        if (strstr(cur_dev->path, "&col02#") == NULL) {
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
psmove_count_connected_moved(moved_client *client)
{
    psmove_return_val_if_fail(client != NULL, 0);
    return moved_client_send(client, MOVED_REQ_COUNT_CONNECTED, 0, NULL);
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
psmove_connect_internal(wchar_t *serial, char *path, int id)
{
    char *tmp;

    PSMove *move = (PSMove*)calloc(1, sizeof(PSMove));
    move->type = PSMove_HIDAPI;

    /* Make sure the first LEDs update will go through (+ init get_ticks) */
    move->last_leds_update = psmove_util_get_ticks() - PSMOVE_MAX_LED_INHIBIT_MS;

#ifdef _WIN32
    /* Windows Quirk: USB devices have "0" as serial, BT devices their addr */
    if (serial != NULL && wcslen(serial) > 1) {
        move->is_bluetooth = 1;
    }
    /* Windows Quirk: Use path instead of serial number by ignoring serial */
    serial = NULL;

    /* XXX Ugly: Convert "col02" path to "col01" path (tested w/ BT and USB) */
    char *p;
    psmove_return_val_if_fail((p = strstr(path, "&col02#")) != NULL, NULL);
    p[5] = '1';
    psmove_return_val_if_fail((p = strstr(path, "&0001#")) != NULL, NULL);
    p[4] = '0';
#endif

    if (serial == NULL && path != NULL) {
        move->handle = hid_open_path(path);
    } else {
        move->handle = hid_open(PSMOVE_VID, PSMOVE_PID, serial);
    }

    if (!move->handle) {
        free(move);
        return NULL;
    }

    /* Use Non-Blocking I/O */
    hid_set_nonblocking(move->handle, 1);

    /* Message type for LED set requests */
    move->leds.type = PSMove_Req_SetLEDs;

    /* Remember the ID/index */
    move->id = id;

    /* Remember the serial number */
    move->serial_number = (char*)calloc(PSMOVE_MAX_SERIAL_LENGTH, sizeof(char));
    if (serial != NULL) {
        wcstombs(move->serial_number, serial, PSMOVE_MAX_SERIAL_LENGTH);
    }

    if (path != NULL) {
        move->device_path = strdup(path);
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

        *tmp = tolower(*tmp);
        tmp++;
    }

#if defined(PSMOVE_USE_PTHREADS)
    psmove_return_val_if_fail(pthread_mutex_init(&move->led_write_mutex,
                NULL) == 0, NULL);
    psmove_return_val_if_fail(pthread_cond_init(&move->led_write_new_data,
                NULL) == 0, NULL);
    psmove_return_val_if_fail(pthread_create(&move->led_write_thread,
            NULL,
            _psmove_led_write_thread_proc,
            (void*)move) == 0, NULL);
    /* Assume that the first LED write will succeed */
    move->led_write_last_result = Update_Success;
#endif

    /* Bookkeeping of open handles (for psmove_reinit) */
    psmove_num_open_handles++;

    move->calibration = psmove_calibration_new(move);
    move->orientation = psmove_orientation_new(move);

    /* Load magnetometer calibration data */
    psmove_load_magnetometer_calibration(move);

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
     **/

    if (psmove_connection_type(move) == Conn_Bluetooth) {
        expected_res += 1;
        p = buf + 1;
    }

    psmove_return_val_if_fail(res == expected_res, NULL);

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
    int mode_magic_val;

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

    move->client = client;
    move->remote_id = remote_id;

    /* Message type for LED set requests */
    move->leds.type = PSMove_Req_SetLEDs;

    /* Remember the ID/index */
    move->id = id;

    /* Remember the serial number */
    move->serial_number = (char*)calloc(PSMOVE_MAX_SERIAL_LENGTH, sizeof(char));

    if (moved_client_send(move->client, MOVED_REQ_SERIAL,
                move->remote_id, NULL)) {
        /* Retrieve the serial number from the remote host */
        strncpy(move->serial_number, (char*)move->client->read_response_buf,
                PSMOVE_MAX_SERIAL_LENGTH);
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
    int count = 0;
    PSMove *move = NULL;

    devs = hid_enumerate(PSMOVE_VID, PSMOVE_PID);
    cur_dev = devs;
    while (cur_dev) {
#ifdef _WIN32
        if (strstr(cur_dev->path, "&col02#") != NULL) {
#endif
            if (count == id) {
                move = psmove_connect_internal(cur_dev->serial_number,
                        cur_dev->path, id);
                break;
            }
#ifdef _WIN32
        } else {
            count--;
        }
#endif

        count++;
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);

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
    unsigned char btg[PSMOVE_BTADDR_GET_SIZE];
    int res;

    psmove_return_val_if_fail(move != NULL, 0);

    if (move->type == PSMove_MOVED) {
        psmove_CRITICAL("Not implemented in MOVED mode");
        return 0;
    }

    /* Get Bluetooth address */
    memset(btg, 0, sizeof(btg));
    btg[0] = PSMove_Req_GetBTAddr;
    res = hid_get_feature_report(move->handle, btg, sizeof(btg));

    if (res == sizeof(btg)) {
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
_psmove_get_calibration_blob(PSMove *move, char **dest, size_t *size)
{
    psmove_return_val_if_fail(move != NULL, 0);
    psmove_return_val_if_fail(dest != NULL, 0);
    psmove_return_val_if_fail(size != NULL, 0);

    unsigned char calibration[PSMOVE_CALIBRATION_BLOB_SIZE];

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

int
psmove_set_btaddr(PSMove *move, PSMove_Data_BTAddr *addr)
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

    res = hid_send_feature_report(move->handle, bts, sizeof(bts));

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


#if defined(__APPLE__)
    char *btaddr_string = macosx_get_btaddr();
    if (btaddr_string == NULL) {
        fprintf(stderr, "WARNING: Can't determine Bluetooth address.\n"
                "Make sure Bluetooth is turned on.\n");
    }
    psmove_return_val_if_fail(btaddr_string != NULL, PSMove_False);
    if (!_psmove_btaddr_from_string(btaddr_string, &btaddr)) {
        free(btaddr_string);
        return PSMove_False;
    }
    free(btaddr_string);
#elif defined(__linux)
    PSMove_Data_BTAddr blank;
    memset(blank, 0, sizeof(PSMove_Data_BTAddr));
    memset(btaddr, 0, sizeof(PSMove_Data_BTAddr));
    hci_for_each_dev(HCI_UP, _psmove_linux_bt_dev_info, (long)btaddr);
    if(memcmp(btaddr, blank, sizeof(PSMove_Data_BTAddr))==0) {
        fprintf(stderr, "WARNING: Can't determine Bluetooth address.\n"
                "Make sure Bluetooth is turned on.\n");
        return PSMove_False;
    }
#elif defined(_WIN32)
    HANDLE hRadio;
    if (windows_get_first_bluetooth_radio(&hRadio) != 0 || !hRadio) {
        psmove_WARNING("Failed to find a Bluetooth radio");
        return PSMove_False;
    }

    BLUETOOTH_RADIO_INFO radioInfo;
    radioInfo.dwSize = sizeof(BLUETOOTH_RADIO_INFO);

    if (BluetoothGetRadioInfo(hRadio, &radioInfo) != ERROR_SUCCESS) {
        psmove_CRITICAL("BluetoothGetRadioInfo");
        CloseHandle(hRadio);
        return PSMove_False;
    }

    int i;
    for (i=0; i<6; i++) {
        btaddr[i] = radioInfo.address.rgBytes[i];
    }

#else
    /* TODO: Implement for other OSes (if any?) */
    return PSMove_False;
#endif

    if (memcmp(current_host, btaddr, sizeof(PSMove_Data_BTAddr)) != 0) {
        if (!psmove_set_btaddr(move, &btaddr)) {
#if defined(_WIN32)
            CloseHandle(hRadio);
#endif
            return PSMove_False;
        }
    } else {
        psmove_DEBUG("Already paired.\n");
    }

    char *addr = psmove_get_serial(move);
    char *host = _psmove_btaddr_to_string(btaddr);

#if defined(__linux)
    /* Add entry to Bluez' bluetoothd state file */
    linux_bluez_register_psmove(addr, host);
#endif

#if defined(__APPLE__)
    /* Add entry to the com.apple.Bluetooth.plist file */
    macosx_blued_register_psmove(addr);
#endif

#if defined(_WIN32)
    windows_register_psmove(addr, hRadio);
    CloseHandle(hRadio);
#endif

    free(addr);
    free(host);

    return PSMove_True;
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
        if (!psmove_set_btaddr(move, &new_host)) {
            return PSMove_False;
        }
    } else {
        psmove_DEBUG("Already paired.\n");
    }

    char *addr = psmove_get_serial(move);
    char *host = _psmove_btaddr_to_string(new_host);

#if defined(__linux)
    /* Add entry to Bluez' bluetoothd state file */
    linux_bluez_register_psmove(addr, host);
#endif

#if defined(__APPLE__)
    /* Add entry to the com.apple.Bluetooth.plist file */
    macosx_blued_register_psmove(addr);
#endif

    free(addr);
    free(host);

    return PSMove_True;
}

enum PSMove_Connection_Type
psmove_connection_type(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, Conn_Unknown);

    if (move->type == PSMove_MOVED) {
        return Conn_Bluetooth;
    }

#if defined(_WIN32)
    if (move->is_bluetooth) {
        return Conn_Bluetooth;
    } else {
        return Conn_USB;
    }
#endif

    if (move->serial_number == NULL) {
        return Conn_Unknown;
    }

    if (strlen(move->serial_number) == 0) {
        return Conn_USB;
    }

    return Conn_Bluetooth;
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
        tmp[5-i] = value;
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
#if defined(PSMOVE_USE_PTHREADS)
            pthread_mutex_lock(&(move->led_write_mutex));
            move->led_write_thread_write_queued = 1;
            pthread_cond_signal(&(move->led_write_new_data));
            pthread_mutex_unlock(&(move->led_write_mutex));
            /**
             * Can only return the last result, but assume that we can either
             * write successfully or not at all, this should be good enough.
             **/
            return move->led_write_last_result;
#else
            if (hid_write(move->handle, (unsigned char*)(&(move->leds)),
                    sizeof(move->leds)) == sizeof(move->leds)) {
                return Update_Success;
            } else {
                return Update_Failed;
            }
#endif
            break;
        case PSMove_MOVED:
            /**
             * XXX: This only tells us that the sending went through, but
             * as we don't wait for a confirmation, the packet might still
             * not arrive at the target machine. As we usually send many
             * updates, a few dropped packets are normally no problem.
             **/
            if (moved_client_send(move->client, MOVED_REQ_WRITE,
                        move->remote_id, (unsigned char*)(&move->leds))) {
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
    int oldseq = (move->input.buttons4 & 0x0F);

    switch (move->type) {
        case PSMove_HIDAPI:
            res = hid_read(move->handle, (unsigned char*)(&(move->input)),
                sizeof(move->input));
            break;
        case PSMove_MOVED:
            if (moved_client_send(move->client, MOVED_REQ_READ,
                        move->remote_id, NULL)) {
                /**
                 * The input buffer is stored at offset 1 (the first byte
                 * contains the return value of the remote psmove_poll())
                 **/
                memcpy((unsigned char*)(&(move->input)),
                        move->client->read_response_buf+1,
                        sizeof(move->input));

                /**
                 * The first byte of the response contains the return value
                 * of the remote psmove_poll() call - if it is nonzero, we
                 * want to calculate a non-zero value locally (see below).
                 *
                 * See also _psmove_read_data() for how the buffer is filled
                 * on the remote end of the moved protocol connection.
                 **/
                if (move->client->read_response_buf[0] != 0) {
                    res = sizeof(move->input);
                }
            }
            break;
        default:
            psmove_CRITICAL("Unknown device type");
    }

    if (res == sizeof(move->input)) {
        /* Sanity check: The first byte should be PSMove_Req_GetInput */
        psmove_return_val_if_fail(move->input.type == PSMove_Req_GetInput, 0);

        /**
         * buttons4's 4 least significant bits contain the sequence number,
         * so we add 1 to signal "success" and add the sequence number for
         * consumers to utilize the data
         **/
        int seq = (move->input.buttons4 & 0x0F);
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

    assert(sizeof(*data) >= sizeof(move->input.extdata));

    memcpy(data, move->input.extdata, sizeof(move->input.extdata));
    return PSMove_True;
}

unsigned int
psmove_get_buttons(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);

    /**
     * See also: enum PSMove_Button in include/psmove.h
     *
     * Source:
     * https://code.google.com/p/moveonpc/wiki/InputReport
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

    return ((move->input.buttons2) |
            (move->input.buttons1 << 8) |
            ((move->input.buttons3 & 0x01) << 16) |
            ((move->input.buttons4 & 0xF0) << 13 /* 13 = 17 - 4 */));
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

    if((move->input.buttons4 & 0x10) != 0) {
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

enum PSMove_Battery_Level
psmove_get_battery(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);

    return move->input.battery;
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

    return ((move->input.temphigh << 4) |
            ((move->input.templow_mXhigh & 0xF0) >> 4));
}

float
psmove_get_temperature_in_celsius(PSMove *move)
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

    psmove_return_val_if_fail(move != NULL, 0.0);

    int raw_value = psmove_get_temperature(move);
    int i;

    for (i = 0; i < 80; i++) {
        if (temperature_lookup[i] > raw_value) {
            return i - 10;
        }
    }

    return 70;
}

unsigned char
psmove_get_trigger(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);

    return (move->input.trigger + move->input.trigger2) / 2;
}

void
psmove_get_half_frame(PSMove *move, enum PSMove_Sensor sensor,
        enum PSMove_Frame frame, int *x, int *y, int *z)
{
    psmove_return_if_fail(move != NULL);
    psmove_return_if_fail(sensor == Sensor_Accelerometer || sensor == Sensor_Gyroscope);
    psmove_return_if_fail(frame == Frame_FirstHalf || frame == Frame_SecondHalf);

    int base;

    switch (sensor) {
        case Sensor_Accelerometer:
            base = offsetof(PSMove_Data_Input, aXlow);
            break;
        case Sensor_Gyroscope:
            base = offsetof(PSMove_Data_Input, gXlow);
            break;
        default:
            return;
    }

    if (frame == Frame_SecondHalf) {
        base += 6;
    }

    if (x != NULL) {
        *x = psmove_decode_16bit((void*)&move->input, base + 0);
    }

    if (y != NULL) {
        *y = psmove_decode_16bit((void*)&move->input, base + 2);
    }

    if (z != NULL) {
        *z = psmove_decode_16bit((void*)&move->input, base + 4);
    }
}

void
psmove_get_accelerometer(PSMove *move, int *ax, int *ay, int *az)
{
    psmove_return_if_fail(move != NULL);

    if (ax != NULL) {
        *ax = ((move->input.aXlow + move->input.aXlow2) +
               ((move->input.aXhigh + move->input.aXhigh2) << 8)) / 2 - 0x8000;
    }

    if (ay != NULL) {
        *ay = ((move->input.aYlow + move->input.aYlow2) +
               ((move->input.aYhigh + move->input.aYhigh2) << 8)) / 2 - 0x8000;
    }

    if (az != NULL) {
        *az = ((move->input.aZlow + move->input.aZlow2) +
               ((move->input.aZhigh + move->input.aZhigh2) << 8)) / 2 - 0x8000;
    }
}

void
psmove_get_gyroscope(PSMove *move, int *gx, int *gy, int *gz)
{
    psmove_return_if_fail(move != NULL);

    if (gx != NULL) {
        *gx = ((move->input.gXlow + move->input.gXlow2) +
               ((move->input.gXhigh + move->input.gXhigh2) << 8)) / 2 - 0x8000;
    }

    if (gy != NULL) {
        *gy = ((move->input.gYlow + move->input.gYlow2) +
               ((move->input.gYhigh + move->input.gYhigh2) << 8)) / 2 - 0x8000;
    }

    if (gz != NULL) {
        *gz = ((move->input.gZlow + move->input.gZlow2) +
               ((move->input.gZhigh + move->input.gZhigh2) << 8)) / 2 - 0x8000;
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
psmove_get_magnetometer_vector(PSMove *move,
        float *mx, float *my, float *mz)
{
    PSMove_3AxisVector raw;

    psmove_return_if_fail(move != NULL);

    psmove_get_magnetometer(move, &raw.x, &raw.y, &raw.z);

    /* Update minimum values */
    if (raw.x < move->magnetometer_min.x) {
        move->magnetometer_min.x = raw.x;
    }
    if (raw.y < move->magnetometer_min.y) {
        move->magnetometer_min.y = raw.y;
    }
    if (raw.z < move->magnetometer_min.z) {
        move->magnetometer_min.z = raw.z;
    }

    /* Update maximum values */
    if (raw.x > move->magnetometer_max.x) {
        move->magnetometer_max.x = raw.x;
    }
    if (raw.y > move->magnetometer_max.y) {
        move->magnetometer_max.y = raw.y;
    }
    if (raw.z > move->magnetometer_max.z) {
        move->magnetometer_max.z = raw.z;
    }

    /* Map [min..max] to [-1..+1] */
    if (mx) {
        *mx = -1.f + 2.f * (float)(raw.x - move->magnetometer_min.x) /
            (float)(move->magnetometer_max.x - move->magnetometer_min.x);
    }
    if (my) {
        *my = -1.f + 2.f * (float)(raw.y - move->magnetometer_min.y) /
            (float)(move->magnetometer_max.y - move->magnetometer_min.y);
    }
    if (mz) {
        *mz = -1.f + 2.f * (float)(raw.z - move->magnetometer_min.z) /
            (float)(move->magnetometer_max.z - move->magnetometer_min.z);
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

    if (mx != NULL) {
        *mx = TWELVE_BIT_SIGNED(((move->input.templow_mXhigh & 0x0F) << 8) |
                move->input.mXlow);
    }

    if (my != NULL) {
        *my = TWELVE_BIT_SIGNED((move->input.mYhigh << 4) |
               (move->input.mYlow_mZhigh & 0xF0) >> 4);
    }

    if (mz != NULL) {
        *mz = TWELVE_BIT_SIGNED(((move->input.mYlow_mZhigh & 0x0F) << 8) |
                move->input.mZlow);
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
    psmove_return_val_if_fail(move != NULL, 0);
    psmove_return_val_if_fail(move->orientation != NULL, 0);

#if !defined(PSMOVE_WITH_MADGWICK_AHRS)
    psmove_WARNING("Built without Madgwick AHRS - no orientation support");
    return PSMove_False;
#endif

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

DWORD WINAPI
_psmove_dead_reckoning_thread_proc(void *data)
{
    PSMove *move = (PSMove*)data;
    long intended_tick_length = 40;
    long ticks = 0;
    long diff;

    float orientation[16];

    float raw_ax, raw_ay, raw_az;
    float raw_acceleration[16];
    float oriented_acceleration[16];
    float bias_ax = +0.038;
    float bias_ay = -0.02;
    float bias_az = -0.02;
    float corrected_ax, corrected_ay, corrected_az;

    while (move->dead_reckoning_running) {
        long diff = psmove_util_get_ticks() - ticks;
        ticks = ticks + diff;

        printf("\nTicks Passed [ms]: %ld / now at %ld\n", diff, ticks);

        if (psmove_get_buttons(move) & Btn_CIRCLE) {
          psmove_reset_orientation(move);
          move->vx = 0;
          move->vy = 0;
          move->vz = 0;
          move->px = 0;
          move->py = 0;
          move->pz = 0;
        }

        // heading
        float hw, hx, hy, hz;
        psmove_get_orientation(move, &hw, &hx, &hy, &hz);
        printf("heading: %3f %3f %3f %3f\n", hw, hx, hy, hz);

        // quaternion to 4x4 from http://www.flipcode.com/documents/matrfaq.html#Q54
        float xx = hx * hx;
        float xy = hx * hy;
        float xz = hx * hz;
        float xw = hx * hw;

        float yy = hy * hy;
        float yz = hy * hz;
        float yw = hy * hw;

        float zz = hz * hz;
        float zw = hz * hw;

        orientation[0]  = 1 - 2 * ( yy + zz );
        orientation[1]  =     2 * ( xy - zw );
        orientation[2]  =     2 * ( xz + yw );

        orientation[4]  =     2 * ( xy + zw );
        orientation[5]  = 1 - 2 * ( xx + zz );
        orientation[6]  =     2 * ( yz - xw );

        orientation[8]  =     2 * ( xz - yw );
        orientation[9]  =     2 * ( yz + xw );
        orientation[10] = 1 - 2 * ( xx + yy );

        orientation[3]  = orientation[7] = orientation[11] = orientation[12] = orientation[13] = orientation[14] = 0;
        orientation[15] = 1;

        int frame;
        for (frame = 0; frame<2; frame++) {
          float total;
          psmove_get_accelerometer_frame(move, frame, &raw_ax, &raw_ay, &raw_az);
          total = sqrt(raw_ax*raw_ax + raw_ay*raw_ay + raw_az*raw_az);
          printf("raw acceleration %05f;%05f;%05f;%05f\n", raw_ax, raw_ay, raw_az, total);

          // initial 4x4 matrix
          raw_acceleration[0] = 1;
          raw_acceleration[1] = 0;
          raw_acceleration[2] = 0;
          raw_acceleration[3] = raw_ax - bias_ax;
          raw_acceleration[4] = 0;
          raw_acceleration[5] = 1;
          raw_acceleration[6] = 0;
          raw_acceleration[7] = raw_ay - bias_ay;
          raw_acceleration[8] = 0;
          raw_acceleration[9] = 0;
          raw_acceleration[10] = 1;
          raw_acceleration[11] = raw_az - bias_az;
          raw_acceleration[12] = 0;
          raw_acceleration[13] = 0;
          raw_acceleration[14] = 0;
          raw_acceleration[15] = 1;

          oriented_acceleration[0] = raw_acceleration[0]*orientation[0] + raw_acceleration[4]*orientation[1] + raw_acceleration[8]*orientation[2] + raw_acceleration[12]*orientation[3];
          oriented_acceleration[1] = raw_acceleration[1]*orientation[0] + raw_acceleration[5]*orientation[1] + raw_acceleration[9]*orientation[2] + raw_acceleration[13]*orientation[3];
          oriented_acceleration[2] = raw_acceleration[2]*orientation[0] + raw_acceleration[6]*orientation[1] + raw_acceleration[10]*orientation[2] + raw_acceleration[14]*orientation[3];
          oriented_acceleration[3] = raw_acceleration[3]*orientation[0] + raw_acceleration[7]*orientation[1] + raw_acceleration[11]*orientation[2] + raw_acceleration[15]*orientation[3];
          oriented_acceleration[4] = raw_acceleration[0]*orientation[4] + raw_acceleration[4]*orientation[5] + raw_acceleration[8]*orientation[6] + raw_acceleration[12]*orientation[7];
          oriented_acceleration[5] = raw_acceleration[1]*orientation[4] + raw_acceleration[5]*orientation[5] + raw_acceleration[9]*orientation[6] + raw_acceleration[13]*orientation[7];
          oriented_acceleration[6] = raw_acceleration[2]*orientation[4] + raw_acceleration[6]*orientation[5] + raw_acceleration[10]*orientation[6] + raw_acceleration[14]*orientation[7];
          oriented_acceleration[7] = raw_acceleration[3]*orientation[4] + raw_acceleration[7]*orientation[5] + raw_acceleration[11]*orientation[6] + raw_acceleration[15]*orientation[7];
          oriented_acceleration[8] = raw_acceleration[0]*orientation[8] + raw_acceleration[4]*orientation[9] + raw_acceleration[8]*orientation[10] + raw_acceleration[12]*orientation[11];
          oriented_acceleration[9] = raw_acceleration[1]*orientation[8] + raw_acceleration[5]*orientation[9] + raw_acceleration[9]*orientation[10] + raw_acceleration[13]*orientation[11];
          oriented_acceleration[10] = raw_acceleration[2]*orientation[8] + raw_acceleration[6]*orientation[9] + raw_acceleration[10]*orientation[10] + raw_acceleration[14]*orientation[11];
          oriented_acceleration[11] = raw_acceleration[3]*orientation[8] + raw_acceleration[7]*orientation[9] + raw_acceleration[11]*orientation[10] + raw_acceleration[15]*orientation[11];
          oriented_acceleration[12] = raw_acceleration[0]*orientation[12] + raw_acceleration[4]*orientation[13] + raw_acceleration[8]*orientation[14] + raw_acceleration[12]*orientation[15];
          oriented_acceleration[13] = raw_acceleration[1]*orientation[12] + raw_acceleration[5]*orientation[13] + raw_acceleration[9]*orientation[14] + raw_acceleration[13]*orientation[15];
          oriented_acceleration[14] = raw_acceleration[2]*orientation[12] + raw_acceleration[6]*orientation[13] + raw_acceleration[10]*orientation[14] + raw_acceleration[14]*orientation[15];
          oriented_acceleration[15] = raw_acceleration[3]*orientation[12] + raw_acceleration[7]*orientation[13] + raw_acceleration[11]*orientation[14] + raw_acceleration[15]*orientation[15];

          oriented_acceleration[7] -= 1; // account for gravity (hopefully)

          // first integration for speed
          // TODO: needs to take orientation into account!
          float g = 9.80665f; // [m/s²]
          float dt = 0.5f * diff / 1000; // [s] for each frame
          move->vx += oriented_acceleration[3] * g * dt;
          move->vy += oriented_acceleration[7] * g * dt;
          move->vz += oriented_acceleration[11] * g * dt;
          printf("velocity: %3f %3f %3f\n", move->vx, move->vy, move->vz);

          // second integration for position [m]
          move->px += move->vx * dt;
          move->py += move->vy * dt;
          move->pz += move->vz * dt;
          printf("position: %3f %3f %3f\n", move->px, move->py, move->pz);
        }


        Sleep(intended_tick_length - (ticks % intended_tick_length));
    }

    return 0;
}



void
psmove_enable_background_dead_reckoning(PSMove *move)
{
    printf("Trying to create a thread");

    move->dead_reckoning_running = 1;
    move->dead_reckoning_thread = CreateThread(
        NULL, // LPSec
        0, // SizeT
        _psmove_dead_reckoning_thread_proc, // routine
        (void*)move, // parameter
        0, // creation flags
        NULL // out opt
    );
}

void
psmove_disable_background_dead_reckoning(PSMove *move)
{
    printf("Trying to stop and join the thread");
    move->dead_reckoning_running = 0;
    WaitForSingleObject(&(move->dead_reckoning_thread), INFINITE);
    printf("joined.");
}

void
psmove_get_dead_reckoning_position(PSMove *move, float *out_x, float *out_y, float *out_z)
{
    *out_x = move->px;
    *out_y = move->py;
    *out_z = move->pz;
}

void
psmove_get_dead_reckoning_velocity(PSMove *move, float *out_x, float *out_y, float *out_z)
{
    *out_x = move->vx;
    *out_y = move->vy;
    *out_z = move->vz;
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

    move->magnetometer_min.x =
        move->magnetometer_min.y =
        move->magnetometer_min.z = INT_MAX;
    move->magnetometer_max.x =
        move->magnetometer_max.y =
        move->magnetometer_max.z = INT_MIN;
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

    fprintf(fp, "axis,min,max\n");
    fprintf(fp, "x,%d,%d\n", move->magnetometer_min.x, move->magnetometer_max.x);
    fprintf(fp, "y,%d,%d\n", move->magnetometer_min.y, move->magnetometer_max.y);
    fprintf(fp, "z,%d,%d\n", move->magnetometer_min.z, move->magnetometer_max.z);

    fclose(fp);
}

void
psmove_load_magnetometer_calibration(PSMove *move)
{
    psmove_return_if_fail(move != NULL);
    psmove_reset_magnetometer_calibration(move);
    char *filename = psmove_get_magnetometer_calibration_filename(move);
    FILE *fp = fopen(filename, "r");
    free(filename);

    if (fp == NULL) {
        char *addr = psmove_get_serial(move);
        psmove_WARNING("Magnetometer in %s not yet calibrated.\n", addr);
        free(addr);
        return;
    }

    char s_axis[5], s_min[4], s_max[4];
    char c_axis;
    int i_min, i_max;
    int result;

    result = fscanf(fp, "%4s,%3s,%3s\n", s_axis, s_min, s_max);
    psmove_goto_if_fail(result == 3, finish);
    psmove_goto_if_fail(strcmp(s_axis, "axis") == 0, finish);
    psmove_goto_if_fail(strcmp(s_min, "min") == 0, finish);
    psmove_goto_if_fail(strcmp(s_max, "max") == 0, finish);

    result = fscanf(fp, "%c,%d,%d\n", &c_axis, &i_min, &i_max);
    psmove_goto_if_fail(result == 3, finish);
    psmove_goto_if_fail(c_axis == 'x', finish);
    move->magnetometer_min.x = i_min;
    move->magnetometer_max.x = i_max;

    result = fscanf(fp, "%c,%d,%d\n", &c_axis, &i_min, &i_max);
    psmove_goto_if_fail(result == 3, finish);
    psmove_goto_if_fail(c_axis == 'y', finish);
    move->magnetometer_min.y = i_min;
    move->magnetometer_max.y = i_max;

    result = fscanf(fp, "%c,%d,%d\n", &c_axis, &i_min, &i_max);
    psmove_goto_if_fail(result == 3, finish);
    psmove_goto_if_fail(c_axis == 'z', finish);
    move->magnetometer_min.z = i_min;
    move->magnetometer_max.z = i_max;

finish:
    fclose(fp);
}

int
psmove_get_magnetometer_calibration_range(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0.);

    PSMove_3AxisVector diff = {
        move->magnetometer_max.x - move->magnetometer_min.x,
        move->magnetometer_max.y - move->magnetometer_min.y,
        move->magnetometer_max.z - move->magnetometer_min.z,
    };

    return psmove_3axisvector_min(diff);
}

void
psmove_disconnect(PSMove *move)
{
    psmove_return_if_fail(move != NULL);

#if defined(PSMOVE_USE_PTHREADS)
    if (move->type != PSMove_MOVED) {
        while (pthread_tryjoin_np(move->led_write_thread, NULL) != 0) {
            pthread_mutex_lock(&(move->led_write_mutex));
            pthread_cond_signal(&(move->led_write_new_data));
            pthread_mutex_unlock(&(move->led_write_mutex));
        }

        pthread_mutex_destroy(&(move->led_write_mutex));
        pthread_cond_destroy(&(move->led_write_new_data));
    }
#endif

    switch (move->type) {
        case PSMove_HIDAPI:
            hid_close(move->handle);
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
    free(move);

    /* Bookkeeping of open handles (for psmove_reinit) */
    psmove_return_if_fail(psmove_num_open_handles > 0);
    psmove_num_open_handles--;
}

long
psmove_util_get_ticks()
{
#ifdef WIN32
    static LARGE_INTEGER startup_time = { .QuadPart = 0 };
    static LARGE_INTEGER frequency = { .QuadPart = 0 };
    LARGE_INTEGER now;

    if (frequency.QuadPart == 0) {
        psmove_return_val_if_fail(QueryPerformanceFrequency(&frequency), 0);
    }

    psmove_return_val_if_fail(QueryPerformanceCounter(&now), 0);

    /* The first time this function gets called, we init startup_time */
    if (startup_time.QuadPart == 0) {
        startup_time.QuadPart = now.QuadPart;
    }

    return (long)((now.QuadPart - startup_time.QuadPart) * 1000 /
            frequency.QuadPart);
#else
    static long startup_time = 0;
    long now;
    struct timeval tv;

    psmove_return_val_if_fail(gettimeofday(&tv, NULL) == 0, 0);
    now = (tv.tv_sec * 1000 + tv.tv_usec / 1000);

    /* The first time this function gets called, we init startup_time */
    if (startup_time == 0) {
        startup_time = now;
    }

    return (now - startup_time);
#endif
}

const char *
psmove_util_get_data_dir()
{
    static char dir[PATH_MAX];

    if (strlen(dir) == 0) {
        strncpy(dir, getenv(ENV_USER_HOME), sizeof(dir));
        strncat(dir, PATH_SEP ".psmoveapi", sizeof(dir));
    }

    return dir;
}

char *
psmove_util_get_file_path(const char *filename)
{
    const char *parent = psmove_util_get_data_dir();
    char *result;
    struct stat st;

#ifndef __WIN32
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
    int len = strlen(PSMOVE_SYSTEM_DATA_DIR) + 1 + strlen(filename) + 1;

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
    int count = strlen(addr);

    if (count != 17) {
        psmove_WARNING("Invalid address: '%s'\n", addr);
        return NULL;
    }

    char *result = malloc(count + 1);
    int i;

    for (i=0; i<strlen(addr); i++) {
        if (addr[i] >= 'A' && addr[i] <= 'F' && i % 3 != 2) {
            if (lowercase) {
                result[i] = tolower(addr[i]);
            } else {
                result[i] = addr[i];
            }
        } else if (addr[i] >= '0' && addr[i] <= '9' && i % 3 != 2) {
            result[i] = addr[i];
        } else if (addr[i] >= 'a' && addr[i] <= 'f' && i % 3 != 2) {
            if (lowercase) {
                result[i] = addr[i];
            } else {
                result[i] = toupper(addr[i]);
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

#if defined(__APPLE__)

#define CLOCK_MONOTONIC 0

static int
clock_gettime(int unused, struct timespec *ts)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;

    return 0;
}
#endif /* __APPLE__ */

PSMove_timestamp
_psmove_timestamp()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

PSMove_timestamp
_psmove_timestamp_diff(PSMove_timestamp a, PSMove_timestamp b)
{
    struct timespec ts;
    if (a.tv_nsec >= b.tv_nsec) {
        ts.tv_sec = a.tv_sec - b.tv_sec;
        ts.tv_nsec = a.tv_nsec - b.tv_nsec;
    } else {
        ts.tv_sec = a.tv_sec - b.tv_sec - 1;
        ts.tv_nsec = 1000000000 + a.tv_nsec - b.tv_nsec;
    }
    return ts;
}

double
_psmove_timestamp_value(PSMove_timestamp ts)
{
    return ts.tv_sec + ts.tv_nsec * 0.000000001;
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

