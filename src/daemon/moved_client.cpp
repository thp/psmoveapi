
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
#include "../psmove_private.h"
#include "../psmove_port.h"
#include "../psmove_sockets.h"
#include "moved_client.h"

#include <map>
#include <string>
#include <algorithm>

moved_client_list *
moved_client_list_insert(moved_client_list *list, moved_client *client)
{
    moved_client_list *result = (moved_client_list*)calloc(1,
            sizeof(moved_client_list));

    result->client = client;
    result->next = list;

    return result;
}

static moved_client_list *
moved_client_list_discover(moved_client_list *result)
{
    int fd = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (fd == -1) {
        return nullptr;
    }

    psmove_port_set_socket_timeout_ms(fd, 10);

    int enabled = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char *)&enabled, sizeof(enabled));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MOVED_UDP_PORT);
    addr.sin_addr.s_addr = INADDR_BROADCAST;

    union PSMoveMovedRequest request;
    union PSMoveMovedRequest response;

    memset(&request, 0, sizeof(request));
    request.header.request_sequence = 0x47110000;
    request.header.command_id = MOVED_REQ_DISCOVER;
    request.header.controller_id = 0;

    for (int i=0; i<5; i++) {
        request.header.request_sequence++;

        int res = sendto(fd, (const char *)&request, sizeof(request),
                /*flags=*/0, (struct sockaddr *)&addr, sizeof(addr));

        if (res == -1) {
            psmove_WARNING("Could not send discovery request");
        }

        psmove_port_sleep_ms(20);
    }

    psmove_port_sleep_ms(50);

    std::map<std::string, bool> discovered;

    int failures = 0;
    while (failures < 10) {
        struct sockaddr_in addr_server;
        socklen_t addr_server_len = sizeof(addr_server);
        memset(&addr_server, 0, sizeof(addr_server));
        addr_server.sin_family = AF_INET;

        int res = recvfrom(fd, (char *)&response, sizeof(response), /*flags=*/0,
                (struct sockaddr *)&addr_server, &addr_server_len);

        if (res == sizeof(response)) {
            char temp[1024];
            inet_ntop(addr.sin_family, &addr_server.sin_addr, temp, sizeof(temp));
            std::string hostname(temp);
            discovered[hostname] = true;
            psmove_DEBUG("Discovered daemon: '%s' (seq=0x%08x)\n", temp, response.header.request_sequence);
            failures = 0;
        } else {
            failures++;
        }
    }

    psmove_port_close_socket(fd);

    for (auto it: discovered) {
        const char *hostname = it.first.c_str();
        printf("Using remote host '%s' (from auto-discovery)\n", hostname);
        moved_client *client = moved_client_create(hostname);
        result = moved_client_list_insert(result, client);
        if (moved_client_send(client, MOVED_REQ_GET_HOST_BTADDR, 0, nullptr, 0)) {
            char *serial = _psmove_btaddr_to_string(*((PSMove_Data_BTAddr *)client->response_buf.get_host_btaddr.btaddr));
            printf("Bluetooth host address of '%s' is '%s'\n", hostname, serial);
            free(serial);
        }
    }
    return result;
}

moved_client_list *
moved_client_list_open()
{
    moved_client_list *result = NULL;
    char hostname[255];
    FILE *fp;

    char *filename = psmove_util_get_file_path(MOVED_HOSTS_LIST_FILE);

    fp = fopen(filename, "r");
    if (fp != NULL) {
        while (fgets(hostname, sizeof(hostname), fp) != NULL) {
            char *end = hostname + strlen(hostname) - 1;
            if (*end == '\n' || *end == '\r') {
                *end = '\0';
            }

            if (strcmp(hostname, "*") == 0) {
                printf("Found hostname '*', starting auto-discovery.\n");
                result = moved_client_list_discover(result);
                continue;
            }

            printf("Using remote host '%s' (from %s)\n",
                    hostname, MOVED_HOSTS_LIST_FILE);
            result = moved_client_list_insert(result,
                    moved_client_create(hostname));
        }
        fclose(fp);
    }
    free(filename);

    /* XXX: Read from config file */
    //result = moved_client_list_insert(result, moved_client_create("localhost"));
    return result;
}

void
moved_client_list_destroy(moved_client_list *client_list)
{
    moved_client_list *cur = client_list;
    moved_client_list *prev = NULL;

    while (cur != NULL) {
        moved_client_destroy(cur->client);

        prev = cur;
        cur = cur->next;
        free(prev);
    }
}

moved_client *
moved_client_create(const char *hostname)
{
    psmove_port_initialize_sockets();

    moved_client *client = (moved_client*)calloc(1, sizeof(moved_client));

    client->request_buf.header.request_sequence = 1;

    client->hostname = strdup(hostname);

    client->socket = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(client->socket != -1);

    psmove_port_set_socket_timeout_ms(client->socket, MOVED_TIMEOUT_MS);

    client->moved_addr.sin_family = AF_INET;
    client->moved_addr.sin_port = htons(MOVED_UDP_PORT);
    client->moved_addr.sin_addr.s_addr = inet_addr(hostname);

    // If hostname failed to convert to an address, it's probably an actual name, so try to resolve the name to IP
    if (client->moved_addr.sin_addr.s_addr == INADDR_NONE)
    {
        struct hostent *remoteHost = gethostbyname(hostname);

        if (remoteHost->h_addrtype == AF_INET) 
            client->moved_addr.sin_addr.s_addr = *(u_long *)remoteHost->h_addr_list[0];
    }

    //assert(client->moved_addr.sin_addr.s_addr != INADDR_NONE);

    return client;
}

int
moved_client_send(moved_client *client, enum PSMoveMovedCmd cmd, int controller_id,
        const uint8_t *data, size_t len)
{
    int retry_count = 0;

    client->request_buf.header.request_sequence++;
    client->request_buf.header.command_id = (uint16_t)cmd;
    client->request_buf.header.controller_id = (uint16_t)controller_id;

    if (data != NULL) {
        memset(client->request_buf.payload, 0, sizeof(client->request_buf.payload));
        memcpy(client->request_buf.payload, data, std::min(len, sizeof(client->request_buf.payload)));
    }

    while (retry_count < MOVED_MAX_RETRIES) {
        int res = sendto(client->socket, (const char *)client->request_buf.bytes, sizeof(client->request_buf),
                /*flags=*/0, (struct sockaddr *)&(client->moved_addr), sizeof(client->moved_addr));

        if (res == sizeof(client->request_buf)) {
            while (1) {
                res = recv(client->socket, (char *)client->response_buf.bytes, sizeof(client->response_buf),
                        /*flags=*/0);

                if (res != sizeof(client->response_buf)) {
                    break;
                }

                if (client->request_buf.header.request_sequence == client->response_buf.header.request_sequence) {
                    break;
                } else {
                    // Invalid sequence number received, retry reading a (newer?) packet
                }
            }

            if (res != sizeof(client->response_buf)) {
                if (client->request_buf.header.command_id == MOVED_REQ_SET_LEDS) {
                    // Writing LEDs should be fast, do not wait for a response here
                    return 0;
                }

                retry_count++;
                continue;
            }

            return 1;
        }
    }

    psmove_WARNING("Command %d to %s, controller %d failed (errno=%d)\n",
            (int)cmd, client->hostname, controller_id, errno);

    return 0;
}

void
moved_client_destroy(moved_client *client)
{
    psmove_port_close_socket(client->socket);
    free(client);
}

