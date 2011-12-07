
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

#ifndef MOVED_H
#define MOVED_H


#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#include "moved_protocol.h"


#define each(name,set) (name=set; name; name=name->next)


#define L2CAP_PSM_HIDP_CTRL 0x11
#define L2CAP_PSM_HIDP_INTR 0x13

#define HIDP_TRANS_SET_REPORT    0x50
#define HIDP_DATA_RTYPE_OUTPUT   0x02


typedef struct _psmove_dev {
  int index;

  bdaddr_t addr;
  int csk;
  int isk;

  unsigned char input[50];
  unsigned char output[8];

  int dirty_output;
  int output_acks_waiting;
  int dirty_input;
  int disconnect_flag;

  struct _psmove_dev *next;
} psmove_dev;


typedef struct _move_daemon {
    psmove_dev *devs;
    int next_devindex;

    int devices_changed;
    fd_set fds;
    int fdmax;

    int csk;
    int isk;
} move_daemon;


typedef struct {
    int socket;
    struct sockaddr_in server_addr;
    move_daemon *moved;
} moved_server;


moved_server *
moved_server_create();

void
moved_server_handle_request(moved_server *server);

void
moved_server_destroy(moved_server *server);



int
l2cap_listen(unsigned short psm);


psmove_dev *
psmove_dev_accept(move_daemon *moved);

void
psmove_dev_delete(psmove_dev *dev);

void
psmove_dev_set_output(psmove_dev *dev, const unsigned char *output);

const unsigned char *
psmove_dev_get_input(psmove_dev *dev);

int
psmove_dev_write(psmove_dev *dev);

int
psmove_dev_read(psmove_dev *dev);


move_daemon *
moved_init();

void
moved_update_devices(move_daemon *moved, moved_server *server);

void
moved_handle_connection(move_daemon *moved, fd_set *fds);

void
moved_read_reports(move_daemon *moved, fd_set *fds);

void
moved_write_reports(move_daemon *moved);

void
moved_handle_disconnect(move_daemon *moved);


#endif
