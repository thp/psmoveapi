/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2014 Alexander Nitsch <nitsch@ht.tu-berlin.de>
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

#include "../psmove_private.h"
#include "psmove_winsupport.h"

#include <bluetoothapis.h>
#include <tchar.h>
#include <strsafe.h>

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* define flag that seems to be missing in MinGW */
#ifndef BLUETOOTH_SERVICE_ENABLE
#    define BLUETOOTH_SERVICE_ENABLE 0x01
#endif


#define WINPAIR_DEBUG(msg, ...) \
        psmove_DEBUG(msg "\n", ## __VA_ARGS__)


/* the number of successive checks that we require to be sure the Bluetooth
 * connection is indeed properly established
 */
#define CONN_CHECK_NUM_TRIES 5

/* the delay (in milliseconds) between consecutive checks for a properly
 * established Bluetooth connection
 */
#define CONN_CHECK_DELAY 300

/* number of connection retries before removing the controller (in software)
 * and starting all over again
 */
#define CONN_RETRIES 80

/* the delay (in milliseconds) between connection retries
 */
#define CONN_DELAY 300

/* time out indicator for a Bluetooth inquiry
 */
#define GET_BT_DEVICES_TIMEOUT_MULTIPLIER 1

// Every x loop issue a new inquiry
#define BT_SCAN_NEW_INQUIRY 5

// Sleep value between bt device scan
// Recommondation: Value should be higher than GET_BT_DEVICES_TIMEOUT_MULTIPLIER * 1.28 * 1000
#define SLEEP_BETWEEN_SCANS (unsigned int) GET_BT_DEVICES_TIMEOUT_MULTIPLIER * 1.28 * 1000 * 1.1


static BLUETOOTH_ADDRESS *
string_to_btaddr(const char *str)
{
    /* check input's length */
    if (strlen(str) != 17) {
        return NULL;
    }

    /* allocate return buffer */
    BLUETOOTH_ADDRESS *addr = (BLUETOOTH_ADDRESS *) calloc(1, sizeof(BLUETOOTH_ADDRESS));
    if (!addr) {
        return NULL;
    }

    const char *nptr = str;
    unsigned int i;

    for (i = 0; i < 6; i++) {
        char *endptr = NULL;
        addr->rgBytes[5-i] = (BYTE) strtol(nptr, &endptr, 16);

        /* we require blocks to be composed of exactly two hexadecimal
         * digits and to be separated by a double colon
         */
        if (((i < 5) && (*endptr != ':')) || (endptr - nptr != 2)) {
            free(addr);
            return NULL;
        }

        /* continue with the character following the separator */
        nptr = endptr + 1;
    }

    return addr;
}


static int
set_up_bluetooth_radio(const HANDLE hRadio)
{
    /* NOTE: Order matters for the following two operations: The radio must
     *       allow incoming connections prior to being made discoverable.
     */

    if (!BluetoothIsConnectable(hRadio)) {
        WINPAIR_DEBUG("Making radio accept incoming connections");
        if (BluetoothEnableIncomingConnections(hRadio, TRUE) == FALSE) {
            WINPAIR_DEBUG("Failed to enable incoming connections");
            return 1;
        }
    }

    if (!BluetoothIsDiscoverable(hRadio)) {
        WINPAIR_DEBUG("Making radio discoverable");
        if (BluetoothEnableDiscovery(hRadio, TRUE) == FALSE) {
            WINPAIR_DEBUG("Failed to make radio discoverable");
            return 1;
        }
    }

    if ((BluetoothIsConnectable(hRadio) != FALSE) && (BluetoothIsDiscoverable(hRadio) != FALSE)) {
        return 0;
    } else {
        return 1;
    }
}


static int
get_bluetooth_device_info(const HANDLE hRadio, const BLUETOOTH_ADDRESS *addr, BLUETOOTH_DEVICE_INFO *device_info, unsigned int inquire)
{
    if (!addr || !device_info) {
        return 1;
    }

    BLUETOOTH_DEVICE_SEARCH_PARAMS search_params;
    search_params.dwSize               = sizeof(search_params);
    search_params.cTimeoutMultiplier   = GET_BT_DEVICES_TIMEOUT_MULTIPLIER;
    search_params.fIssueInquiry        = inquire;
    search_params.fReturnAuthenticated = TRUE;
    search_params.fReturnConnected     = TRUE;
    search_params.fReturnRemembered    = TRUE;
    search_params.fReturnUnknown       = TRUE;
    search_params.hRadio               = hRadio;

    device_info->dwSize = sizeof(*device_info);

    HBLUETOOTH_DEVICE_FIND hFind = BluetoothFindFirstDevice(&search_params, device_info);

    if (!hFind) {
        if (GetLastError() != ERROR_NO_MORE_ITEMS) {
            WINPAIR_DEBUG("Failed to enumerate devices");
            return 1;
        }
    } else {
        do {
            /* check if the device's Bluetooth address matches the one we are looking for */
            if (device_info->Address.ullLong == addr->ullLong) {
                break;
            }
        } while(BluetoothFindNextDevice(hFind, device_info));

        if (!BluetoothFindDeviceClose(hFind)) {
            WINPAIR_DEBUG("Failed to close device enumeration handle");
            return 1;
        }
    }

    return 0;
}


static int
is_move_motion_controller(const BLUETOOTH_DEVICE_INFO *device_info)
{
    return wcscmp(device_info->szName, L"Motion Controller") == 0;
}


static int
is_hid_service_enabled(const HANDLE hRadio, BLUETOOTH_DEVICE_INFO *device_info)
{
    /* retrieve number of installed services */
    DWORD num_services = 0;
    DWORD result = BluetoothEnumerateInstalledServices(hRadio, device_info, &num_services, NULL);
    if (result != ERROR_SUCCESS) {
        /* NOTE: Sometimes we get ERROR_MORE_DATA, sometimes we do not.
         *       The number of services seems to be correct in any case, so
         *       we will just ignore this.
         */
        if (result != ERROR_MORE_DATA) {
            WINPAIR_DEBUG("Failed to count installed services");
            return 0;
        }
    }

    if (num_services == 0) {
        return 0;
    }

    /* retrieve actual list of installed services */
    GUID *service_list = (GUID *) calloc(num_services, sizeof(GUID));
    if (!service_list) {
        return 0;
    }
    result = BluetoothEnumerateInstalledServices(hRadio, device_info, &num_services, service_list);
    if (result != ERROR_SUCCESS) {
        WINPAIR_DEBUG("Failed to enumerate installed services");
        return 0;
    }

    /* check if the HID service is part of that list */
    unsigned int i;
    int found = 0;
    GUID service = HumanInterfaceDeviceServiceClass_UUID;
    for (i = 0; i < num_services; i++) {
        if (IsEqualGUID(&service_list[i], &service)) {
            found = 1;
            break;
        }
    }

    free(service_list);

    return found;
}


static int
update_device_info(const HANDLE hRadio, BLUETOOTH_DEVICE_INFO *device_info)
{
    DWORD result = BluetoothGetDeviceInfo(hRadio, device_info);
    if (result != ERROR_SUCCESS) {
        WINPAIR_DEBUG("Failed to read device info");
        return 1;
    }

    return 0;
}


static int
is_connection_established(const HANDLE hRadio, BLUETOOTH_DEVICE_INFO *device_info)
{
    /* NOTE: Sometimes the Bluetooth connection appears to be established
     *       even though the Move decided that it is not really connected
     *       yet. That is why we cannot simply stop trying to connect after
     *       the first successful check. Instead, we require a minimum
     *       number of successive successful checks to be sure.
     */

    unsigned int i;
    for (i = 0; i < CONN_CHECK_NUM_TRIES; i++) {
        /* read device info again to check if we have a connection */
        if (update_device_info(hRadio, device_info) != 0) {
            return 0;
        }

        if (device_info->fConnected && device_info->fRemembered && is_hid_service_enabled(hRadio, device_info)) {
        } else {
            return 0;
        }

        Sleep(CONN_CHECK_DELAY);
    }

    return 1;
}


static int
patch_registry(const BLUETOOTH_ADDRESS *move_addr, const BLUETOOTH_ADDRESS *radio_addr)
{
    int ret = 0;

    TCHAR sub_key[1024];
    HRESULT res = StringCchPrintf(
        sub_key,
        1024,
        _T("SYSTEM\\CurrentControlSet\\Services\\HidBth\\Parameters\\Devices\\" \
           "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"),
        radio_addr->rgBytes[5], radio_addr->rgBytes[4], radio_addr->rgBytes[3],
        radio_addr->rgBytes[2], radio_addr->rgBytes[1], radio_addr->rgBytes[0],
        move_addr->rgBytes[5], move_addr->rgBytes[4], move_addr->rgBytes[3],
        move_addr->rgBytes[2], move_addr->rgBytes[1], move_addr->rgBytes[0] );

    if (FAILED(res)) {
        WINPAIR_DEBUG("Failed to build registry subkey");
        return 1;
    }

    /* open registry key for modifying a value */
    HKEY hKey;
    LONG result;
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sub_key, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &hKey);
    if (result != ERROR_SUCCESS) {
        if (result == ERROR_FILE_NOT_FOUND) {
            WINPAIR_DEBUG("Failed to open registry key, it does not yet exist");
        } else {
            WINPAIR_DEBUG("Failed to open registry key");
        }

        return 1;
    }

    DWORD data = 1;
    result = RegSetValueEx(hKey, _T("VirtuallyCabled"), 0, REG_DWORD, (const BYTE *) &data, sizeof(data));
    if (result != ERROR_SUCCESS) {
        WINPAIR_DEBUG("Failed to set 'VirtuallyCabled'");
        ret = 1;
    }

    RegCloseKey(hKey);

    return ret;
}


static int
is_windows8_or_later()
{
    OSVERSIONINFOEX info;
    memset(&info, 0, sizeof(info));
    info.dwOSVersionInfoSize = sizeof(info);
    info.dwMajorVersion    = 6;
    info.dwMinorVersion    = 2;
    info.wServicePackMajor = 0;
    info.wServicePackMinor = 0;

    DWORDLONG conditionMask = 0;
    VER_SET_CONDITION(conditionMask, VER_MAJORVERSION,     VER_GREATER_EQUAL);
    VER_SET_CONDITION(conditionMask, VER_MINORVERSION,     VER_GREATER_EQUAL);
    VER_SET_CONDITION(conditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
    VER_SET_CONDITION(conditionMask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);

    return VerifyVersionInfo(
        &info, 
        VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        conditionMask);
}


static void
handle_windows8_and_later(const BLUETOOTH_ADDRESS *move_addr, const BLUETOOTH_ADDRESS *radio_addr, const HANDLE hRadio)
{
    unsigned int scan = 0;
    int connected = 0;
    while (!connected) {
        BLUETOOTH_DEVICE_INFO device_info;
        if (get_bluetooth_device_info(hRadio, move_addr, &device_info, scan == 0) != 0) {
            WINPAIR_DEBUG("No Bluetooth device found matching the given address");
        } else {
            if (is_move_motion_controller(&device_info)) {
                WINPAIR_DEBUG("Found Move Motion Controller matching the given address");

                unsigned int conn_try;
                for (conn_try = 1; conn_try <= CONN_RETRIES; conn_try++) {
                    WINPAIR_DEBUG("Connection try %d/%d", conn_try, CONN_RETRIES);

                    if (update_device_info(hRadio, &device_info) != 0) {
                        break;
                    }

                    if (device_info.fConnected) {
                        /* Windows 8 (and later) seems to require manual help with setting up
                         * the device in the registry.
                         */
                        WINPAIR_DEBUG("Patching the registry ...");
                        if (patch_registry(move_addr, radio_addr) != 0) {
                            WINPAIR_DEBUG("Failed to patch the registry");
                        }

                        /* enable HID service only if necessary */
                        WINPAIR_DEBUG("Checking HID service ...");
                        if (!is_hid_service_enabled(hRadio, &device_info)) {
                            WINPAIR_DEBUG("Enabling HID service ...");
                            GUID service = HumanInterfaceDeviceServiceClass_UUID;
                            DWORD result = BluetoothSetServiceState(hRadio, &device_info, &service, BLUETOOTH_SERVICE_ENABLE);
                            if (result != ERROR_SUCCESS) {
                                WINPAIR_DEBUG("Failed to enable HID service");
                            }

                            WINPAIR_DEBUG("Patching the registry ...");
                            if (patch_registry(move_addr, radio_addr) != 0) {
                                WINPAIR_DEBUG("Failed to patch the registry");
                            }
                        }

                        WINPAIR_DEBUG("Verifying successful connection ...");
                        if (is_connection_established(hRadio, &device_info)) {
                            /* if we have a connection, stop trying to connect this device */
                            printf("Connection verified.\n");
                            connected = 1;
                            break;
                        }
                    }
                
                    Sleep(CONN_DELAY);
                }

                if(!device_info.fConnected) {
                    BluetoothRemoveDevice(&(device_info.Address));
                    WINPAIR_DEBUG("Device removed, starting all over again");
                }
            } else {
                WINPAIR_DEBUG("Bluetooth device matching the given address is not a Move Motion Controller");
            }
        }

        Sleep(SLEEP_BETWEEN_SCANS);
        scan = (scan + 1) % BT_SCAN_NEW_INQUIRY;
    }
}


static void
handle_windows_pre8(const BLUETOOTH_ADDRESS *move_addr, const BLUETOOTH_ADDRESS *radio_addr, const HANDLE hRadio)
{
    int connected = 0;
    while (!connected) {
        BLUETOOTH_DEVICE_INFO device_info;
        if (get_bluetooth_device_info(hRadio, move_addr, &device_info, 0) != 0) {
            WINPAIR_DEBUG("No Bluetooth device found matching the given address");
        } else {
            if (is_move_motion_controller(&device_info)) {
                WINPAIR_DEBUG("Found Move Motion Controller matching the given address");

                if (device_info.fConnected) {
                    /* enable HID service only if necessary */
                    WINPAIR_DEBUG("Checking HID service ...");
                    if (!is_hid_service_enabled(hRadio, &device_info)) {
                        WINPAIR_DEBUG("Enabling HID service ...");
                        GUID service = HumanInterfaceDeviceServiceClass_UUID;
                        DWORD result = BluetoothSetServiceState(hRadio, &device_info, &service, BLUETOOTH_SERVICE_ENABLE);
                        if (result != ERROR_SUCCESS) {
                            WINPAIR_DEBUG("Failed to enable HID service");
                        }
                    }

                    WINPAIR_DEBUG("Verifying successful connection ...");
                    if (is_connection_established(hRadio, &device_info)) {
                        /* if we have a connection, stop trying to connect this device */
                        printf("Connection verified.\n");
                        connected = 1;
                        break;
                    }
                }
            } else {
                WINPAIR_DEBUG("Bluetooth device matching the given address is not a Move Motion Controller");
            }
        }

        Sleep(SLEEP_BETWEEN_SCANS);
    }
}


int
windows_register_psmove(const char *move_addr_str, const BLUETOOTH_ADDRESS *radio_addr, const HANDLE hRadio)
{
    /* parse controller's Bluetooth device address string */
    BLUETOOTH_ADDRESS *move_addr = string_to_btaddr(move_addr_str);
    if (!move_addr) {
        WINPAIR_DEBUG("Cannot parse controller address: '%s'", move_addr_str);
        return 1;
    }

    if (!radio_addr) {
        WINPAIR_DEBUG("Invalid Bluetooth device address for radio");
        return 1;
    }

    if (set_up_bluetooth_radio(hRadio) != 0) {
        WINPAIR_DEBUG("Failed to configure Bluetooth radio for use");
        return 1;
    }

    printf("\n" \
           "    Unplug the controller.\n" \
           "\n"
           "    Now press the controller's PS button. The red status LED\n" \
           "    will start blinking. Whenever it goes off, press the\n" \
           "    PS button again. Repeat this until the status LED finally\n" \
           "    remains lit. Press Ctrl+C to cancel anytime.\n");

    if (is_windows8_or_later()) {
        WINPAIR_DEBUG("Dealing with Windows 8 or later");
        handle_windows8_and_later(move_addr, radio_addr, hRadio);
    } else {
        WINPAIR_DEBUG("Dealing with Windows version older than 8");
        handle_windows_pre8(move_addr, radio_addr, hRadio);
    }

    free(move_addr);

    return 0;
}


int
windows_get_first_bluetooth_radio(HANDLE *hRadio)
{
    if (!hRadio) {
        return 1;
    }

    BLUETOOTH_FIND_RADIO_PARAMS radio_params;
    radio_params.dwSize = sizeof(BLUETOOTH_FIND_RADIO_PARAMS);

    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&radio_params, hRadio);

    if (!hFind) {
        WINPAIR_DEBUG("Failed to enumerate Bluetooth radios");
        return 1;
    }

    BluetoothFindRadioClose(hFind);

    return 0;
}


