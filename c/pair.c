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
    int count = psmove_count_connected();
    int result = 0;

    printf("Connected controllers: %d\n", count);

    for (i=0; i<count; i++) {
        move = psmove_connect_by_id(i);

        if (move == NULL) {
            printf("Error connecting to PSMove #%d\n", i+1);
            result = 1;
            continue;
        }

        if (psmove_connection_type(move) == Conn_USB) {
            printf("PSMove #%d connected via USB. ", i+1);
            if (psmove_pair(move)) {
                printf("Pairing succeeded!\n");
            } else {
                printf("Pairing failed.\n");
            }
        } else {
            printf("Ignoring non-USB PSMove #%d\n", i+1);
        }

        psmove_disconnect(move);
    }

    return result;
}

