
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2013 Thomas Perl <m@thp.io>
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
#include <string.h>

#include "psmove.h"
#include "../psmove_private.h"


#define I_KNOW_WHAT_I_DO "--i-know-what-i-do"
#define OPT_BTDFU        "--btdfu"

int main(int argc, char* argv[])
{
    PSMove *move;
    enum PSMove_Operation_Mode mode = Mode_STDFU;

    if (argc < 2 || strcmp(argv[1], I_KNOW_WHAT_I_DO) != 0) {
        printf("Usage: %s <options>\n\n", argv[0]);
        printf("This utility puts your controller via USB in a special operation\n" \
               "mode. You probably don't want to use this utility. If you know\n" \
               "what you are doing, use the option '%s'.\n\n", I_KNOW_WHAT_I_DO);
        printf("By default, the controller is put in STDFU mode. Use the option\n" \
               "'%s' to put it in BTDFU mode instead.\n\n", OPT_BTDFU);
        printf("This could damage your controller! If it breaks, it's your fault.\n");
        return 1;
    }
    
    if (argc == 3 && strcmp(argv[2], OPT_BTDFU) == 0) {
        mode = Mode_BTDFU;
    }

    if (psmove_count_connected() == 0) {
        printf("No controllers connected.\n");
        return 2;
    }

    move = psmove_connect();
    if (move == NULL) {
        printf("Cannot connect to controller.\n");
        return 3;
    }

    if (psmove_connection_type(move) == Conn_USB) {
        printf("Serial: %s\n", psmove_get_serial(move));
        if (_psmove_set_operation_mode(move, mode)) {
            printf("DFU Mode activated.\n");
            printf("To exit, press the reset button on the back of the controller.\n");
        } else {
            printf("Could not set DFU mode.\n");
        }
    }
    else {
        printf("Please connect the controller via USB.\n");
    }

    psmove_disconnect(move);
    return 0;
}
