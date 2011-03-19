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
    enum PSMove_Connection_Type ctype;
    int i;

    move = psmove_connect();

    if (move == NULL) {
        printf("Move controller not found.\n"
               "Please connect one via USB or Bluetooth.\n");
        exit(1);
    }

    ctype = psmove_connection_type(move);
    switch (ctype) {
        case Conn_USB:
            printf("Connected via USB.\n");
            break;
        case Conn_Bluetooth:
            printf("Connected via Bluetooth.\n");
            break;
        case Conn_Unknown:
            printf("Unknown connection type.\n");
            break;
    }


    if (ctype == Conn_USB) {
        PSMove_Data_BTAddr addr;
        psmove_get_btaddr(move, &addr);
        printf("Current BT Host: ");
        for (i=0; i<6; i++) {
            printf("%02x ", addr[i]);
        }
        printf("\n");

#if 0
        /* Example BT Address: 01:23:45:67:89:ab */
        const PSMove_Data_BTAddr newhost = {
            0xab, 0x89, 0x67, 0x45, 0x23, 0x01
        };
        if (!psmove_set_btaddr(move, &newhost)) {
            printf("Could not set BT address!\n");
        }
#endif
    }

    for (i=0; i<10; i++) {
        psmove_set_leds(move, 0, 255*(i%3==0), 0);
        psmove_set_rumble(move, 255*(i%2));
        psmove_update_leds(move);
        usleep(10000*(i%10));
    }

    for (i=250; i>=0; i-=5) {
        psmove_set_leds(move, i, i, 0);
        psmove_set_rumble(move, 0);
        psmove_update_leds(move);
    }

    psmove_set_leds(move, 0, 0, 0);
    psmove_set_rumble(move, 0);
    psmove_update_leds(move);

    while (!(psmove_get_buttons(move) & Btn_PS)) {
        int res = psmove_poll(move);
        if (res) {
            if (psmove_get_buttons(move) & Btn_TRIANGLE) {
                printf("Triangle pressed, with trigger value: %d\n",
                        psmove_get_trigger(move));
                psmove_set_rumble(move, psmove_get_trigger(move));
            } else {
                psmove_set_rumble(move, 0x00);
            }

            psmove_set_leds(move, 0, 0, psmove_get_trigger(move));

            int x, y, z;
            psmove_get_accelerometer(move, &x, &y, &z);
            printf("accel: %5d %5d %5d\n", x, y, z);
            psmove_get_gyroscope(move, &x, &y, &z);
            printf("gyro: %5d %5d %5d\n", x, y, z);
            psmove_get_magnetometer(move, &x, &y, &z);
            printf("magnetometer: %5d %5d %5d\n", x, y, z);
            printf("buttons: %x\n", psmove_get_buttons(move));

            psmove_update_leds(move);
        }
    }

    psmove_disconnect(move);

    return 0;
}

