
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>

/* OS-specific includes, for getting the Bluetooth address */
#ifdef __APPLE__
#  include <IOBluetooth/Bluetooth.h>
#endif

#ifdef __linux
#  include <bluetooth/bluetooth.h>
#  include <bluetooth/hci.h>
#  include <sys/ioctl.h>
#endif

#ifdef _WIN32
#  include <windows.h>
#  include <Bthsdpdef.h>
#  include <BluetoothAPIs.h>
#endif

#ifdef WITH_MOVED_CLIENT
#  include "moved_client.h"
#else
#  include "hidapi.h"
#endif


/* Begin private definitions */

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

/* Decode 12-bit signed value (assuming two's complement) */
#define TWELVE_BIT_SIGNED(x) (((x) & 0x800)?(-(((~(x)) & 0xFFF) + 1)):(x))

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

struct _PSMove {
    /* The handle to the HIDAPI device */
#ifndef WITH_MOVED_CLIENT
    hid_device *handle;
#endif

    /* Index (at connection time) - not exposed yet */
    int id;

    /* Various buffers for PS Move-related data */
    PSMove_Data_LEDs leds;
    PSMove_Data_Input input;

    /* Save location for the controller BTAddr */
    PSMove_Data_BTAddr btaddr;

#ifdef _WIN32
    int is_bluetooth;
#endif
};

/* Macro: Print a critical message if an assertion fails */
#define psmove_CRITICAL(x) \
        {fprintf(stderr, "[PSMOVE] Assertion fail in %s: %s\n", __func__, x);}

/* Macros: Return immediately if an assertion fails + log */
#define psmove_return_if_fail(expr) \
        {if(!(expr)){psmove_CRITICAL(#expr);return;}}
#define psmove_return_val_if_fail(expr, val) \
        {if(!(expr)){psmove_CRITICAL(#expr);return(val);}}

/* End private definitions */


#ifdef WITH_MOVED_CLIENT
static moved_client *client;
#endif

/* Private functionality needed by the Linux version */
#if defined(__linux)

int _psmove_linux_bt_dev_info(int s, int dev_id, long arg)
{
    struct hci_dev_info di = { dev_id: dev_id };
    unsigned char *btaddr = (void*)arg;
    int i;

    if (ioctl(s, HCIGETDEVINFO, (void *) &di) == 0) {
        for (i=0; i<6; i++) {
            btaddr[i] = di.bdaddr.b[5-i];
        }
    }

    return 0;
}

#endif /* defined(__linux) */


/* Start implementation of the API */

int
psmove_count_connected()
{
#ifdef WITH_MOVED_CLIENT
    if (client == NULL) {
        client = moved_client_create("127.0.0.1");
    }
    return moved_client_send(client, MOVED_REQ_COUNT_CONNECTED, 0, NULL);
#else
    struct hid_device_info *devs, *cur_dev;
    int count = 0;

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
#endif
}

PSMove *
psmove_connect_internal(wchar_t *serial, char *path, int id)
{
    PSMove *move = (PSMove*)calloc(1, sizeof(PSMove));

#ifdef _WIN32
    /* Windows Quirk: USB devices have "0" as serial, BT devices their addr */
    if (wcslen(serial) > 1) {
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

#ifdef WITH_MOVED_CLIENT
    if (client == NULL) {
        client = moved_client_create("127.0.0.1");
    }
#else
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
#endif

    /* Message type for LED set requests */
    move->leds.type = PSMove_Req_SetLEDs;

    /* Remember the ID/index */
    move->id = id;

    return move;
}

PSMove *
psmove_connect_by_id(int id)
{
#ifdef WITH_MOVED_CLIENT
    if (psmove_count_connected() > id) {
        return psmove_connect_internal(NULL, NULL, id);
    } else {
        return NULL;
    }
#else
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
#endif
}


PSMove *
psmove_connect()
{
    return psmove_connect_internal(NULL, NULL, 0);
}

int
psmove_get_btaddr(PSMove *move, PSMove_Data_BTAddr *addr)
{
#ifdef WITH_MOVED_CLIENT
    return 0;
#else /* !WITH_MOVED_CLIENT */
    unsigned char cal[PSMOVE_CALIBRATION_SIZE];
    unsigned char btg[PSMOVE_BTADDR_GET_SIZE];
    int res;
    int i;

    psmove_return_val_if_fail(move != NULL, 0);

    /* Get calibration data */
    memset(cal, 0, sizeof(cal));
    cal[0] = PSMove_Req_GetCalibration;
    res = hid_get_feature_report(move->handle, cal, sizeof(cal));
    if (res < 0) {
        return 0;
    }

    /* Get Bluetooth address */
    memset(btg, 0, sizeof(btg));
    btg[0] = PSMove_Req_GetBTAddr;
    res = hid_get_feature_report(move->handle, btg, sizeof(btg));

    if (res == sizeof(btg)) {
#ifdef PSMOVE_DEBUG
        fprintf(stderr, "[PSMOVE] controller bt mac addr: ");
        for (i=6; i>=1; i--) {
            if (i != 6) putc(':', stderr);
            fprintf(stderr, "%02x", btg[i]);
        }
        fprintf(stderr, "\n");

        fprintf(stderr, "[PSMOVE] current host bt mac addr: ");
        for (i=15; i>=10; i--) {
            if (i != 15) putc(':', stderr);
            fprintf(stderr, "%02x", btg[i]);
        }
        fprintf(stderr, "\n");
#endif /* PSMOVE_DEBUG */
        if (addr != NULL) {
            /* Copy 6 bytes (btg[10]..btg[15]) into addr, reversed */
            for (i=0; i<6; i++) {
                (*addr)[i] = btg[15-i];
            }
            memcpy(addr, btg+10, 6);
        }

        /* Success! */
        return 1;
    }

    return 0;
#endif /* !WITH_MOVED_CLIENT */
}

int
psmove_controller_btaddr(PSMove *move, PSMove_Data_BTAddr *addr)
{
    int i;

    psmove_return_val_if_fail(move != NULL, 0);
    psmove_return_val_if_fail(addr != NULL, 0);

    memcpy(addr, move->btaddr, sizeof(PSMove_Data_BTAddr));

    return 1;
}

int
psmove_set_btaddr(PSMove *move, PSMove_Data_BTAddr *addr)
{
#ifdef WITH_MOVED_CLIENT
    return 0;
#else
    unsigned char bts[PSMOVE_BTADDR_SET_SIZE];
    int res;
    int i;
    PSMove_Data_BTAddr btaddr;

    psmove_return_val_if_fail(move != NULL, 0);
    psmove_return_val_if_fail(addr != NULL, 0);

    /* Get calibration data */
    memset(bts, 0, sizeof(bts));
    bts[0] = PSMove_Req_SetBTAddr;

    /* Copy 6 bytes from addr into bts[1]..bts[6], reversed */
    for (i=0; i<6; i++) {
        bts[1+5-i] = (*addr)[i];
    }

    res = hid_send_feature_report(move->handle, bts, sizeof(bts));

    return (res == sizeof(bts));
#endif
}

int
psmove_pair(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, 0);

    PSMove_Data_BTAddr btaddr;
    int i;

    if (!psmove_get_btaddr(move, &btaddr)) {
        return 0;
    }


#if defined(__APPLE__)
    BluetoothDeviceAddress address;
    IOReturn result;

    result = IOBluetoothLocalDeviceReadAddress(&address, NULL, NULL, NULL);

    if (result != kIOReturnSuccess) {
        return 0;
    }

    for (i=0; i<6; i++) {
        btaddr[i] = address.data[i];
    }

#elif defined(__linux) && !defined(WITH_MOVED_CLIENT)
    hci_for_each_dev(HCI_UP, _psmove_linux_bt_dev_info, (long)btaddr);
#elif defined(_WIN32)
    HBLUETOOTH_RADIO_FIND hFind;
    HANDLE hRadio;
    BLUETOOTH_RADIO_INFO radioInfo;

    BLUETOOTH_FIND_RADIO_PARAMS btfrp;
    btfrp.dwSize = sizeof(BLUETOOTH_FIND_RADIO_PARAMS);
    hFind = BluetoothFindFirstRadio(&btfrp, &hRadio);

    psmove_return_val_if_fail(hFind != NULL, 0);
    radioInfo.dwSize = sizeof(BLUETOOTH_RADIO_INFO);

    if (BluetoothGetRadioInfo(hRadio, &radioInfo) != ERROR_SUCCESS) {
        psmove_CRITICAL("BluetoothGetRadioInfo");
        CloseHandle(hRadio);
        BluetoothFindRadioClose(hFind);
        return 0;
    }

    for (i=0; i<6; i++) {
        btaddr[i] = radioInfo.address.rgBytes[5-i];
    }

    CloseHandle(hRadio);
    BluetoothFindRadioClose(hFind);

#else
    /* TODO: Implement for other OSes (if any?) */
    return 0;
#endif

    if (!psmove_set_btaddr(move, &btaddr)) {
        return 0;
    }

    return 1;
}

int
psmove_pair_custom(PSMove *move, const char *btaddr_string)
{
    psmove_return_val_if_fail(move != NULL, 0);

    PSMove_Data_BTAddr btaddr;

    if (!psmove_get_btaddr(move, &btaddr)) {
        return 0;
    }

    if (!psmove_btaddr_from_string(btaddr_string, &btaddr)) {
        return 0;
    }

    if (!psmove_set_btaddr(move, &btaddr)) {
        return 0;
    }

    return 1;
}

enum PSMove_Connection_Type
psmove_connection_type(PSMove *move)
{
#if defined(_WIN32)
    return move->is_bluetooth?Conn_Bluetooth:Conn_USB;
#elif defined(WITH_MOVED_CLIENT)
    return Conn_Bluetooth;
#else
    wchar_t wstr[255];
    int res;

    psmove_return_val_if_fail(move != NULL, Conn_Unknown);

    wstr[0] = 0x0000;
    res = hid_get_serial_number_string(move->handle,
            wstr, sizeof(wstr)/sizeof(wstr[0]));

    /**
     * As it turns out, we don't get a serial number when connected via USB,
     * so assume that when the serial number length is zero, then we have the
     * USB connection type, and if we have a greater-than-zero length, then it
     * is a Bluetooth connection.
     **/
    if (res == 0) {
        return Conn_USB;
    } else if (res > 0) {
        return Conn_Bluetooth;
    }

    return Conn_Unknown;
#endif
}

int
psmove_btaddr_from_string(const char *string, PSMove_Data_BTAddr *dest)
{
    int i, value;
    PSMove_Data_BTAddr tmp = {0};

    /* strlen("00:11:22:33:44:55") == 17 */
    psmove_return_val_if_fail(strlen(string) == 17, 0);

    for (i=0; i<6; i++) {
        value = strtol(string + i*3, NULL, 16);
        psmove_return_val_if_fail(value >= 0x00 && value <= 0xFF, 0);
        tmp[i] = value;
    }

    if (dest != NULL) {
        memcpy(dest, tmp, sizeof(PSMove_Data_BTAddr));
    }

    return 1;
}

void
psmove_set_leds(PSMove *move, unsigned char r, unsigned char g,
        unsigned char b)
{
    psmove_return_if_fail(move != NULL);
    move->leds.r = r;
    move->leds.g = g;
    move->leds.b = b;
}

void
psmove_set_rumble(PSMove *move, unsigned char rumble)
{
    psmove_return_if_fail(move != NULL);
    move->leds.rumble2 = 0x00;
    move->leds.rumble = rumble;
}

int
psmove_update_leds(PSMove *move)
{
    int res;

    psmove_return_val_if_fail(move != NULL, 0);

#ifdef WITH_MOVED_CLIENT
    moved_client_send(client, MOVED_REQ_WRITE, move->id, (unsigned char*)(&move->leds));
    return 1; // XXX
#else
    res = hid_write(move->handle, (unsigned char*)(&(move->leds)),
            sizeof(move->leds));
    return (res == sizeof(move->leds));
#endif
}

int
psmove_poll(PSMove *move)
{
    int res;

    psmove_return_val_if_fail(move != NULL, 0);

#ifdef PSMOVE_DEBUG
    /* store old sequence number before reading */
    int oldseq = (move->input.buttons4 & 0x0F);
#endif

#ifdef WITH_MOVED_CLIENT
    if (moved_client_send(client, MOVED_REQ_READ, move->id, NULL)) {
        memcpy((unsigned char*)(&(move->input)),
                client->read_response_buf, sizeof(move->input));
        res = sizeof(move->input); // XXX
    }
#else
    res = hid_read(move->handle, (unsigned char*)(&(move->input)),
        sizeof(move->input));
#endif

    if (res == sizeof(move->input)) {
        /* Sanity check: The first byte should be PSMove_Req_GetInput */
        psmove_return_val_if_fail(move->input.type == PSMove_Req_GetInput, 0);

        /**
         * buttons4's 4 least significant bits contain the sequence number,
         * so we add 1 to signal "success" and add the sequence number for
         * consumers to utilize the data
         **/
#ifdef PSMOVE_DEBUG
        int seq = (move->input.buttons4 & 0x0F);
        if (seq != ((oldseq + 1) % 16)) {
            fprintf(stderr, "[PSMOVE] Warning: Dropped frames (seq %d -> %d)\n",
                    oldseq, seq);
        }
#endif
        return 1 + (move->input.buttons4 & 0x0F);
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

unsigned char
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
psmove_disconnect(PSMove *move)
{
    psmove_return_if_fail(move != NULL);
#ifndef WITH_MOVED_CLIENT
    hid_close(move->handle);
#endif
    free(move);
}


