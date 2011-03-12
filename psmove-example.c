/**
 * PS Move API
 * Copyright (c) 2011 Thomas Perl <thp.io/about>
 * All Rights Reserved
 **/

#include <unistd.h>

#include "psmove.h"

int main(int argc, char* argv[])
{
    PSMove *move;
    enum PSMove_Connection_Type ctype;

    move = PSMove_connect();

    ctype = PSMove_connection_type(move);
    switch (ctype) {
        case PSMove_USB:
            printf("Connected via USB.\n");
            break;
        case PSMove_Bluetooth:
            printf("Connected via Bluetooth.\n");
            break;
        case PSMove_Unknown:
            printf("Unknown connection type.\n");
            break;
    }


    if (ctype == PSMove_USB) {
        PSMove_Data_BTAddr addr;
        PSMove_get_btaddr(move, &addr);
        printf("Current BT Host: ");
        for (int i=0; i<6; i++) {
            printf("%02x ", addr[i]);
        }
        printf("\n");

#if 0
        /* Example BT Address: 01:23:45:67:89:ab */
        const PSMove_Data_BTAddr newhost = {
            0xab, 0x89, 0x67, 0x45, 0x23, 0x01
        };
        if (!PSMove_set_btaddr(move, &newhost)) {
            printf("Could not set BT address!\n");
        }
#endif
    }

    for (int i=0; i<10; i++) {
        PSMove_set_leds(move, 0, 255*(i%3==0), 0);
        PSMove_set_rumble(move, 255*(i%2));
        PSMove_update_leds(move);
        usleep(10000*(i%10));
    }

    for (int i=250; i>=0; i-=5) {
        PSMove_set_leds(move, i, i, 0);
        PSMove_set_rumble(move, 0);
        PSMove_update_leds(move);
    }

    PSMove_set_leds(move, 0, 0, 0);
    PSMove_set_rumble(move, 0);
    PSMove_update_leds(move);

    while (!(PSMove_get_buttons(move) & PSMove_PS)) {
        int res = PSMove_poll(move);
        if (res) {
            if (PSMove_get_buttons(move) & PSMove_TRIANGLE) {
                printf("Triangle pressed, with trigger value: %d\n",
                        PSMove_get_trigger(move));
                PSMove_set_rumble(move, PSMove_get_trigger(move));
            } else {
                PSMove_set_rumble(move, 0x00);
            }

            PSMove_set_leds(move, 0, 0, PSMove_get_trigger(move));

            int x, y, z;
            PSMove_get_accelerometer(move, &x, &y, &z);
            printf("accel: %5d %5d %5d\n", x, y, z);
            PSMove_get_gyroscope(move, &x, &y, &z);
            printf("gyro: %5d %5d %5d\n", x, y, z);
            PSMove_get_magnetometer(move, &x, &y, &z);
            printf("magnetometer: %5d %5d %5d\n", x, y, z);

            PSMove_update_leds(move);
        }
    }

    PSMove_disconnect(move);

    return 0;
}

