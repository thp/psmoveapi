
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

#include "moved.h"

#define LOG(format, ...) fprintf(stderr, "moved:" format, __VA_ARGS__)


int
main(int argc, char *argv[])
{
    moved_server *server = moved_server_create();
    move_daemon *moved = moved_init(server);

    /* Never act as a client in "moved" mode */
    psmove_disable_remote();

    int id, count=psmove_count_connected();
    LOG("%d devices connected\n", count);
    for (id=0; id<count; id++) {
        moved_handle_connection(moved, id);
    }

    while (1) {
        moved_server_handle_request(server);
        moved_write_reports(moved);
    }

    moved_server_destroy(server);
    moved_destroy(moved);

    return 0;
}

moved_server *
moved_server_create()
{
    moved_server *server = (moved_server*)calloc(1, sizeof(moved_server));

    server->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(server->socket != -1);

    server->server_addr.sin_family = AF_INET;
    server->server_addr.sin_port = htons(MOVED_UDP_PORT);
    server->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    assert(bind(server->socket, (struct sockaddr *)&(server->server_addr),
                sizeof(server->server_addr)) != -1);

    return server;
}

void
moved_server_handle_request(moved_server *server)
{
    struct sockaddr_in si_other;
    unsigned int si_len = sizeof(si_other);

    psmove_dev *dev = NULL;
    int count = 0;
    int send_response = 0;

    int request_id = -1, device_id = -1;
    unsigned char request[MOVED_SIZE_REQUEST] = {0};
    unsigned char response[MOVED_SIZE_READ_RESPONSE] = {0};

    assert(recvfrom(server->socket, request, sizeof(request),
                0, (struct sockaddr *)&si_other, &si_len) != -1);

    request_id = request[0];
    device_id = request[1];

    switch (request_id) {
        case MOVED_REQ_COUNT_CONNECTED:
            for each (dev, server->moved->devs) {
                count++;
            }
            response[0] = count;

            send_response = 1;
            break;
        case MOVED_REQ_WRITE:
            for each (dev, server->moved->devs) {
                if (count == device_id) {
                    break;
                }
                count++;
            }

            if (dev != NULL) {
                psmove_dev_set_output(dev, request+2);
            } else {
                LOG("Cannot write to device %d.\n", device_id);
            }
            break;
        case MOVED_REQ_READ:
            for each (dev, server->moved->devs) {
                if (count == device_id) {
                    break;
                }
                count++;
            }

            if (dev != NULL) {
                _psmove_read_data(dev->move, dev->input, sizeof(dev->input));
                memcpy(response, dev->input, sizeof(dev->input));
            } else {
                LOG("Cannot read from device %d.\n", device_id);
            }

            send_response = 1;
            break;
        default:
            LOG("Unsupported call: %x - ignoring.\n", request_id);
            return;
    }

    /* Some requests need a response - send it here */
    if (send_response) {
        assert(sendto(server->socket, response, sizeof(response),
                0, (struct sockaddr *)&si_other, si_len) != -1);
    }
}

void
moved_server_destroy(moved_server *server)
{
    close(server->socket);
    free(server);
}


psmove_dev *
psmove_dev_create(int id)
{
    psmove_dev *dev = calloc(1, sizeof(psmove_dev));
    assert(dev != NULL);

    dev->move = psmove_connect_by_id(id);
    psmove_set_rate_limiting(dev->move, 0);
    return dev;
}

void
psmove_dev_set_output(psmove_dev *dev, const unsigned char *output)
{
    memcpy(dev->output, output, sizeof(dev->output));
    dev->dirty_output = 1;
}

void
psmove_dev_destroy(psmove_dev *dev)
{
    psmove_disconnect(dev->move);
    free(dev);
}


move_daemon *
moved_init(moved_server *server)
{
    move_daemon *moved = (move_daemon *)calloc(1, sizeof(move_daemon));
    server->moved = moved;
    return moved;
}

void
moved_handle_connection(move_daemon *moved, int id)
{
    psmove_dev *dev = psmove_dev_create(id);
    dev->next = moved->devs;
    moved->devs = dev;

    LOG("New device %d\n", id);
}


void
moved_write_reports(move_daemon *moved)
{
    psmove_dev *dev;

    for each(dev, moved->devs) {
        /* Send new outputs for devices with "dirty" output */
        if (dev->dirty_output) {
            _psmove_write_data(dev->move, dev->output, sizeof(dev->output));
            dev->dirty_output = 0;
        }
    }
}

void
moved_destroy(move_daemon *moved)
{
    while (moved->devs != NULL) {
        psmove_dev *next_dev = moved->devs->next;
        psmove_dev_destroy(moved->devs);
        moved->devs = next_dev;
    }
    free(moved);
}

