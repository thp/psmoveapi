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

#include <windows.h>

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
