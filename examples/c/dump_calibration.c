
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psmove.h"

int
main(int argc, char* argv[])
{
    PSMove *move;
    int i;
    int count;

    if ((count = psmove_count_connected()) < 1) {
        fprintf(stderr, "No controllers connected.\n");
        return EXIT_FAILURE;
    }

    for (i=0; i<count; i++) {
        move = psmove_connect_by_id(i);

        if (move == NULL) {
            fprintf(stderr, "Could not connect to controller #%d.\n", i);
            continue;
        }

        psmove_dump_calibration(move);
        psmove_disconnect(move);
    }

    return EXIT_SUCCESS;
}

