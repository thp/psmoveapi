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

#include <stdint.h>

enum PSMoveUDPPort {
    // Legacy moved is running on port 17777
    MOVED_UDP_PORT = 17778,
};

enum PSMoveMovedCmd {
    MOVED_REQ_DISCOVER = 1,
    MOVED_REQ_COUNT_CONNECTED = 2,
    MOVED_REQ_SET_LEDS = 3,
    MOVED_REQ_READ_INPUT = 4,
    MOVED_REQ_GET_SERIAL = 5,
    MOVED_REQ_GET_HOST_BTADDR = 6,
    MOVED_REQ_REGISTER_CONTROLLER = 7,
};

#ifdef _WIN32
#define PACKED
#pragma pack(push,1)
#else
#define PACKED __attribute__ ((__packed__))
#endif

struct PACKED PSMoveMovedProtocolHeader {
    // 8-byte packet header
    uint32_t request_sequence;
    uint16_t command_id;
    uint16_t controller_id;
};

union PACKED PSMoveMovedRequest {
    struct {
        struct PSMoveMovedProtocolHeader header;

        union {
            uint8_t payload[8];

            struct {
                uint8_t data[7];
            } set_leds;

            struct {
                uint8_t btaddr[6];
            } register_controller;
        };
    };

    uint8_t bytes[16];
};

union PACKED PSMoveMovedResponse {
    struct {
        struct PSMoveMovedProtocolHeader header;

        union {
            struct {
                uint32_t count;
            } count_connected;

            struct {
                // VS requres at least one member
                uint32_t _dummy;
            } set_leds;

            struct {
                int32_t poll_return_value;
                uint8_t data[49];
            } read_input;

            struct {
                uint8_t btaddr[6];
            } get_serial;

            struct {
                uint8_t btaddr[6];
            } get_host_btaddr;
        };

        uint8_t _padding[3];
    };

    uint8_t bytes[64];
};

#ifdef _WIN32
#pragma pack(pop)
#endif

#define MOVED_HOSTS_LIST_FILE "moved2_hosts.txt"

/* Client socket timeout for receiving data (in ms) */
#define MOVED_TIMEOUT_MS 10

/* Maximum number of retries for client requests */
#define MOVED_MAX_RETRIES 5
