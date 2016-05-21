
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

#include "../psmove_private.h"
#include "../psmove_port.h"



/* For now, monitoring is only supported on Linux */
#ifdef __linux

#include "../daemon/moved_monitor.h"

void
on_monitor_update(enum MonitorEvent event,
        enum MonitorEventDeviceType device_type,
        const char *path, const wchar_t *serial,
        void *user_data)
{
    moved_server *server = (moved_server*)user_data;
    move_daemon *moved = server->moved;

    if (event == EVENT_DEVICE_ADDED) {
        if (device_type == EVENT_DEVICE_TYPE_USB) {
            PSMove *move = psmove_connect_internal((wchar_t*)serial, (char*)path, -1);
            if (psmove_pair(move)) {
                // Indicate to the user that pairing was successful
                psmove_set_leds(move, 0, 255, 0);
                psmove_update_leds(move);
            } else {
                printf("Pairing failed for newly-connected USB device.\n");
            }
            psmove_disconnect(move);
        }

        moved_handle_connection(moved, path, serial);
    } else if (event == EVENT_DEVICE_REMOVED) {
        moved_handle_disconnect(moved, path);
    }
}

#endif /* __linux */



int
main(int argc, char *argv[])
{
    moved_server *server = moved_server_create();
    move_daemon *moved = moved_init(server);

#ifdef __linux
    moved_monitor *monitor = moved_monitor_new(on_monitor_update, server);
#endif

    /* Never act as a client in "moved" mode */
    psmove_set_remote_config(PSMove_OnlyLocal);

    moved_dump_devices(moved);

    int id, count = psmove_count_connected();
    for (id=0; id<count; id++) {
        moved_handle_connection(moved, NULL, NULL);
    }

#ifdef __linux
    fd_set fds;
    int server_fd = server->socket;
    int monitor_fd = moved_monitor_get_fd(monitor);
    int nfds = ((server_fd > monitor_fd)?(server_fd):(monitor_fd)) + 1;
#endif

    for (;;) {
#ifdef __linux
        FD_ZERO(&fds);
        FD_SET(server_fd, &fds);
        FD_SET(monitor_fd, &fds);

        if (select(nfds, &fds, NULL, NULL, NULL)) {
            if (FD_ISSET(monitor_fd, &fds)) {
                moved_monitor_poll(monitor);
            }

            if (FD_ISSET(server_fd, &fds))  {
                moved_server_handle_request(server);
            }
        }
#else
        moved_server_handle_request(server);
#endif

        moved_write_reports(moved);
    }

#ifdef __linux
    moved_monitor_free(monitor);
#endif

    moved_server_destroy(server);
    moved_destroy(moved);

    return 0;
}

moved_server *
moved_server_create()
{
    moved_server *server = (moved_server*)calloc(1, sizeof(moved_server));

    psmove_port_initialize_sockets();

    server->socket = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(server->socket != -1);

    server->server_addr.sin_family = AF_INET;
    server->server_addr.sin_port = htons(MOVED_UDP_PORT);
    server->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int result = bind(server->socket, (struct sockaddr *)&(server->server_addr),
                      sizeof(server->server_addr));
    (void)result;
    assert(result != -1);

    return server;
}

void
moved_server_handle_request(moved_server *server)
{
    struct sockaddr_in si_other;
    socklen_t si_len = sizeof(si_other);

    psmove_dev *dev = NULL;
    int send_response = 0;

    int request_id = -1, device_id = -1;
    unsigned char request[MOVED_SIZE_REQUEST] = {0};
    unsigned char response[MOVED_SIZE_READ_RESPONSE] = {0};

    int bytes_received = recvfrom(server->socket, (char*)request, sizeof(request),
        0, (struct sockaddr *)&si_other, &si_len);
    assert(bytes_received != -1);

    // Client disconnected
    if (bytes_received == 0)
        return;

    request_id = request[0];
    device_id = request[1];

    switch (request_id) {
        case MOVED_REQ_COUNT_CONNECTED:
            response[0] = (unsigned char)server->moved->count;

            send_response = 1;
            break;
        case MOVED_REQ_WRITE:
            for each (dev, server->moved->devs) {
                if (dev->assigned_id == device_id) {
                    break;
                }
            }

            if (dev != NULL) {
                psmove_dev_set_output(dev, request+2);
            } else {
                printf("Cannot write to device %d.\n", device_id);
            }
            break;
        case MOVED_REQ_READ:
            for each (dev, server->moved->devs) {
                if (dev->assigned_id == device_id) {
                    break;
                }
            }

            if (dev != NULL) {
                _psmove_read_data(dev->move, dev->input, sizeof(dev->input));
                memcpy(response, dev->input, sizeof(dev->input));
            } else {
                printf("Cannot read from device %d.\n", device_id);
            }

            send_response = 1;
            break;
        case MOVED_REQ_SERIAL:
            for each (dev, server->moved->devs) {
                if (dev->assigned_id == device_id) {
                    break;
                }
            }

            if (dev != NULL) {
                char *serial = psmove_get_serial(dev->move);
                memcpy(response, serial, strlen(serial)+1);
                free(serial);
            } else {
                printf("Cannot read from device %d.\n", device_id);
            }
            send_response = 1;
            break;
        default:
            printf("Unsupported call: %x - ignoring.\n", request_id);
            return;
    }

    /* Some requests need a response - send it here */
    if (send_response) {
        int result = sendto(server->socket, (char*)response, sizeof(response),
                0, (struct sockaddr *)&si_other, si_len);
        (void)result;
        assert(result != -1);
    }
}

void
moved_server_destroy(moved_server *server)
{
    close(server->socket);
    free(server);
}


psmove_dev *
psmove_dev_create(move_daemon *moved, const char *path, const wchar_t *serial)
{
    psmove_dev *dev = calloc(1, sizeof(psmove_dev));
    assert(dev != NULL);

    if (path != NULL) {
        dev->move = psmove_connect_internal((wchar_t*)serial, (char*)path, moved->count);
    } else {
        dev->move = psmove_connect_by_id(moved->count);
    }
    dev->assigned_id = moved_get_next_id(moved);
    moved->count++;

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
psmove_dev_destroy(move_daemon *moved, psmove_dev *dev)
{
    psmove_disconnect(dev->move);
    free(dev);

    moved->count--;
}


move_daemon *
moved_init(moved_server *server)
{
    move_daemon *moved = (move_daemon *)calloc(1, sizeof(move_daemon));
    server->moved = moved;
    return moved;
}

void
moved_handle_connection(move_daemon *moved, const char *path, const wchar_t *serial)
{
    psmove_dev *dev = psmove_dev_create(moved, path, serial);
    dev->next = moved->devs;
    moved->devs = dev;

    moved_dump_devices(moved);
}

void
moved_dump_devices(move_daemon *moved)
{
    printf("%d connected\n", moved->count);
#ifdef PSMOVE_DEBUG
    psmove_dev *dev;
    for each(dev, moved->devs) {
        char *serial = psmove_get_serial(dev->move);
        printf("Device %d: %s\n", dev->assigned_id, serial);
        free(serial);
    }
#endif
    fflush(stdout);
}

int
moved_get_next_id(move_daemon *moved)
{
    int next_id = 0;
    int available = 0;

    while (!available) {
        available = 1;

        psmove_dev *dev;
        for each(dev, moved->devs) {
            if (dev->assigned_id == next_id) {
                available = 0;
                next_id++;
            }
        }
    }

    return next_id;
}

void
moved_handle_disconnect(move_daemon *moved, const char *path)
{
    psmove_dev *prev = NULL;

    psmove_dev *dev;
    for each(dev, moved->devs) {
        const char *dev_path = _psmove_get_device_path(dev->move);
        if (strcmp(path, dev_path) == 0) {
            if (prev != NULL) {
                prev->next = dev->next;
            } else {
                moved->devs = dev->next;
            }

            psmove_dev_destroy(moved, dev);
            break;
        }

        prev = dev;
    }

    moved_dump_devices(moved);
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
        psmove_dev_destroy(moved, moved->devs);
        moved->devs = next_dev;
    }
    free(moved);
}

