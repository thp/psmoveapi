
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2011 Thomas Perl <m@thp.io>
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




#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "psmove.h"
#include "../psmove_private.h"

int main(int argc, char* argv[])
{
    PSMove *move;
    int i;
    int count = psmove_count_connected();
    int result = 0;
    int custom_addr = 0;

    if (argc > 1) {
        if (_psmove_btaddr_from_string(argv[1], NULL)) {
            printf("Using user-supplied host address: %s\n", argv[1]);
        } else {
            printf("Cannot convert host address: %s\n", argv[1]);
            return 1;
        }
        custom_addr = 1;
    }

#ifdef __linux
    /**
     * In order to be able to start/stop bluetoothd and to
     * add new entries to the Bluez configuration files, we
     * need to run as root (platform/psmove_linuxsupport.c)
     **/
    if (!custom_addr && geteuid() != 0) {
        printf("This program must be run as root (or use sudo).\n");
        return 1;
    }
#endif

    printf("Connected controllers: %d\n", count);

    for (i=0; i<count; i++) {
        move = psmove_connect_by_id(i);

        if (move == NULL) {
            printf("Error connecting to PSMove #%d\n", i+1);
            result = 1;
            continue;
        }

        if (psmove_connection_type(move) != Conn_Bluetooth) {
            printf("PSMove #%d connected via USB.\n", i+1);
            int result = 0;

            if (custom_addr) {
                result = psmove_pair_custom(move, argv[1]);
            } else {
                result = psmove_pair(move);
            }

            if (result) {
                printf("Pairing of #%d succeeded!\n", i+1);
                char *serial = psmove_get_serial(move);
                printf("Controller address: %s\n", serial);
                free(serial);
            } else {
                printf("Pairing of #%d failed.\n", i+1);
            }

            if (psmove_has_calibration(move)) {
                printf("Calibration data available and saved.\n");
            } else {
                printf("Error reading/loading calibration data.\n");
            }
        } else {
            printf("Ignoring non-USB PSMove #%d\n", i+1);
        }

        psmove_disconnect(move);
    }

    return result;
}

