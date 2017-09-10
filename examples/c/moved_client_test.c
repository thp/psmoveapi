
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


#include "../../src/daemon/moved_client.h"

int main(int argc, char *argv[])
{
    moved_client *client = moved_client_create("127.0.0.1");

    if (moved_client_send(client, MOVED_REQ_COUNT_CONNECTED, 0, NULL, 0) == 0) {
        printf("Could not send count connected\n");
    }

    int i;

    int connected = client->response_buf.count_connected.count;

    printf("Connected: %d\n", connected);
    unsigned char output[] = {2, 0, 255, 255, 0, 0, 0};

    for (i=0; i<connected; i++) {
        printf("Writing to dev %d...\n", i);
        moved_client_send(client, MOVED_REQ_SET_LEDS, i, output, sizeof(output));
    }

    if (moved_client_send(client, MOVED_REQ_READ_INPUT, 0, NULL, 0)) {
        printf("====================\n");
        for (i=0; i<sizeof(client->response_buf); i++) {
            printf("%02x ", client->response_buf.bytes[i]);
        }
        printf("\n====================\n");
    }

    moved_client_destroy(client);
    return 0;
}

