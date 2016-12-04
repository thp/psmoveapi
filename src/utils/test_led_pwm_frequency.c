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

/**
 * A test program for changing the RGB LED PWM frequency
 *
 * Press the X button to cycle through a list of hard-coded frequencies. Press
 * the SQUARE button to actually use this value as new PWM frequency.
 *
 * Since the available PWM frequencies are all well above 100 Hz, there should
 * be no visible flickering. The sphere's color should appear exactly the same
 * regardless of the PWM frequency.
 */
 
#include "psmove.h"

#include <stdio.h>
#include <stdlib.h>


#define NUM_FREQS 8

unsigned long freqs[NUM_FREQS] = { 800, 2500, 5000, 10000, 153600, 230400, 307200, 384000 };


int main(int argc, char *argv[])
{
    if (!psmove_init(PSMOVE_CURRENT_VERSION)) {
        fprintf(stderr, "PS Move API init failed (wrong version?)\n");
        exit(1);
    }

    /* Check if at least one controller is attached */
    if (psmove_count_connected() < 1) {
        printf("No Move controller attached.\n");
        exit(1);
    }

    PSMove *move = psmove_connect();
    if(move == NULL) {
        printf("Could not connect to default Move controller.\n");
        exit(1);
    }

    /* Make sure we are connected via Bluetooth */
    if (psmove_connection_type(move) != Conn_Bluetooth) {
        printf("Controller must be connected via Bluetooth.\n");
        psmove_disconnect(move);
        exit(1);
    }

    unsigned int freq_idx = 0;

    while (!(psmove_get_buttons(move) & Btn_PS)) {
        if (!psmove_poll(move)) {
            continue;
        }

        unsigned int pressed_buttons;
        psmove_get_button_events(move, &pressed_buttons, NULL);

        unsigned long frequency = freqs[freq_idx];

        /* Cycle through the list of frequencies */
        if (pressed_buttons & Btn_CROSS) {
            freq_idx = (freq_idx + 1) % NUM_FREQS;
            frequency = freqs[freq_idx];
            printf("Selecting frequency %lu Hz. Press SQUARE to apply.\n", frequency);
        }

        /* Apply the currently selected frequency as new PWM frequency */
        if (pressed_buttons & Btn_SQUARE) {
            printf("Setting LED PWM frequency to %lu Hz.\n", frequency);

            /*
               Switch off the LEDs. If we do not do this, changing the PWM
               frequency will switch off the LEDs and keep them off until
               the Bluetooth connection has been teared down (at least
               once) and then reestablished.
            */
            psmove_set_leds(move, 0, 0, 0);
            psmove_update_leds(move);

            /*
               TODO:
               How can we make sure the LEDs are really off before changing
               the PWM frequency? It seems to work in all tests so far, but
               a proper verification would be nice.
            */
            if (!psmove_set_led_pwm_frequency(move, frequency)) {
                printf("Failed to set LED PWM frequency.\n");
            }
        }

        /* Keep fixed color for visual feedback */
        psmove_set_leds(move, 0, 0, 63);
        psmove_update_leds(move);
    }

    psmove_disconnect(move);

    return 0;
}

