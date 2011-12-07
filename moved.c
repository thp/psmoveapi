
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

#include "moved.h"


/**
 * This is "moved", the Linux PS Move Daemon.
 * Inspired by the connection technique of linmctool, the move daemon
 * can keep the connection to the controller(s) open while other apps
 * connect to it. This is useful in cases where hidapi has problems.
 **/

#define LOG(format, ...) fprintf(stderr, "moved:" format, __VA_ARGS__)


int
main(int argc, char *argv[])
{
    fd_set fds;

    move_daemon *moved = moved_init();
    moved_server *server = moved_server_create();

    server->moved = moved;
    if (server->socket > moved->fdmax) {
        moved->fdmax = server->socket;
    }

    while (1) {
        /* Update the list of devices to wait on */
        if (moved->devices_changed) {
            moved_update_devices(moved, server);
        }

        /* Wait for next event */
        fds = moved->fds;
        assert(select(moved->fdmax+1, &fds, NULL, NULL, NULL) >= 0);

        if (FD_ISSET(server->socket, &fds)) {
            moved_server_handle_request(server);
        }

        /* Handle new connections */
        moved_handle_connection(moved, &fds);

        moved_read_reports(moved, &fds);
        moved_write_reports(moved);
        moved_handle_disconnect(moved);
    }

    moved_server_destroy(server);

    return 0;
}

void
set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        flags = 0;
    }

    assert(fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
}

char *
myba2str(const bdaddr_t *ba)
{
    static char buf[18];
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
            ba->b[5], ba->b[4], ba->b[3], ba->b[2], ba->b[1], ba->b[0]);
    return buf;
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
    int si_len = sizeof(si_other);
    char request[MOVED_SIZE_REQUEST];

    assert(recvfrom(server->socket, request, sizeof(request),
                0, (struct sockaddr *)&si_other, &si_len) != -1);

    switch (request[0]) {
        case MOVED_REQ_COUNT_CONNECTED:
            {
                char response[MOVED_SIZE_RESPONSE] = {0};

                psmove_dev *dev;
                int count = 0;
                for each (dev, server->moved->devs) {
                    count++;
                }

                response[0] = count;
                assert(sendto(server->socket, response, sizeof(response),
                        0, (struct sockaddr *)&si_other, si_len) != -1);
            }
            break;
        case MOVED_REQ_DISCONNECT:
            {
                int device_id = request[1];

                psmove_dev *dev;
                int count = 0;
                for each (dev, server->moved->devs) {
                    if (count == device_id) {
                        break;
                    }
                    count++;
                }

                if (dev != NULL) {
                    dev->disconnect_flag = 1;
                }
            }
            break;
        case MOVED_REQ_WRITE:
            {
                int device_id = request[1];

                psmove_dev *dev;
                int count = 0;
                for each (dev, server->moved->devs) {
                    if (count == device_id) {
                        break;
                    }
                    count++;
                }

                if (dev != NULL) {
                    psmove_dev_set_output(dev, request+2);
                }
            }
            break;
        case MOVED_REQ_READ:
            {
                char response[MOVED_SIZE_READ_RESPONSE] = {0};
                int device_id = request[1];

                psmove_dev *dev;
                int count = 0;
                for each (dev, server->moved->devs) {
                    if (count == device_id) {
                        break;
                    }
                    count++;
                }

                if (dev != NULL) {
                    const unsigned char *input = psmove_dev_get_input(dev);
                    memcpy(response, input, sizeof(dev->input)-1);
                }

                assert(sendto(server->socket, response, sizeof(response),
                        0, (struct sockaddr *)&si_other, si_len) != -1);
            }
            break;
        default:
            assert(0);
            break;
    }

#if 0
    LOG("Received packet from %s:%d\nData: %s\n\n",
            inet_ntoa(si_other.sin_addr),
            ntohs(si_other.sin_port),
            request);
#endif
}

void
moved_server_destroy(moved_server *server)
{
    close(server->socket);
    free(server);
}


int
l2cap_listen(unsigned short psm)
{
    int sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    assert(sock >= 0);

    struct sockaddr_l2 addr = {
        .l2_family = AF_BLUETOOTH,
        .l2_bdaddr = *BDADDR_ANY,
        .l2_psm = htobs(psm),
        .l2_cid = 0,
    };

    assert(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0);
    assert(listen(sock, 5) == 0);

    return sock;
}

psmove_dev *
psmove_dev_accept(move_daemon *moved)
{
    psmove_dev *dev = calloc(1, sizeof(psmove_dev));
    assert(dev != NULL);

    dev->csk = accept(moved->csk, NULL, NULL);
    dev->isk = accept(moved->isk, NULL, NULL);
    assert(dev->csk >= 0 && dev->isk >= 0);

    /* Use non-blocking I/O for the control socket (writable socket) */
    set_nonblocking(dev->csk);

    /* Header byte for LED output */
    dev->output[0] = HIDP_TRANS_SET_REPORT | HIDP_DATA_RTYPE_OUTPUT;

    struct sockaddr_l2 addr;
    socklen_t addrlen = sizeof(addr);
    assert(getpeername(dev->isk, (struct sockaddr *)&addr, &addrlen) >= 0);
    dev->addr = addr.l2_bdaddr;

    return dev;
}

void
psmove_dev_delete(psmove_dev *dev)
{
    close(dev->csk);
    close(dev->isk);
    free(dev);
}

void
psmove_dev_set_output(psmove_dev *dev, const unsigned char *output)
{
    memcpy(dev->output+1, output, sizeof(dev->output)-1);
    dev->dirty_output = 1;
}

const unsigned char *
psmove_dev_get_input(psmove_dev *dev)
{
    dev->dirty_input = 0;
    return dev->input+1;
}

int
psmove_dev_write(psmove_dev *dev)
{
    if (send(dev->csk, dev->output, sizeof(dev->output), 0) <= 0) {
        return 0;
    }

    dev->output_acks_waiting++;
    return 1;
}

int
psmove_dev_read(psmove_dev *dev)
{
    if (recv(dev->isk, dev->input, sizeof(dev->input), 0) > 0) {
        if (dev->input[0] == 0xA1) {
            dev->dirty_input = 1;
            return 1;
        }
    }

    return 0;
}

move_daemon *
moved_init()
{
    move_daemon *moved = (move_daemon *)calloc(1, sizeof(move_daemon));

    moved->devices_changed = 1;

    moved->csk = l2cap_listen(L2CAP_PSM_HIDP_CTRL);
    moved->isk = l2cap_listen(L2CAP_PSM_HIDP_INTR);
    assert(moved->csk >= 0 && moved->isk >= 0);

    moved->fdmax = (moved->csk > moved->isk)?(moved->csk):(moved->isk);

    return moved;
}

void
moved_update_devices(move_daemon *moved, moved_server *server)
{
    FD_ZERO(&(moved->fds));

    FD_SET(moved->csk, &(moved->fds));
    FD_SET(moved->isk, &(moved->fds));
    FD_SET(server->socket, &(moved->fds));

    psmove_dev *dev;
    for each(dev, moved->devs) {
        FD_SET(dev->csk, &(moved->fds));
        FD_SET(dev->isk, &(moved->fds));
    }

    moved->devices_changed = 0;
}

void
moved_handle_connection(move_daemon *moved, fd_set *fds)
{
    if (!FD_ISSET(moved->csk, fds)) {
        return;
    }

    psmove_dev *dev = psmove_dev_accept(moved);

    if (dev->csk > moved->fdmax) moved->fdmax = dev->csk;
    if (dev->isk > moved->fdmax) moved->fdmax = dev->isk;

    dev->index = moved->next_devindex++;
    dev->next = moved->devs;
    moved->devs = dev;

    LOG("New device %d %s\n", dev->index, myba2str(&dev->addr));

    moved->devices_changed = 1;
}


void
moved_read_reports(move_daemon *moved, fd_set *fds)
{
    psmove_dev *dev;

    for each(dev, moved->devs) {
        if (FD_ISSET(dev->isk, fds)) {
            psmove_dev_read(dev);
        }
    }
}

void
moved_write_reports(move_daemon *moved)
{
    psmove_dev *dev;
    char ack;

    for each(dev, moved->devs) {
        /**
         * Send new outputs for devices with "dirty" output
         *
         * The uppser limit for output_acks_waiting has been determined
         * by experimentation - it turns out we can have one additional
         * write before reading outstanding acks.
         **/
        if (dev->dirty_output && dev->output_acks_waiting <= 1) {
            psmove_dev_write(dev);
        }

        /**
         * Receive outstanding acknowledgements - ignore the
         * value in "ack", because at this point there is no
         * way of reporting an error. Sending LED/rumble data
         * is done on a best effort basis, anyway.
         **/
        while (recv(dev->csk, &ack, sizeof(ack), 0) != -1 &&
                dev->output_acks_waiting > 0) {
            dev->dirty_output = 0;
            dev->output_acks_waiting--;
        }
    }
}

void
moved_handle_disconnect(move_daemon *moved)
{
    psmove_dev *dev;
    psmove_dev *prev = NULL;

    for each(dev, moved->devs) {
        if (dev->disconnect_flag) {
            moved->devices_changed = 1;

            if (prev == NULL) {
                moved->devs = dev->next;
            } else {
                prev->next = dev->next;
            }

            LOG("Removing device %d %s\n", dev->index,
                    myba2str(&dev->addr));

            psmove_dev_delete(dev);
        }
        prev = dev;
    }
}

