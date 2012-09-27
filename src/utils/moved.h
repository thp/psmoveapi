
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

#ifndef MOVED_H
#define MOVED_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <wchar.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <fcntl.h>
#endif

#include "../daemon/psmove_moved_protocol.h"

#include "psmove.h"


#define each(name,set) (name=set; name; name=name->next)

typedef struct _psmove_dev {
    PSMove *move;
    int assigned_id;

    unsigned char input[MOVED_SIZE_READ_RESPONSE];
    unsigned char output[7];

    int dirty_output;

    struct _psmove_dev *next;
} psmove_dev;


typedef struct _move_daemon {
    psmove_dev *devs;
    int count;
} move_daemon;


typedef struct {
    int socket;
    struct sockaddr_in server_addr;
    move_daemon *moved;
} moved_server;


/* moved_server */

moved_server *
moved_server_create();

void
moved_server_handle_request(moved_server *server);

void
moved_server_destroy(moved_server *server);


/* psmove_dev */

psmove_dev *
psmove_dev_create(move_daemon *moved, const char *path, const wchar_t *serial);

void
psmove_dev_set_output(psmove_dev *dev, const unsigned char *output);

void
psmove_dev_destroy(move_daemon *moved, psmove_dev *dev);


/* move_daemon */

move_daemon *
moved_init();

void
moved_handle_connection(move_daemon *moved, const char *path, const wchar_t *serial);

void
moved_handle_disconnect(move_daemon *moved, const char *path);

void
moved_write_reports(move_daemon *moved);

void
moved_dump_devices(move_daemon *moved);

int
moved_get_next_id(move_daemon *moved);

void
moved_destroy(move_daemon *moved);


#endif
