
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
    int i, c;

    c = psmove_count_connected();
    printf("Connected controllers: %d\n", c);

    for (i=0; i<c; i++) {
        move = psmove_connect_by_id(i);
        switch (i) {
            case 0:
                psmove_set_leds(move, 255, 0, 0);
                break;
            case 1:
                psmove_set_leds(move, 0, 255, 0);
                break;
            case 2:
                psmove_set_leds(move, 0, 0, 255);
                break;
            case 3:
                psmove_set_leds(move, 255, 255, 0);
                break;
            case 4:
                psmove_set_leds(move, 0, 255, 255);
                break;
            case 5:
                psmove_set_leds(move, 255, 0, 255);
                break;
            default:
                psmove_set_leds(move, 255, 255, 255);
                break;
        }
        psmove_update_leds(move);
        psmove_disconnect(move);
    }

    return 0;
}

