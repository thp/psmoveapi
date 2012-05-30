
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2011, 2012 Thomas Perl <m@thp.io>
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
#include <assert.h>

#include "psmove.h"

enum TestMode {
    NO_UPDATES,
    ALL_UPDATES,
    SMART_UPDATES,
    STATIC_UPDATES,
};

void test_read_performance(PSMove *move, enum TestMode mode) {
    int round, max_rounds = 3;
    float sum = 0.;

    /* Only enable rate limiting if we test for smart updates */
    psmove_set_rate_limiting(move, mode == SMART_UPDATES);

    if (mode == STATIC_UPDATES) {
        psmove_set_leds(move, 0, 255, 0);
    }

    for (round=0; round<max_rounds; round++) {
        long packet_loss = 0;
        long started = psmove_util_get_ticks();
        long reads = 0;
        int old_sequence = -1, sequence;

        while (reads < 1000) {
            if (mode != NO_UPDATES) {
                if (mode != STATIC_UPDATES) {
                    psmove_set_leds(move, reads%255, reads%255, reads%255);
                }

                psmove_update_leds(move);
            }

            if (reads % 20 == 0) {
                fprintf(stderr, "\r%c", "-\\|/"[(reads/20)%4]);
            }

            while (!(sequence = psmove_poll(move))) {}

            if (old_sequence != -1) {
                if (sequence != ((old_sequence % 16) + 1)) {
                    packet_loss++;
                    /*fprintf(stderr, " %d->%d", old_sequence, sequence);*/
                    fputc('x', stderr);
                }
            }
            old_sequence = sequence;

            reads++;
        }
        fputc('\r', stderr);
        long diff = psmove_util_get_ticks() - started;

        double reads_per_second = 1000. * (double)reads / (double)diff;
        printf("%ld reads in %ld ms = %f reads/sec "
                "(%ldx seq jump = %.2f %%)\n",
                reads, diff, reads_per_second, packet_loss,
                100. * (double)packet_loss / (double)reads);
        sum += reads_per_second;
    }

    printf("=====\n");
    printf("Mean over %d rounds: %f reads/sec\n", max_rounds,
            sum/(double)max_rounds);
}

int main(int argc, char* argv[])
{
    PSMove *move = psmove_connect();

    if (move == NULL) {
        printf("Could not connect to default Move controller.\n"
               "Please connect one via USB or Bluetooth.\n");
        exit(1);
    }

    printf("\n -- PS Move API Sensor Reading Performance Test -- \n");

    printf("\nTesting STATIC READ performance (non-changing LED setting)\n");
    test_read_performance(move, STATIC_UPDATES);

    printf("\nTesting SMART READ performance (rate-limited LED setting)\n");
    test_read_performance(move, SMART_UPDATES);

    printf("\nTesting BAD READ performance (continous LED setting)\n");
    test_read_performance(move, ALL_UPDATES);

    printf("\nTesting RAW READ performance (no LED setting)\n");
    test_read_performance(move, NO_UPDATES);

    printf("\n");

    return 0;
}


