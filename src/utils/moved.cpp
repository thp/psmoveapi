
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <wchar.h>

#include "../psmove_sockets.h"
#include "../psmove_private.h"
#include "../psmove_port.h"

#include "../daemon/psmove_moved_protocol.h"

#include "psmove.h"

#include <vector>

struct move_daemon;

struct psmove_dev {
    psmove_dev(move_daemon *moved, const char *path, const wchar_t *serial);
    ~psmove_dev();

    void set_output(const unsigned char *output);

    PSMove *move;
    int assigned_id;

    unsigned char input[MOVED_SIZE_READ_RESPONSE];
    unsigned char output[7];

    int dirty_output;
};

struct moved_server {
    moved_server();
    ~moved_server();

    void handle_request();

    int count() { return devs.size(); }
    int get_socket() { return socket; }

protected:
    int socket;
    struct sockaddr_in server_addr;
    std::vector<psmove_dev *> devs;
};

struct move_daemon : public moved_server {
    move_daemon();
    ~move_daemon();

    void handle_connection(const char *path, const wchar_t *serial);
    void handle_disconnect(const char *path);

    void write_reports();
    void dump_devices();
    int get_next_id();
};


/* For now, monitoring is only supported on Linux */
#ifdef __linux

#include "../daemon/moved_monitor.h"

static void
on_monitor_update_moved(enum MonitorEvent event,
        enum MonitorEventDeviceType device_type,
        const char *path, const wchar_t *serial,
        void *user_data)
{
    move_daemon *moved = static_cast<move_daemon *>(user_data);

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

        moved->handle_connection(path, serial);
    } else if (event == EVENT_DEVICE_REMOVED) {
        moved->handle_disconnect(path);
    }
}

#endif /* __linux */



int
main(int argc, char *argv[])
{
    move_daemon moved;

#ifdef __linux
    moved_monitor *monitor = moved_monitor_new(on_monitor_update_moved, &moved);
#endif

    /* Never act as a client in "moved" mode */
    psmove_set_remote_config(PSMove_OnlyLocal);

    moved.dump_devices();

    int id, count = psmove_count_connected();
    for (id=0; id<count; id++) {
        moved.handle_connection(NULL, NULL);
    }

#ifdef __linux
    fd_set fds;
    int server_fd = moved.get_socket();
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
                moved.handle_request();
            }
        }
#else
        moved.handle_request();
#endif

        moved.write_reports();
    }

#ifdef __linux
    moved_monitor_free(monitor);
#endif

    return 0;
}

moved_server::moved_server()
{
    psmove_port_initialize_sockets();

    socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(socket != -1);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MOVED_UDP_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socket, (struct sockaddr *)&(server_addr), sizeof(server_addr)) == -1) {
        fprintf(stderr, "Could not bind to UDP port %d", MOVED_UDP_PORT);
        exit(1);
    }
}

void
moved_server::handle_request()
{
    struct sockaddr_in si_other;
    socklen_t si_len = sizeof(si_other);

    psmove_dev *dev = NULL;
    int send_response = 0;

    int request_id = -1, device_id = -1;
    unsigned char request[MOVED_SIZE_REQUEST] = {0};
    unsigned char response[MOVED_SIZE_READ_RESPONSE] = {0};

    int bytes_received = recvfrom(socket, (char*)request, sizeof(request),
        0, (struct sockaddr *)&si_other, &si_len);
    assert(bytes_received != -1);

    // Client disconnected
    if (bytes_received == 0)
        return;

    request_id = request[0];
    device_id = request[1];

    for (psmove_dev *dev: devs) {
        if (dev->assigned_id == device_id) {
            break;
        }
    }

    if (dev != NULL && dev->assigned_id != device_id) {
        dev = NULL;
    }

    switch (request_id) {
        case MOVED_REQ_COUNT_CONNECTED:
            response[0] = (unsigned char)count();

            send_response = 1;
            break;
        case MOVED_REQ_WRITE:
            if (dev != NULL) {
                dev->set_output(request+2);
            } else {
                printf("Cannot write to device %d.\n", device_id);
            }
            break;
        case MOVED_REQ_READ:
            if (dev != NULL) {
                _psmove_read_data(dev->move, dev->input, sizeof(dev->input));
                memcpy(response, dev->input, sizeof(dev->input));
            } else {
                printf("Cannot read from device %d.\n", device_id);
            }

            send_response = 1;
            break;
        case MOVED_REQ_SERIAL:
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
        int result = sendto(socket, (char*)response, sizeof(response),
                0, (struct sockaddr *)&si_other, si_len);
        (void)result;
        assert(result != -1);
    }
}

moved_server::~moved_server()
{
    psmove_port_close_socket(socket);
}


psmove_dev::psmove_dev(move_daemon *moved, const char *path, const wchar_t *serial)
{
    if (path != NULL) {
        move = psmove_connect_internal((wchar_t *)serial, (char *)path, moved->count());
    } else {
        move = psmove_connect_by_id(moved->count());
    }

    assigned_id = moved->get_next_id();

    psmove_set_rate_limiting(move, PSMove_False);
}

void
psmove_dev::set_output(const unsigned char *output)
{
    memcpy(this->output, output, sizeof(this->output));
    dirty_output++;
}

psmove_dev::~psmove_dev()
{
    psmove_disconnect(move);
}


move_daemon::move_daemon()
    : moved_server()
{
}

void
move_daemon::handle_connection(const char *path, const wchar_t *serial)
{
    devs.push_back(new psmove_dev(this, path, serial));
    dump_devices();
}

void
move_daemon::dump_devices()
{
    printf("%d connected\n", count());
#ifdef PSMOVE_DEBUG
    for (psmove_dev *dev: devs) {
        char *serial = psmove_get_serial(dev->move);
        printf("Device %d: %s\n", dev->assigned_id, serial);
        free(serial);
    }
#endif
    fflush(stdout);
}

int
move_daemon::get_next_id()
{
    int next_id = 0;
    int available = 0;

    while (!available) {
        available = 1;

        for (psmove_dev *dev: devs) {
            if (dev->assigned_id == next_id) {
                available = 0;
                next_id++;
            }
        }
    }

    return next_id;
}

void
move_daemon::handle_disconnect(const char *path)
{
    std::vector<psmove_dev *>::iterator it;
    for (it = devs.begin(); it != devs.end(); ++it) {
        const char *dev_path = _psmove_get_device_path((*it)->move);
        if (strcmp(path, dev_path) == 0) {
            delete (*it);
            it = devs.erase(it);
            break;
        }
    }

    dump_devices();
}

void
move_daemon::write_reports()
{
    for (psmove_dev *dev: devs) {
        /* Send new outputs for devices with "dirty" output */
        if (dev->dirty_output) {
            _psmove_write_data(dev->move, dev->output, sizeof(dev->output));
            dev->dirty_output = 0;
        }
    }
}

move_daemon::~move_daemon()
{
    for (psmove_dev *dev: devs) {
        delete dev;
    }
}
