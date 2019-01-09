/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2014 Alexander Nitsch <nitsch@ht.tu-berlin.de>
 * Copyright (c) 2016 Thomas Perl <m@thp.io>
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


#include "psmove_port.h"
#include "psmove_sockets.h"
#include "psmove_private.h"

#include <windows.h>
#include <winsock2.h>
#include <bthsdpdef.h>
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

struct BluetoothAuthenticationCallbackState
{
    HANDLE hRadio;
    BLUETOOTH_DEVICE_INFO *deviceInfo;
    HANDLE hAuthenticationCompleteEvent;
};

static BOOL CALLBACK 
bluetooth_auth_callback(
    LPVOID pvParam, 
    PBLUETOOTH_AUTHENTICATION_CALLBACK_PARAMS pAuthCallbackParams)
{
    struct BluetoothAuthenticationCallbackState *state= (struct BluetoothAuthenticationCallbackState *)pvParam;

    BLUETOOTH_AUTHENTICATE_RESPONSE AuthRes;
    memset(&AuthRes, 0, sizeof(BLUETOOTH_AUTHENTICATE_RESPONSE));
    AuthRes.authMethod = pAuthCallbackParams->authenticationMethod;
    AuthRes.bthAddressRemote = pAuthCallbackParams->deviceInfo.Address;
    AuthRes.negativeResponse = 0;

    // Send authentication response to authenticate device
    DWORD dwRet = BluetoothSendAuthenticationResponseEx(state->hRadio, &AuthRes);
    switch (dwRet) {
        case ERROR_SUCCESS:
            // Flag the device as authenticated
            WINPAIR_DEBUG("Bluetooth device authenticated!");
            state->deviceInfo->fAuthenticated = TRUE;
            break;
        case ERROR_CANCELLED:
            WINPAIR_DEBUG("Bluetooth device denied passkey response");
            break;
        case E_FAIL:
            WINPAIR_DEBUG("Failure during authentication");
            break;
        case ERROR_NOT_READY:
            WINPAIR_DEBUG("Device not ready");
            break;
        case ERROR_INVALID_PARAMETER:
            WINPAIR_DEBUG("Invalid parameter");
            break;
        case 1244:  // TODO: Is there a named constant for this?
            WINPAIR_DEBUG("Not authenticated");
            break;
        default:
            WINPAIR_DEBUG("BluetoothSendAuthenticationResponseEx failed: %d", GetLastError());
            break;
    }

    // Signal the thread that the authentication callback completed
    if (!SetEvent(state->hAuthenticationCompleteEvent)) 
    {
        WINPAIR_DEBUG("Failed to set event: %d", GetLastError());
    }

    return TRUE;
}

static int
authenticate_controller(
    HANDLE hRadio,
    BLUETOOTH_DEVICE_INFO *device_info)
{
    int ret;

    if (device_info->fAuthenticated == 0)
    {
        struct BluetoothAuthenticationCallbackState callbackState;
        callbackState.deviceInfo= device_info;
        callbackState.hAuthenticationCompleteEvent= INVALID_HANDLE_VALUE;
        callbackState.hRadio= hRadio;

        // Register authentication callback before starting authentication request
        HBLUETOOTH_AUTHENTICATION_REGISTRATION hRegHandle = 0;
        DWORD dwRet = BluetoothRegisterForAuthenticationEx(
            device_info, &hRegHandle, &bluetooth_auth_callback, &callbackState);

        if (dwRet == ERROR_SUCCESS)
        {
            callbackState.hAuthenticationCompleteEvent = CreateEvent( 
                NULL,               // default security attributes
                TRUE,               // manual-reset event
                FALSE,              // initial state is non-signaled
                TEXT("AuthenticationCompleteEvent"));  // object name

            // Start the authentication request
            dwRet = BluetoothAuthenticateDeviceEx(
                NULL, 
                hRadio, 
                device_info, 
                NULL, 
                MITMProtectionNotRequiredBonding);

            if (dwRet == ERROR_NO_MORE_ITEMS)
            {
                WINPAIR_DEBUG("Already paired.");
                ret= 0;
            }
            else if (dwRet == ERROR_CANCELLED)
            {
                WINPAIR_DEBUG("User canceled the authentication.");
                ret= 1;
            }
            else if (dwRet == ERROR_INVALID_PARAMETER)
            {
                WINPAIR_DEBUG("Invalid parameter!");
                ret= 1;
            }
            else
            {
                // Block on authentication completing
                WaitForSingleObject(callbackState.hAuthenticationCompleteEvent, INFINITE);

                if (device_info->fAuthenticated)
                {
                    WINPAIR_DEBUG("Successfully paired.");
                    ret= 0;
                }
                else
                {
                    WINPAIR_DEBUG("Failed to authenticate!");
                    ret= 1;
                }
            }

            if (callbackState.hAuthenticationCompleteEvent != INVALID_HANDLE_VALUE)
            {
                CloseHandle(callbackState.hAuthenticationCompleteEvent);
            }
        }
        else
        {
            WINPAIR_DEBUG("BluetoothRegisterForAuthenticationEx failed!");
            ret= 1;
        }

        if (hRegHandle != 0)
        {
            BluetoothUnregisterAuthentication(hRegHandle);
            hRegHandle= 0;
        }
    }
    else
    {
        WINPAIR_DEBUG("Already authenticated.");
        ret= 0;
    }

    return ret;
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
handle_windows8_and_later(const BLUETOOTH_ADDRESS *move_addr, const BLUETOOTH_ADDRESS *radio_addr, const HANDLE hRadio, enum PSMove_Model_Type model)
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

                if (model == Model_ZCM2)
                {
                    if (authenticate_controller(hRadio, &device_info) != 0)
                    {
                        WINPAIR_DEBUG("Failed to authenticate the controller");
                        Sleep(SLEEP_BETWEEN_SCANS);
                        continue;
                    }
                }

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
handle_windows_pre8(const BLUETOOTH_ADDRESS *move_addr, const BLUETOOTH_ADDRESS *radio_addr, const HANDLE hRadio, enum PSMove_Model_Type model)
{
    int connected = 0;
    while (!connected) {
        BLUETOOTH_DEVICE_INFO device_info;
        if (get_bluetooth_device_info(hRadio, move_addr, &device_info, 0) != 0) {
            WINPAIR_DEBUG("No Bluetooth device found matching the given address");
        } else {
            if (is_move_motion_controller(&device_info)) {
                WINPAIR_DEBUG("Found Move Motion Controller matching the given address");

                if (model == Model_ZCM2)
                {
                    if (authenticate_controller(hRadio, &device_info) != 0)
                    {
                        WINPAIR_DEBUG("Failed to authenticate the controller");
                        Sleep(SLEEP_BETWEEN_SCANS);
                        continue;
                    }
                }

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


static int
windows_register_psmove(const char *move_addr_str, const BLUETOOTH_ADDRESS *radio_addr, const HANDLE hRadio, enum PSMove_Model_Type model)
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
        handle_windows8_and_later(move_addr, radio_addr, hRadio, model);
    } else {
        WINPAIR_DEBUG("Dealing with Windows version older than 8");
        handle_windows_pre8(move_addr, radio_addr, hRadio, model);
    }

    free(move_addr);

    return 0;
}


static int
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

void
psmove_port_initialize_sockets()
{
    /* "wsa" = Windows Sockets API, not a misspelling of "was" */
    static int wsa_initialized = 0;

    if (!wsa_initialized) {
        WSADATA wsa_data;
        int result = WSAStartup(MAKEWORD(1, 1), &wsa_data);
        wsa_initialized = (result == 0);
    }
}

int
psmove_port_check_pairing_permissions()
{
    // https://stackoverflow.com/a/8196291/1047040
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof( TOKEN_ELEVATION );
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }

    if (hToken) {
        CloseHandle(hToken);
    }

    if (!fRet) {
        printf("This program must be run as Administrator.\n");
    }

    return (fRet != FALSE);
}

uint64_t
psmove_port_get_time_ms()
{
    static LARGE_INTEGER startup_time = { .QuadPart = 0 };
    static LARGE_INTEGER frequency = { .QuadPart = 0 };
    LARGE_INTEGER now;

    if (frequency.QuadPart == 0) {
        if (!QueryPerformanceFrequency(&frequency)) {
            return 0;
        }
    }

    if (!QueryPerformanceCounter(&now)) {
        return 0;
    }

    /* The first time this function gets called, we init startup_time */
    if (startup_time.QuadPart == 0) {
        startup_time.QuadPart = now.QuadPart;
    }

    return (uint64_t)((now.QuadPart - startup_time.QuadPart) * 1000 / frequency.QuadPart);
}

void
psmove_port_set_socket_timeout_ms(int socket, uint32_t timeout_ms)
{
    DWORD receive_timeout = timeout_ms;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&receive_timeout, sizeof(receive_timeout));
}

void
psmove_port_sleep_ms(uint32_t duration_ms)
{
    LARGE_INTEGER ft;

    // Convert to 100 nanosecond interval, negative value indicates relative time
    ft.QuadPart = -(10ll * 1000ll * (LONGLONG)duration_ms);

    HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
    if (timer == NULL) {
        psmove_WARNING("In psmove_port_sleep_ms, CreateWaitableTimer failed (%d)\n", GetLastError());
        return;
    }
    if (!SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0)) {
        psmove_WARNING("In psmove_port_sleep_ms, SetWaitableTimer failed (%d)\n", GetLastError());
        CloseHandle(timer);
        return;
    }
    if (WaitForSingleObject(timer, INFINITE) != WAIT_OBJECT_0) {
        psmove_WARNING("In psmove_port_sleep_ms, WaitForSingleObject failed (%d)\n", GetLastError());
    }
    CloseHandle(timer);
}

void
psmove_port_close_socket(int socket)
{
    closesocket(socket);
}

char *
psmove_port_get_host_bluetooth_address()
{
    PSMove_Data_BTAddr btaddr;

    HANDLE hRadio;
    if (windows_get_first_bluetooth_radio(&hRadio) != 0 || !hRadio) {
        psmove_WARNING("Failed to find a Bluetooth radio");
        return NULL;
    }

    BLUETOOTH_RADIO_INFO radioInfo;
    radioInfo.dwSize = sizeof(BLUETOOTH_RADIO_INFO);

    if (BluetoothGetRadioInfo(hRadio, &radioInfo) != ERROR_SUCCESS) {
        psmove_CRITICAL("BluetoothGetRadioInfo");
        CloseHandle(hRadio);
        return NULL;
    }

    int i;
    for (i=0; i<6; i++) {
        btaddr[i] = radioInfo.address.rgBytes[i];
    }

    CloseHandle(hRadio);
    return _psmove_btaddr_to_string(btaddr);
}

enum PSMove_Bool
psmove_port_register_psmove(const char *addr, const char *host, enum PSMove_Model_Type model)
{
    // TODO: FIXME: If necessary, handle different controller models differently.

    // FIXME: Host is ignored for now

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

    int res = windows_register_psmove(addr, &radioInfo.address, hRadio, model);
    CloseHandle(hRadio);

    return (res == 0) ? PSMove_True : PSMove_False;
}

#if !defined(_MSC_VER)
// Based on code from: https://memset.wordpress.com/2010/10/09/inet_ntop-for-win32/
// See also: psmove_sockets.h
const char *inet_ntop(int af, const void *src, char *dst, int cnt)
{
    struct sockaddr_in srcaddr;

    memset(&srcaddr, 0, sizeof(struct sockaddr_in));
    memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));

    srcaddr.sin_family = af;
    if (WSAAddressToString((struct sockaddr*) &srcaddr, sizeof(struct sockaddr_in), 0, dst, (LPDWORD) &cnt) != 0) {
        DWORD rv = WSAGetLastError();
        psmove_WARNING("WSAAddressToString(): %lu", rv);
        return NULL;
    }
    return dst;
}
#endif
