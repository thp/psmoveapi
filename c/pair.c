
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (C) 2011 Thomas Perl <m@thp.io>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 **/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "psmove.h"

int main(int argc, char* argv[])
{
    PSMove *move;
    enum PSMove_Connection_Type ctype;
    int i;
    int count = psmove_count_connected();
    int result = 0;

    printf("Connected controllers: %d\n", count);

    for (i=0; i<count; i++) {
        move = psmove_connect_by_id(i);

        if (move == NULL) {
            printf("Error connecting to PSMove #%d\n", i+1);
            result = 1;
            continue;
        }

        if (psmove_connection_type(move) == Conn_USB) {
            printf("PSMove #%d connected via USB. ", i+1);
            if (psmove_pair(move)) {
                printf("Pairing succeeded!\n");
            } else {
                printf("Pairing failed.\n");
            }
        } else {
            printf("Ignoring non-USB PSMove #%d\n", i+1);
        }

        psmove_disconnect(move);
    }

    return result;
}

