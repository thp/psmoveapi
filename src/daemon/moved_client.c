
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
#include "moved_client.h"

moved_client_list *
moved_client_list_insert(moved_client_list *list, moved_client *client)
{
    moved_client_list *result = (moved_client_list*)calloc(1,
            sizeof(moved_client_list));

    result->client = client;
    result->next = list;

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
#ifdef _WIN32
    /* "wsa" = Windows Sockets API, not a misspelling of "was" */
    static int wsa_initialized = 0;

    if (!wsa_initialized) {
        WSADATA wsa_data;
        assert(WSAStartup(MAKEWORD(1, 1), &wsa_data) == 0);
        wsa_initialized = 1;
    }
#endif

    moved_client *client = (moved_client*)calloc(1, sizeof(moved_client));

    client->hostname = strdup(hostname);

    client->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(client->socket != -1);

    /**
     * The receiving socket must have a timeout to not block indefinitely
     *
     * With Berkeley sockets, SO_RCVTIMEO takes a struct timeval, whereas
     * Microsoft's WinSock takes a DWORD containing a milliseconds value.
     **/
#ifdef _WIN32
    DWORD receive_timeout = MOVED_TIMEOUT_MS;
#else
    struct timeval receive_timeout = {
        .tv_sec = MOVED_TIMEOUT_MS / 1000,
        .tv_usec = (MOVED_TIMEOUT_MS % 1000) * 1000,
    };
#endif
    assert(setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO,
                &receive_timeout, sizeof(receive_timeout)) == 0);

    client->moved_addr.sin_family = AF_INET;
    client->moved_addr.sin_port = htons(MOVED_UDP_PORT);
#ifdef _WIN32
    client->moved_addr.sin_addr.s_addr = inet_addr(hostname);
    assert(client->moved_addr.sin_addr.s_addr != INADDR_NONE);
#else
    assert(inet_pton(AF_INET, hostname, &(client->moved_addr.sin_addr)) != 0);
#endif

    return client;
}

int
moved_client_send(moved_client *client, char req, char id, const unsigned char *data)
{
    int retry_count = 0;

    client->request_buf[0] = req;
    client->request_buf[1] = id;

    if (data != NULL) {
        memcpy(client->request_buf+2, data, sizeof(client->request_buf)-2);
    }

    while (retry_count < MOVED_MAX_RETRIES) {
        if (sendto(client->socket, client->request_buf,
                    sizeof(client->request_buf), 0,
                    (struct sockaddr *)&(client->moved_addr),
                    sizeof(client->moved_addr)) >= 0)
        {
            switch (req) {
                case MOVED_REQ_COUNT_CONNECTED:
                    if (recv(client->socket, client->read_response_buf,
                                sizeof(client->read_response_buf), 0) == -1) {
                        retry_count++;
                        continue;
                    }
                    return client->read_response_buf[0];
                    break;
                case MOVED_REQ_READ:
                case MOVED_REQ_SERIAL:
                    if (recv(client->socket, client->read_response_buf,
                                sizeof(client->read_response_buf), 0) == -1) {
                        retry_count++;
                        continue;
                    }
                    return 1;
                    break;
                case MOVED_REQ_WRITE:
                    return 1;
                    break;
                default:
                    psmove_WARNING("Unknown request ID: %d\n", req);
                    return 0;
                    break;
            }
        }
    }

    switch (req) {
        case MOVED_REQ_COUNT_CONNECTED:
            psmove_WARNING("Could not get device count from %s (errno=%d)\n",
                    client->hostname, errno);
            break;
        case MOVED_REQ_READ:
            psmove_WARNING("Could not read data from %s (errno=%d)\n",
                    client->hostname, errno);
            break;
        default:
            psmove_WARNING("Request ID %d to %s failed (errno=%d)\n",
                    req, client->hostname, errno);
            break;
    }

    return 0;
}

void
moved_client_destroy(moved_client *client)
{
    close(client->socket);
    free(client);
}

