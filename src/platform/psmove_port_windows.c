/**
 * PS Move API - An interface for the PS Move Motion Controller
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
    // FIXME: Do we need to check for admin privileges here?
    return 1;
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
    HANDLE timer;
    LARGE_INTEGER ft;

    // Convert to 100 nanosecond interval, negative value indicates relative time
    ft.QuadPart = -(10 * 1000 * duration_ms);

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
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

void
psmove_port_register_psmove(const char *addr, const char *host)
{
    // FIXME: Host is ignored for now

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

    windows_register_psmove(addr, &radioInfo.address, hRadio);
    CloseHandle(hRadio);
}
