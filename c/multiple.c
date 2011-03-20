/**
 * PS Move API
 * Copyright (c) 2011 Thomas Perl <thp.io/about>
 * All Rights Reserved
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

