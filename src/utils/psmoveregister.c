
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2011, 2016 Thomas Perl <m@thp.io>
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
#include "../psmove_private.h"
#include "../psmove_port.h"

static const char *OPT_PS4 = "--ps4";

static void
psmoveregister_usage(const char *progname)
{
    fprintf(stderr, "Usage: %s [%s] bluetooth-address\n", progname, OPT_PS4);
}

int
main(int argc, char *argv[])
{
    if (!psmove_port_check_pairing_permissions()) {
        return 1;
    }

    char *bdaddr = NULL;
    PSMove_Model_Type model = Model_ZCM1;

    for (int i=1; i<argc; ++i) {
        if (strcmp(argv[i], OPT_PS4) == 0) {
            model = Model_ZCM2;
        } else if (bdaddr == NULL) {
            bdaddr = argv[i];
        } else {
            psmoveregister_usage(argv[0]);
            fprintf(stderr, "Unrecognized command-line argument: '%s'\n", argv[i]);
            return 1;
        }
    }

    if (bdaddr == NULL) {
        psmoveregister_usage(argv[0]);
        fprintf(stderr, "No Bluetooth address supplied.\n");
        return 1;
    }

    if (psmove_host_pair_custom_model(bdaddr, model)) {
        printf("Paired\n");
    } else {
        printf("Pairing failed\n");
    }

    return 0;
}
