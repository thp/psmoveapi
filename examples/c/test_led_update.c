
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

#define BEGIN_TEST(x) fprintf(stderr, "Testing: %s", x)
#define END_TEST()    fprintf(stderr, " ... OK\n")
#define INFO(x)       fprintf(stderr, "Info: %s\n", x)

void test_succeeds(PSMove *move) {
    int i;

    BEGIN_TEST("psmove_update_leds succeeds (rate limiting disabled)");
    for (i=0; i<10; i++) {
        psmove_set_leds(move, i, i, i);
        assert(psmove_update_leds(move) == Update_Success);
    }
    END_TEST();
}

int main(int argc, char* argv[])
{
    PSMove *move = psmove_connect();
    int i;

    if (move == NULL) {
        printf("Could not connect to default Move controller.\n"
               "Please connect one via USB or Bluetooth.\n");
        exit(1);
    }

    INFO("Rate limiting is disabled by default");
    test_succeeds(move);

    INFO("Enabling rate limiting");
    psmove_set_rate_limiting(move, 1);

    BEGIN_TEST("psmove_update_leds ignores updates (rate limiting enabled)");
    for (i=0; i<10; i++) {
        psmove_set_leds(move, i, i, i);
        assert(psmove_update_leds(move) == Update_Ignored);
    }
    END_TEST();

    INFO("Disabling rate limiting");
    psmove_set_rate_limiting(move, 0);

    BEGIN_TEST("psmove_update_leds ignores updates (unchanged values)");
    psmove_set_leds(move, 1, 1, 1);
    assert(psmove_update_leds(move) == Update_Success);
    for (i=0; i<10; i++) {
        assert(psmove_update_leds(move) == Update_Ignored);
    }
    END_TEST();

    test_succeeds(move);

    psmove_disconnect(move);

    return 0;
}

