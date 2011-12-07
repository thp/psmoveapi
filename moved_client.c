
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


#include "moved_client.h"

moved_client *
moved_client_create(const char *hostname)
{
    moved_client *client = (moved_client*)calloc(1, sizeof(moved_client));

    client->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(client->socket != -1);

    client->moved_addr.sin_family = AF_INET;
    client->moved_addr.sin_port = htons(MOVED_UDP_PORT);
    assert(inet_aton(hostname, &(client->moved_addr.sin_addr)) != 0);

    return client;
}

int
moved_client_send(moved_client *client, char req, char id, const unsigned char *data)
{
    client->request_buf[0] = req;
    client->request_buf[1] = id;

    if (data != NULL) {
        memcpy(client->request_buf+2, data, sizeof(client->request_buf)-2);
    }

    if (sendto(client->socket, client->request_buf,
                sizeof(client->request_buf), 0,
                (struct sockaddr *)&(client->moved_addr),
                sizeof(client->moved_addr)) >= 0)
    {
        switch (req) {
            case MOVED_REQ_COUNT_CONNECTED:
                assert(recv(client->socket, client->response_buf,
                            sizeof(client->response_buf), 0) != -1);
                return client->response_buf[0];
                break;
            case MOVED_REQ_READ:
                assert(recv(client->socket, client->read_response_buf,
                            sizeof(client->read_response_buf), 0) != -1);
                return 1;
                break;
            case MOVED_REQ_WRITE:
                return 1;
                break;
            case MOVED_REQ_DISCONNECT:
                /* No response for these? */
                return 1;
                break;
            default:
                break;
        }
    }

    return 0;
}

void
moved_client_destroy(moved_client *client)
{
    close(client->socket);
    free(client);
}

