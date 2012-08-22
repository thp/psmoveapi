
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
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

#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#if 0
#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"
#endif

#include "psmove.h"
#include "psmove_tracker.h"

#ifdef _WIN32
#  include "conio.h"
#endif

#include <TuioServer.h>
#include <TuioCursor.h>
#include <TuioTime.h>


int quit = 0;

const char *
default_hostname = "localhost";

const int
default_port = 3333;

void
usage(const char *progname, const char *error_message=NULL)
{
    fprintf(stderr, "\nUsage: %s [host] [port]\n\n", progname);
    fprintf(stderr, "  host ... Target host name (default: %s)\n",
            default_hostname);
    fprintf(stderr, "  port ... Target port (default: %d)\n",
            default_port);
    fprintf(stderr, "\n");

    if (error_message) {
        fprintf(stderr, "- ERROR: %s\n\n", error_message);
        exit(EXIT_FAILURE);
    } else {
        exit(EXIT_SUCCESS);
    }
}


void
parse_args(int argc, const char **argv, const char **hostname, int *port)
{
    if (argc == 3) {
        // Hostname and port supplied
        *hostname = argv[1];
        *port = atoi(argv[2]);

        if (port == 0) {
            usage(argv[0], "Invalid port");
        }
    } else if (argc == 2) {
        if (strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "--help") == 0 ||
                strcmp(argv[1], "/?") == 0) {
            usage(argv[0]);
        }

        // Only hostname specified - Use default port
        *hostname = argv[1];
    } else if (argc == 1) {
        // Use default hostname + port (no arguments)
    } else {
        usage(argv[0], "Too many arguments");
    }
}


int
main(int argc, const char **argv) {
    int count = psmove_count_connected();

    if (count == 0) {
        usage(argv[0], "No controllers connected");
    }

    const char *hostname = default_hostname;
    int port = default_port;

    parse_args(argc, argv, &hostname, &port);

    printf("\n");
    printf("  TUIO Configuration\n");
    printf("  ==================\n");
    printf("  Hostname: %s\n", hostname);
    printf("  Port:     %d\n", port);
    printf("\n");

    TUIO::TuioServer *tuio_server = new TUIO::TuioServer(hostname, port);
    TUIO::TuioCursor *cursors[count];
    PSMove* moves[count];

    TUIO::TuioTime::initSession();

    PSMoveTracker* tracker = psmove_tracker_new();

    unsigned char r, g, b;
    int width, height;

    for (int i=0; i<count; i++) {
        moves[i] = psmove_connect_by_id(i);
        assert(moves[i] != NULL);

        cursors[i] = NULL;

        printf("Calibrating controller %d: ", i+1);
        fflush(stdout);

        while (!quit && psmove_tracker_enable(tracker, moves[i]) !=
                Tracker_CALIBRATED) {
            printf(".");
            fflush(stdout);
        }

        printf("OK\n");
    }

    psmove_tracker_get_size(tracker, &width, &height);

    while (!quit) {
#ifdef _WIN32
        /* Allow program abortion using the Enter/Return key */
        if (kbhit()) {
            int ch = getch();
            if (ch == 10 || ch == 13) {
                quit = 1;
                break;
            }
        }
#endif

        tuio_server->initFrame(TUIO::TuioTime::getSessionTime());

        psmove_tracker_update_image(tracker);
        psmove_tracker_update(tracker, NULL);

#if 0
        void *frame = psmove_tracker_get_image(tracker);
        if (frame) {
            cvShowImage("live camera feed", frame);
        }
#endif

        for (int i=0; i<count; i++) {
            psmove_tracker_get_color(tracker, moves[i], &r, &g, &b);
            psmove_set_rumble(moves[i], 0);
            psmove_set_leds(moves[i], r, g, b);
            psmove_update_leds(moves[i]);

            // Read latest button states from the controller
            while (psmove_poll(moves[i]) && !quit);
            bool pressed = (psmove_get_buttons(moves[i]) & Btn_T);

            if (psmove_tracker_get_status(tracker, moves[i]) ==
                    Tracker_CALIBRATED_AND_FOUND) {
                float x, y, r;
                psmove_tracker_get_position(tracker, moves[i],
                        &x, &y, &r);

                /* Radius (r) is not used here for now ... */

                /* camera + screen have the same direction -> mirror x */
                x = 1. - (x / (float)width);
                y = y / (float)height;

                if (pressed) {
                    if (cursors[i]) {
                        tuio_server->updateTuioCursor(cursors[i], x, y);
                    } else {
                        cursors[i] = tuio_server->addTuioCursor(x, y);
                    }
                } else {
                    if (cursors[i]) {
                        tuio_server->removeTuioCursor(cursors[i]);
                        cursors[i] = NULL;
                    }
                }
            } else if (pressed) {
                /**
                 * The user presses the button, but the controller is not
                 * tracked by the camera - vibrate the controller slightly.
                 **/
                psmove_set_rumble(moves[i], 100);
                psmove_update_leds(moves[i]);
            }
        }

        tuio_server->commitFrame();
    }

    for (int i=0; i<count; i++) {
        if (moves[i]) {
            psmove_disconnect(moves[i]);
        }
        if (cursors[i]) {
            tuio_server->removeTuioCursor(cursors[i]);
        }
    }

    delete tuio_server;
    psmove_tracker_free(tracker);

    return 0;
}

