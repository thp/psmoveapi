
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2011, 2017 Thomas Perl <m@thp.io>
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
#include "../daemon/moved_client.h"


int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage: %s [hostname]\n", argv[0]);
        return 1;
    }

    // Only remote pair locally-connected controllers
    psmove_set_remote_config(PSMove_OnlyLocal);

    moved_client *client = moved_client_create(argv[1]);

    while (!moved_client_send(client, MOVED_REQ_GET_HOST_BTADDR, 0, nullptr, 0)) {
        printf("Waiting for remote Bluetooth address...\n");
    }

    PSMove_Data_BTAddr *new_host_btaddr = (PSMove_Data_BTAddr *)client->response_buf.get_host_btaddr.btaddr;
    char *new_host = _psmove_btaddr_to_string(*new_host_btaddr);
    printf("Bluetooth host address of '%s' is '%s'\n", argv[1], new_host);

    int count = psmove_count_connected();
    printf("Connected controllers: %d\n", count);

    int result = 0;
    for (int i=0; i<count; i++) {
        PSMove *move = psmove_connect_by_id(i);

        if (move == NULL) {
            printf("Error connecting to PSMove #%d\n", i+1);
            result = 1;
            continue;
        }

        if (psmove_connection_type(move) != Conn_Bluetooth) {
            printf("PSMove #%d connected via USB.\n", i+1);

            if (_psmove_set_btaddr(move, new_host_btaddr)) {
                printf("Paired controller to new host %s via USB.\n", new_host);

                char *serial = psmove_get_serial(move);
                PSMove_Data_BTAddr btaddr;
                if (!_psmove_btaddr_from_string(serial, &btaddr)) {
                    printf("Could not parse serial of controller\n");
                    return 1;
                }

                printf("Controller address: %s\n", serial);

                int retries = 0;
                while (retries < 5 && !moved_client_send(client, MOVED_REQ_REGISTER_CONTROLLER, 0, btaddr, sizeof(btaddr))) {
                    printf("Waiting for server to accept registration...\n");
                    retries++;
                }

                printf("Server has accepted pairing.\n");

                free(serial);
            } else {
                printf("Failed to write new host address via USB.\n");
            }
        } else {
            printf("Ignoring non-USB PSMove #%d\n", i+1);
        }

        psmove_disconnect(move);
    }

    free(new_host);

    moved_client_destroy(client);

    return result;
}
