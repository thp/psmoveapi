#pragma once
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


#include "psmove.h"


#ifdef __APPLE__
#  include <unistd.h>
#  include <sys/syslimits.h>
#  include <sys/stat.h>
#  include <sys/poll.h>
#endif

#ifdef __linux
#  include <unistd.h>
#  include <linux/limits.h>
#  include <sys/poll.h>
#endif

#ifdef _WIN32
#  include <windows.h>
#ifndef PATH_MAX
#  define PATH_MAX MAX_PATH
#endif
#  define ENV_USER_HOME "APPDATA"
#  define PATH_SEP "\\"
#else
#  define ENV_USER_HOME "HOME"
#  define PATH_SEP "/"
#endif

#if defined(_MSC_VER) && !defined(__cplusplus)
// MSVC only supports the "inline" keyword in C++, but not in C (C89),
// so if we're on MSVC and we're not compiling C++ code, define this.
#define inline __inline
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* Initialize sockets API (if required) -- call before using sockets */
ADDAPI void
ADDCALL psmove_port_initialize_sockets();

/**
 * Check if necessary permissions are available for pairing controllers
 * Returns non-zero if permissions are available, zero if not
 **/
ADDAPI int
ADDCALL psmove_port_check_pairing_permissions();

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
ADDAPI void
ADDCALL psmove_port_set_socket_timeout_ms(int socket, uint32_t timeout_ms);

/**
 * Close a socket
 **/
ADDAPI void
ADDCALL psmove_port_close_socket(int socket);

/**
 * Get the default Bluetooth host controller address (result must be freed)
 **/
ADDAPI char *
ADDCALL psmove_port_get_host_bluetooth_address();

/**
 * Add the PS Move Bluetooth controller address to the system database
 **/
ADDAPI enum PSMove_Bool
ADDCALL psmove_port_register_psmove(const char *addr, const char *host, enum PSMove_Model_Type model);

#ifdef __cplusplus
}
#endif
