
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
#  define __USE_GNU
#  include <pthread.h>
#  include <unistd.h>
#  define PSMOVE_USE_PTHREADS
#  include "platform/psmove_linuxsupport.h"
#endif

#ifdef _WIN32
#  include <windows.h>
#  include <bthsdpdef.h>
#  include <bluetoothapis.h>
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

/* Maximum length of the serial string */
#define PSMOVE_MAX_SERIAL_LENGTH 255

/* Maximum milliseconds to inhibit further updates to LEDs if not changed */
#define PSMOVE_MAX_LED_INHIBIT_MS 4000

/* Minimum time (in milliseconds) between two LED updates (rate limiting) */
#define PSMOVE_MIN_LED_UPDATE_WAIT_MS 120

enum PSMove_Request_Type {
    PSMove_Req_GetInput = 0x01,
    PSMove_Req_SetLEDs = 0x02,
    PSMove_Req_GetBTAddr = 0x04,
    PSMove_Req_SetBTAddr = 0x05,
    PSMove_Req_GetCalibration = 0x10,
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
inline int
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

    unsigned char _padding[PSMOVE_BUFFER_SIZE-44]; /* unknown */
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
    } else if (vector.y < vector.y) {
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
    } else if (vector.y > vector.y) {
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
#endif

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
    struct hci_dev_info di = { dev_id: dev_id };
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

#if defined(__linux)
            /* Don't write padding bytes on Linux (makes it faster) */
            hid_write(move->handle, (unsigned char*)(&leds),
                    sizeof(leds) - sizeof(leds._padding));
#else
            hid_write(move->handle, (unsigned char*)(&leds),
                    sizeof(leds));
#endif

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
        assert(res == PSMOVE_CALIBRATION_SIZE);

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
    hci_for_each_dev(HCI_UP, _psmove_linux_bt_dev_info, (long)btaddr);
#elif defined(_WIN32)
    HBLUETOOTH_RADIO_FIND hFind;
    HANDLE hRadio;
    BLUETOOTH_RADIO_INFO radioInfo;

    BLUETOOTH_FIND_RADIO_PARAMS btfrp;
    btfrp.dwSize = sizeof(BLUETOOTH_FIND_RADIO_PARAMS);
    hFind = BluetoothFindFirstRadio(&btfrp, &hRadio);

    psmove_return_val_if_fail(hFind != NULL, PSMove_False);
    radioInfo.dwSize = sizeof(BLUETOOTH_RADIO_INFO);

    if (BluetoothGetRadioInfo(hRadio, &radioInfo) != ERROR_SUCCESS) {
        psmove_CRITICAL("BluetoothGetRadioInfo");
        CloseHandle(hRadio);
        BluetoothFindRadioClose(hFind);
        return PSMove_False;
    }

    int i;
    for (i=0; i<6; i++) {
        btaddr[i] = radioInfo.address.rgBytes[i];
    }

    CloseHandle(hRadio);
    BluetoothFindRadioClose(hFind);

#else
    /* TODO: Implement for other OSes (if any?) */
    return PSMove_False;
#endif

    if (memcmp(current_host, btaddr, sizeof(PSMove_Data_BTAddr)) != 0) {
        if (!psmove_set_btaddr(move, &btaddr)) {
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

    free(addr);
    free(host);

    return PSMove_True;
}

enum PSMove_Bool
psmove_pair_custom(PSMove *move, const char *btaddr_string)
{
    psmove_return_val_if_fail(move != NULL, 0);

    PSMove_Data_BTAddr btaddr;
    PSMove_Data_BTAddr current_host;

    if (!_psmove_read_btaddrs(move, &current_host, NULL)) {
        return PSMove_False;
    }

    if (!_psmove_btaddr_from_string(btaddr_string, &btaddr)) {
        return PSMove_False;
    }

    if (memcmp(current_host, btaddr, sizeof(PSMove_Data_BTAddr)) != 0) {
        if (!psmove_set_btaddr(move, &btaddr)) {
            return PSMove_False;
        }
    } else {
        psmove_DEBUG("Already paired.\n");
    }

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

    snprintf(result, size, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
            addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

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
            return Update_Success;
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

unsigned int
psmove_get_buttons(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);

    return ((move->input.buttons2) |
            (move->input.buttons1 << 8) |
            ((move->input.buttons3 & 0x01) << 16) |
            ((move->input.buttons4 & 0xF0) << 13));
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

    return ((move->input.temphigh << 4) |
            ((move->input.templow_mXhigh & 0xF0) >> 4));
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

    move->orientation_enabled = enabled;
}

enum PSMove_Bool
psmove_has_orientation(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);
    psmove_return_val_if_fail(move->orientation != NULL, 0);

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

#if defined(__APPLE__) || defined(_WIN32)

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
#endif /* __APPLE__ || _WIN32 */

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

