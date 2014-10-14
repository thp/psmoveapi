/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2014 Alexander Nitsch <nitsch@ht.tu-berlin.de>
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

#include "psmove.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


/* device IDs for Sony's official extension devices  */
#define EXT_DEVICE_ID_SHARP_SHOOTER 0x8081
#define EXT_DEVICE_ID_RACING_WHEEL  0x8101

enum Extension_Device {
    Ext_Sharp_Shooter,
    Ext_Racing_Wheel,
    Ext_Unknown,
};


void handle_sharp_shooter(PSMove_Ext_Data *data, unsigned int move_buttons)
{
    /**
     * For a detailed device documentation and a guide for accessing button values, see
     * https://code.google.com/p/moveonpc/wiki/SharpShooter
     **/

    assert(data != NULL);

    unsigned char const c = (*data)[0];

    static unsigned char const weapon_list[] = { '?', '1', '2', '?', '3', '?', '?', '?' };
    static unsigned char last_weapon_idx = 0;

    unsigned char const weapon_idx = c & 7;
    unsigned char const weapon = weapon_list[weapon_idx];

    if (weapon_idx != last_weapon_idx) {
        printf("SELECTING weapon %c\n", weapon);
        last_weapon_idx = weapon_idx;
    }

    if (c & 0x40) {
        printf("FIRING weapon %c\n", weapon);
    } else {
        if (move_buttons & Btn_T) {
            printf("PUMPING\n");
        }
    }

    if (c & 0x80) {
        printf("RELOADING weapon %c\n", weapon);
    }
}

void handle_racing_wheel(PSMove_Ext_Data *data, unsigned int buttons)
{
    /**
     * For a detailed device documentation and a guide for accessing button values, see
     * https://code.google.com/p/moveonpc/wiki/RacingWheel
     **/

    assert(data != NULL);

    unsigned char throttle = (*data)[0];
    unsigned char l2       = (*data)[1];
    unsigned char r2       = (*data)[2];
    unsigned char c        = (*data)[3];

    printf("Throttle: %3d,  L2: %3d,  R2: %3d", throttle, l2, r2);

    if (c & 1) {
        printf(",  LEFT paddle pressed");
    }

    if (c & 2) {
        printf(",  RIGHT paddle pressed");
    }

    printf("\n");
}

int main(int argc, char *argv[])
{
    if (!psmove_init(PSMOVE_CURRENT_VERSION)) {
        fprintf(stderr, "PS Move API init failed (wrong version?)\n");
        exit(1);
    }

    /* check if at least one controller is attached */
    if (psmove_count_connected() < 1) {
        printf("No Move controller attached.\n");
        exit(1);
    }

    PSMove *move = psmove_connect();
    if(move == NULL) {
        printf("Could not connect to default Move controller.\n");
        exit(1);
    }

    /* make sure we are connected via Bluetooth */
    if (psmove_connection_type(move) != Conn_Bluetooth) {
        printf("Controller must be connected via Bluetooth.\n");
        psmove_disconnect(move);
        exit(1);
    }

    int ext_connected = 0;
    enum Extension_Device ext_device = Ext_Unknown;

    while (!(psmove_get_buttons(move) & Btn_PS)) {
        if (!psmove_poll(move)) {
            continue;
        }

        if(psmove_is_ext_connected(move)) {
            /* if the extension device was not connected before, report connect */
            if (!ext_connected) {
                PSMove_Ext_Device_Info ext;
                enum PSMove_Bool success = psmove_get_ext_device_info(move, &ext);

                if (success) {
                    switch (ext.dev_id) {
                        case EXT_DEVICE_ID_SHARP_SHOOTER:
                            ext_device = Ext_Sharp_Shooter;
                            printf("Sharp Shooter extension connected!\n");
                            break;
                        case EXT_DEVICE_ID_RACING_WHEEL:
                            ext_device = Ext_Racing_Wheel;
                            printf("Racing Wheel extension connected!\n");
                            break;
                        default:
                            ext_device = Ext_Unknown;
                            printf("Unknown extension device (id 0x%04X) connected!\n", ext.dev_id);
                            break;
                    }
                }
                else {
                    printf("Unknown extension device connected! Failed to get device info.\n");
                }
            }

            ext_connected = 1;

            /* handle button presses etc. for a known extension device only */
            PSMove_Ext_Data data;
            if (psmove_get_ext_data(move, &data)) {
                unsigned int move_buttons = psmove_get_buttons(move);

                switch (ext_device) {
                    case Ext_Sharp_Shooter:
                        handle_sharp_shooter(&data, move_buttons);
                        break;
                    case Ext_Racing_Wheel:
                        handle_racing_wheel(&data, move_buttons);
                        break;
                    default:
                        break;
                }
            }
        } else {
            /* if the extension device was connected before, report disconnect */
            if (ext_connected) {
                printf("Extension device disconnected!\n");
            }

            ext_connected = 0;
        }
    }

    psmove_disconnect(move);

    return 0;
}

