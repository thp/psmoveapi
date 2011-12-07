
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
#include <curses.h>

#include "psmove.h"

#define INFO_OUT(name, format, ...) {\
    printw(name "=");\
    attron(COLOR_PAIR(2));\
    printw(format, __VA_ARGS__);\
    attroff(COLOR_PAIR(2));\
    printw(" ");\
}

int main(int argc, char* argv[])
{
    PSMove *move;
    enum PSMove_Connection_Type ctype;
    int i;

    initscr();
    start_color();

    curs_set(0);
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);

    i = psmove_count_connected();
    attron(COLOR_PAIR(1));
    printw("Connected controllers: %d\n", i);
    attroff(COLOR_PAIR(1));

    move = psmove_connect();

    if (move == NULL) {
        endwin();
        printf("Could not connect to default Move controller.\n"
               "Please connect one via USB or Bluetooth.\n");
        exit(1);
    }

    ctype = psmove_connection_type(move);
    switch (ctype) {
        case Conn_USB:
            printw("Connected via USB.\n");
            break;
        case Conn_Bluetooth:
            printw("Connected via Bluetooth.\n");
            break;
        case Conn_Unknown:
            printw("Unknown connection type.\n");
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
        /* This is the easy method (pair to this host): */
        if (psmove_pair(move)) {
            printf("Paired. Press the PS Button now :)\n");
        } else {
            printf("psmove_pair() failed :/\n");
        }

        /* This is the advanced method: */

        /* Example BT Address: 01:23:45:67:89:ab */
        const PSMove_Data_BTAddr newhost = {
            0x01, 0x23, 0x45, 0x67, 0x89, 0xab,
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
        move(2, 2);
        int res = psmove_poll(move);
        if (res) {
            if (psmove_get_buttons(move) & Btn_TRIANGLE) {
                psmove_set_rumble(move, psmove_get_trigger(move));
            } else {
                psmove_set_rumble(move, 0x00);
            }

            psmove_set_leds(move, 0, 0, psmove_get_trigger(move));
            printw("    ");

            INFO_OUT("trig", "%3d", psmove_get_trigger(move));

            int x, y, z;
            psmove_get_accelerometer(move, &x, &y, &z);
            INFO_OUT("acc", "%7d %7d %7d", x, y, z);

#if 0
            psmove_get_gyroscope(move, &x, &y, &z);
            INFO_OUT("gyro", "%5d %5d %5d", x, y, z);
            psmove_get_magnetometer(move, &x, &y, &z);
            INFO_OUT("mag", "%10d %10d %10d", x, y, z);
#endif

            INFO_OUT("but", "%6x", psmove_get_buttons(move));

            int battery = psmove_get_battery(move);

            if (battery == Batt_CHARGING) {
                INFO_OUT("bat", "%s", "chg");
            } else if (battery >= Batt_MIN && battery <= Batt_MAX) {
                INFO_OUT("bat", "%d/%d", battery, Batt_MAX);
            } else {
                INFO_OUT("bat", "%x?", battery);
            }

            INFO_OUT("temp", "%d", psmove_get_temperature(move));
            printw("    ");

            psmove_update_leds(move);
            refresh();
        }
    }
    endwin();

    psmove_disconnect(move);

    return 0;
}

