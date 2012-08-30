
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
default_hostname = "127.0.0.1";

const int
default_port = 3333;

const char *
default_tcp_target = "127.0.0.1";

const int
default_tcp_port = 9999;


class TcpEventSender
{
    /**
     *
     * TCP Event Sender Protocol (line-based):
     *
     *     [event] [controller-id] [args]
     *
     * Known events:
     *
     *     trigger - Trigger value updated
     *     press - Button was pressed
     *     release - Button was released
     *     state - Tracking state update
     *     pos - Position update (only sent when TUIO is disabled)
     *
     * trigger events:
     *
     *     trigger [controller-id] [trigger-value]
     *
     *     trigger-value ... 0 (not pressed) - 255 (fully pressed)
     *
     *     Example: trigger 0 255
     *              ^ Trigger on Controller #0 fully pressed
     *
     * press/release events:
     *
     *     press [controller-id] [button-name]
     *     release [controller-id] [button-name]
     *
     *     button-name ... select, start, move, ps, square, triangle,
     *                     cross, circle
     *
     *     Example: press 0 square
     *              ^ Square button on Controller #0 was pressed
     *
     * state events:
     *
     *     state [controller-id] [lost-or-found]
     *
     *     lost-or-found ... "lost" (tracking lost) or "found" (tracking now)
     *
     *     Example: state 0 lost
     *              ^ Tracking of Controller #0 was lost
     *
     * pos events:
     *
     *     pos [controller-id] [xpos-as-float] [ypos-as-float]
     *
     *     xpos-as-float ... X position as float (0.0 - 1.0)
     *     ypos-as-float ... Y position as float (0.0 - 1.0)
     *
     *     Example: pos 0 0.52130 0.56098
     *              ^ Controller #0 is at x=0.5213 and y=0.56098
     *
     *
     * For debugging purposes, you can start a simple server using netcat:
     *
     *     nc -kl 9999
     *
     * Example session transcript:
     *
     *     state 0 found
     *     pos 0 0.51658 0.55999
     *     pos 0 0.51607 0.55951
     *     trigger 0 255
     *     press 0 cross
     *     press 0 square
     *     press 0 move
     *     release 0 cross
     *     release 0 square
     *     release 0 move
     *     trigger 0 0
     *     state 0 lost
     *     state 0 found
     *     pos 0 0.51316 0.55883
     *     pos 0 0.51415 0.55423
     *     pos 0 0.51229 0.55767
     *     state 0 lost
     *     state 0 found
     *     pos 0 0.50931 0.55829
     *     pos 0 0.51026 0.55782
     *     pos 0 0.51415 0.55423
     *     trigger 0 26
     *     press 0 move
     *     trigger 0 255
     *     press 0 cross
     *     press 0 square
     *     release 0 cross
     *     release 0 square
     *     trigger 0 211
     *     release 0 move
     *     trigger 0 13
     *     trigger 0 0
     *
     **/

    public:
        TcpEventSender(const char *target, int port);
        ~TcpEventSender();

        void updateController(int id, int buttons, int trigger);
        void updatePosition(int id, float x, float y);
        void updateFound(int id, bool found);

        inline void flush() { fflush(m_socket); }

    private:
        FILE *m_socket;

        struct {
            int buttons;
            int trigger;
            float x;
            float y;
            bool found;
        } m_state[PSMOVE_TRACKER_MAX_CONTROLLERS];

        const char *buttonName(int button);
};

TcpEventSender::TcpEventSender(const char *target, int port)
{
    memset(m_state, 0, sizeof(m_state));

    int sock;
    struct sockaddr_in addr;

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(sock != -1);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(target);
    assert(addr.sin_addr.s_addr != INADDR_NONE);

    assert(connect(sock, (const struct sockaddr*)&addr, sizeof(addr)) == 0);

    m_socket = fdopen(sock, "w");
}

TcpEventSender::~TcpEventSender()
{
    fclose(m_socket);
}

void
TcpEventSender::updateController(int id, int buttons, int trigger)
{
    // We get the trigger as value, don't expose as button
    buttons &= ~Btn_T;

    if (m_state[id].buttons != buttons) {
        int pressed = buttons & ~(m_state[id].buttons);
        int released = ~buttons & (m_state[id].buttons);
        int value = 1;
        while (pressed || released) {
            if (pressed & value) {
                fprintf(m_socket, "press %d %s\n", id, buttonName(value));
                pressed &= ~value;
            }
            if (released & value) {
                fprintf(m_socket, "release %d %s\n", id, buttonName(value));
                released &= ~value;
            }
            value <<= 1;
        }

        m_state[id].buttons = buttons;
    }

    if (m_state[id].trigger != trigger) {
        fprintf(m_socket, "trigger %d %d\n", id, trigger);
        m_state[id].trigger = trigger;
    }
}


void
TcpEventSender::updatePosition(int id, float x, float y)
{
    if ((m_state[id].x != x) || (m_state[id].y != y)) {
        fprintf(m_socket, "pos %d %.05f %.05f\n", id, x, y);

        m_state[id].x = x;
        m_state[id].y = y;
    }
}

void
TcpEventSender::updateFound(int id, bool found)
{
    if (m_state[id].found != found) {
        fprintf(m_socket, "state %d %s\n", id, found?"found":"lost");
        m_state[id].found = found;
    }
}

const char *
TcpEventSender::buttonName(int button)
{
    switch (button) {
        case Btn_MOVE: return "move";
        case Btn_PS: return "ps";

        case Btn_SELECT: return "select";
        case Btn_START: return "start";

        case Btn_SQUARE: return "square";
        case Btn_TRIANGLE: return "triangle";
        case Btn_CROSS: return "cross";
        case Btn_CIRCLE: return "circle";

        default: return "-";
    }
}


void
usage(const char *progname, const char *error_message=NULL)
{
    fprintf(stderr, "\nUsage: %s [host] [port] [tcp-host] [tcp-port]\n\n",
            progname);
    fprintf(stderr, "  host ....... TUIO host name (default: %s)\n",
            default_hostname);
    fprintf(stderr, "  port ....... TUIO port (default: %d)\n",
            default_port);
    fprintf(stderr, "  tcp-host ... TCP host name (default: %s)\n",
            default_tcp_target);
    fprintf(stderr, "  tcp-port ... TCP port (default: %d)\n",
            default_tcp_port);
    fprintf(stderr, "\n");

    if (error_message) {
        fprintf(stderr, "- ERROR: %s\n\n", error_message);
        exit(EXIT_FAILURE);
    } else {
        exit(EXIT_SUCCESS);
    }
}


void
parse_args(int argc, const char **argv,
        const char **hostname, int *port,
        const char **tcp_target, int *tcp_port)
{
    if (argc == 5) {
        // Hostname and port for TCP
        *tcp_target = argv[3];
        *tcp_port = atoi(argv[4]);

        argc = 3;
    } else if (argc == 4) {
        // Only TCP hostname specified - Use default port
        *tcp_target = argv[3];

        argc = 3;
    }

    if (argc == 3) {
        // Hostname and port for TUIO
        *hostname = argv[1];
        *port = atoi(argv[2]);
    } else if (argc == 2) {
        if (strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "--help") == 0 ||
                strcmp(argv[1], "/?") == 0) {
            usage(argv[0]);
        }

        // Only TUIO hostname specified - Use default port
        *hostname = argv[1];
    } else if (argc == 1) {
        // Use default hostname + port (no arguments)
    } else {
        usage(argv[0], "Wrong number of arguments");
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

    const char *tcp_target = default_tcp_target;
    int tcp_port = default_tcp_port;

    parse_args(argc, argv, &hostname, &port, &tcp_target, &tcp_port);

    // We want to enable at least one of TCP or TUIO
    assert(port || tcp_port);

    printf("\n");
    printf("  TUIO Configuration\n");
    printf("  ==================\n");
    if (port) {
        printf("  Hostname: %s\n", hostname);
        printf("  Port:     %d\n", port);
    } else {
        printf("  Disabled.\n");
    }
    printf("\n");
    printf("  TCP Configuration\n");
    printf("  =================\n");
    if (tcp_port) {
        printf("  TCP host: %s\n", tcp_target);
        printf("  TCP port: %d\n", tcp_port);
    } else {
        printf("  Disabled.\n");
    }
    printf("\n");

    TcpEventSender *event_sender = NULL;

    if (tcp_port) {
        event_sender = new TcpEventSender(tcp_target, tcp_port);
    }

    TUIO::TuioServer *tuio_server = NULL;

    if (port) {
        tuio_server = new TUIO::TuioServer(hostname, port);
    }
    TUIO::TuioCursor *cursors[count];
    PSMove* moves[count];

    if (tuio_server) {
        tuio_server->enableFullUpdate();
        //tuio_server->setVerbose(true);
    }

    TUIO::TuioTime::initSession();
    long timestamp = 0;

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

        if (tuio_server) {
            tuio_server->initFrame(timestamp++);
        }

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

            if (event_sender) {
                event_sender->updateController(i,
                        psmove_get_buttons(moves[i]),
                        psmove_get_trigger(moves[i]));
            }

            // Read latest button states from the controller
            while (psmove_poll(moves[i]));

            bool pressed = (psmove_get_buttons(moves[i]) & Btn_T);

            enum PSMoveTracker_Status status =
                psmove_tracker_get_status(tracker, moves[i]);

            if (event_sender) {
                event_sender->updateFound(i,
                        status == Tracker_TRACKING);
            }

            if (status == Tracker_TRACKING) {
                float x, y, r;
                psmove_tracker_get_position(tracker, moves[i],
                        &x, &y, &r);

                /* Radius (r) is not used here for now ... */

                /* camera + screen have the same direction -> mirror x */
                x = x / (float)width;
                y = y / (float)height;

                if (event_sender && !tuio_server) {
                    // Only send position updates via TCP if TUIO is disabled
                    event_sender->updatePosition(i, x, y);
                }

                if (tuio_server) {
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

        if (tuio_server) {
            tuio_server->commitFrame();
        }
        if (event_sender) {
            event_sender->flush();
        }
    }

    for (int i=0; i<count; i++) {
        if (moves[i]) {
            psmove_disconnect(moves[i]);
        }
        if (cursors[i]) {
            tuio_server->removeTuioCursor(cursors[i]);
        }
    }

    if (event_sender) {
        delete event_sender;
    }
    if (tuio_server) {
        delete tuio_server;
    }
    psmove_tracker_free(tracker);

    return 0;
}

