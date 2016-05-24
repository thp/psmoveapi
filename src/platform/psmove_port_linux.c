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

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void
psmove_port_initialize_sockets()
{
    // Nothing to do on Linux
}

int
psmove_port_check_pairing_permissions()
{
    /**
     * In order to be able to start/stop bluetoothd and to
     * add new entries to the Bluez configuration files, we
     * need to run as root (platform/psmove_linuxsupport.c)
     **/
    if (geteuid() != 0) {
        printf("This program must be run as root (or use sudo).\n");
        return 0;
    }

    return 1;
}

uint64_t
psmove_port_get_time_ms()
{
    static uint64_t startup_time = 0;
    uint64_t now;
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    now = (ts.tv_sec * 1000 + ts.tv_nsec / (1000 * 1000));

    /* The first time this function gets called, we init startup_time */
    if (startup_time == 0) {
        startup_time = now;
    }

    return (now - startup_time);
}

void
psmove_port_set_socket_timeout_ms(int socket, uint32_t timeout_ms)
{
    struct timeval receive_timeout = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000,
    };
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&receive_timeout, sizeof(receive_timeout));
}

void
psmove_port_sleep_ms(uint32_t duration_ms)
{
    usleep(duration_ms * 1000);
}
