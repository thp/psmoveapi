#pragma once

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




#include "psmove.h"

#include "../psmove_sockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "psmove_moved_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *hostname;

    int socket;
    struct sockaddr_in moved_addr;

    union PSMoveMovedRequest request_buf;
    union PSMoveMovedResponse response_buf;
} moved_client;

typedef struct _moved_client_list {
    moved_client *client;
    struct _moved_client_list *next;
} moved_client_list;

ADDAPI moved_client_list *
ADDCALL moved_client_list_open();

ADDAPI void
ADDCALL moved_client_list_destroy(moved_client_list *client_list);

ADDAPI moved_client *
ADDCALL moved_client_create(const char *hostname);

ADDAPI int
ADDCALL moved_client_send(moved_client *client, enum PSMoveMovedCmd cmd, int controller_id,
        const uint8_t *data, size_t len);

ADDAPI void
ADDCALL moved_client_destroy(moved_client *client);

#ifdef __cplusplus
}
#endif
