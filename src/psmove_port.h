/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2016 Thomas Perl <m@thp.io>
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

#ifndef PSMOVE_PORT_H
#define PSMOVE_PORT_H

#include "psmove.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize sockets API (if required) -- call before using sockets */
void
psmove_port_initialize_sockets();

/**
 * Check if necessary permissions are available for pairing controllers
 * Returns non-zero if permissions are available, zero if not
 **/
int
psmove_port_check_pairing_permissions();

/**
 * Get the current time in milliseconds since the first call
 **/
ADDAPI uint64_t
ADDCALL psmove_port_get_time_ms();

/**
 * Sleep for a specified amount of milliseconds
 **/
ADDAPI void
ADDCALL psmove_port_sleep_ms(uint32_t duration_ms);

/**
 * Set the timeout to the specified time in milliseconds for a given socket
 **/
void
psmove_port_set_socket_timeout_ms(int socket, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* PSMOVE_PORT_H */
