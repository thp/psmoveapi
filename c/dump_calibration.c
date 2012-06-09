
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
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
#include <string.h>

#include "psmove.h"

int
decode(char *data, int offset)
{
    unsigned char low = data[offset] & 0xFF;
    unsigned char high = (data[offset+1]) & 0xFF;
    return (low | (high << 8)) - 0x8000;
}

void
parse_calibration_data(char *data, size_t size)
{
    int orientation;
    int x, y, z;

    /* http://code.google.com/p/moveonpc/wiki/CalibrationData */

    for (orientation=0; orientation<6; orientation++) {
        x = decode(data, 0x04 + 6*orientation);
        y = decode(data, 0x04 + 6*orientation + 2);
        z = decode(data, 0x04 + 6*orientation + 4);
        printf("# Orientation #%d: (%5d | %5d | %5d)\n", orientation, x, y, z);
    }

    for (orientation=0; orientation<3; orientation++) {
        x = decode(data, 0x46 + 8*orientation);
        y = decode(data, 0x46 + 8*orientation + 2);
        z = decode(data, 0x46 + 8*orientation + 4);
        printf("# Gyro %c, 80 rpm: (%5d | %5d | %5d)\n", "XYZ"[orientation], x, y, z);
    }

    printf("# byte at 0x3F: %02hhx\n", data[0x3F]);
}

int
main(int argc, char* argv[])
{
    PSMove *move;
    char *data;
    char *serial;
    size_t size;
    int i, j;
    int count;
    PSMove_Data_BTAddr addr;
    int parse = 0;

    /* Use "./dump_calibration parse" to parse the data */
    if (argc == 2 && strcmp(argv[1], "parse") == 0) {
        parse = 1;
    }

    if ((count = psmove_count_connected()) < 1) {
        fprintf(stderr, "No controllers connected.\n");
        return EXIT_FAILURE;
    }

    for (i=0; i<count; i++) {
        move = psmove_connect_by_id(i);

        if (move == NULL) {
            fprintf(stderr, "Could not connect to controller #%d.\n", i);
            continue;
        }

        if (psmove_connection_type(move) != Conn_USB) {
            fprintf(stderr, "Ignoring non-USB controller #%d.\n", i);
            goto next;
        }

        if (!psmove_get_calibration_blob(move, &data, &size)) {
            fprintf(stderr, "Could not read data from controller #%d.\n", i);
            goto next;
        }

        psmove_read_btaddrs(move, NULL, &addr);
        serial = psmove_btaddr_to_string(addr);
        printf("# %s\n", serial);
        free(serial);

        for (j=0; j<size; j++) {
            printf("%02hhx", data[j]);
            if (j % 16 == 15) {
                printf("\n");
            } else if (j < size-1) {
                printf(" ");
            }
        }
        printf("\n");
        if (parse) {
            parse_calibration_data(data, size);
        }
        printf("\n");

        free(data);

next:
        psmove_disconnect(move);
    }

    return EXIT_SUCCESS;
}

