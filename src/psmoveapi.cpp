
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


#include "psmoveapi.h"

#include <vector>
#include <map>
#include <string>

#include <stdlib.h>
#include <string.h>

namespace {

struct ControllerGlue {
    ControllerGlue(int index, const std::string &serial, PSMove *handle_a, PSMove *handle_b);
    ~ControllerGlue();

    ControllerGlue(const ControllerGlue &other) = delete;
    Controller &operator=(const ControllerGlue &other) = delete;

    PSMove *read_move() { return move_bluetooth; }
    PSMove *write_move() { return move_usb ?: move_bluetooth; }

    PSMove *move_bluetooth;
    PSMove *move_usb;
    std::string serial;
    struct Controller controller;
    bool connected;
    bool sent_connect;
    bool sent_disconnect;
};

struct PSMoveAPI {
    PSMoveAPI(EventReceiver *receiver, void *user_data);
    ~PSMoveAPI();

    void update();

    EventReceiver *receiver;
    void *user_data;
    std::vector<ControllerGlue *> controllers;
};

PSMoveAPI *
g_psmove_api = nullptr;

}; // end anonymous namespace

ControllerGlue::ControllerGlue(int index, const std::string &serial, PSMove *handle_a, PSMove *handle_b)
    : move_bluetooth(nullptr)
    , move_usb(nullptr)
    , serial(serial)
    , controller()
    , connected(true)
    , sent_connect(false)
    , sent_disconnect(false)
{
    memset(&controller, 0, sizeof(controller));
    controller.index = index;
    controller.serial = this->serial.c_str();

    if (psmove_connection_type(handle_a) == Conn_USB) {
        move_bluetooth = handle_b;
        move_usb = handle_a;
    } else {
        move_bluetooth = handle_a;
        move_usb = handle_b;
    }

    controller.usb = (move_usb != nullptr);
    controller.bluetooth = (move_bluetooth != nullptr);

    if (controller.usb) {
        controller.battery = Batt_CHARGING;
    }
}

ControllerGlue::~ControllerGlue()
{
    if (move_bluetooth != nullptr) {
        psmove_disconnect(move_bluetooth);
    }
    if (move_usb != nullptr) {
        psmove_disconnect(move_usb);
    }
}

PSMoveAPI::PSMoveAPI(EventReceiver *receiver, void *user_data)
    : receiver(receiver)
    , user_data(user_data)
    , controllers()
{
    std::map<std::string, std::vector<PSMove *>> moves;

    int n = psmove_count_connected();
    for (int i=0; i<n; i++) {
        PSMove *move = psmove_connect_by_id(i);

        char *tmp = psmove_get_serial(move);
        std::string serial(tmp);
        free(tmp);

        moves[serial].emplace_back(move);
    }

    int i = 0;
    for (auto &kv: moves) {
        if (kv.second.size() == 2) {
            // Have two handles for this controller (USB + Bluetooth)
            controllers.emplace_back(new ControllerGlue(i++, kv.first, kv.second[0], kv.second[1]));
        } else if (kv.second.size() == 1) {
            // Have only one handle for this controller
            controllers.emplace_back(new ControllerGlue(i++, kv.first, kv.second[0], nullptr));
        } else {
            // FATAL
        }
    }
}

PSMoveAPI::~PSMoveAPI()
{
    for (auto &c: controllers) {
        if (!c->sent_disconnect) {
            if (receiver->disconnect != nullptr) {
                // Send disconnect event
                receiver->disconnect(&c->controller, user_data);
            }

            c->sent_disconnect = true;
        }

        delete c;
    }
}

void
PSMoveAPI::update()
{
    for (auto &c: controllers) {
        if (c->connected && !c->sent_connect) {
            if (receiver->connect != nullptr) {
                // Send initial connect event
                receiver->connect(&c->controller, user_data);
            }
            c->sent_connect = true;
        }

        if (!c->connected && !c->sent_disconnect) {
            if (receiver->disconnect != nullptr) {
                // Send disconnect event
                receiver->disconnect(&c->controller, user_data);
            }
            c->sent_disconnect = true;
        }

        if (!c->connected) {
            // Done with this controller (TODO: Can we check reconnect?)
            continue;
        }

        auto read_move = c->read_move();
        if (read_move == nullptr) {
            // We don't have a handle that supports reading (USB-only connection);
            // we call update exactly once (no new data will be reported), so that
            // the update method can change the LED and rumble values
            if (receiver->update != nullptr) {
                receiver->update(&c->controller, user_data);
            }
        } else {
            while (psmove_poll(read_move)) {
                if (receiver->update != nullptr) {
                    c->controller.buttons = psmove_get_buttons(read_move);
                    c->controller.trigger = float(psmove_get_trigger(read_move)) / 255.f;

                    psmove_get_accelerometer_frame(read_move, Frame_SecondHalf,
                            &c->controller.accelerometer.x,
                            &c->controller.accelerometer.y,
                            &c->controller.accelerometer.z);
                    psmove_get_gyroscope_frame(read_move, Frame_SecondHalf,
                            &c->controller.gyroscope.x,
                            &c->controller.gyroscope.y,
                            &c->controller.gyroscope.z);
                    psmove_get_magnetometer_vector(read_move,
                            &c->controller.magnetometer.x,
                            &c->controller.magnetometer.y,
                            &c->controller.magnetometer.z);
                    c->controller.battery = psmove_get_battery(read_move);

                    receiver->update(&c->controller, user_data);
                }
            }
        }

        auto write_move = c->write_move();
        if (write_move != nullptr) {
            psmove_set_leds(write_move,
                    uint32_t(255 * c->controller.color.r),
                    uint32_t(255 * c->controller.color.g),
                    uint32_t(255 * c->controller.color.b));
            psmove_set_rumble(write_move, uint32_t(255 * c->controller.rumble));
            if (psmove_update_leds(write_move) == Update_Failed) {
                if (read_move != nullptr && write_move != read_move) {
                    // Disconnected from USB, but we read from Bluetooth, so we
                    // can just give up the USB connection and use Bluetooth
                    psmove_disconnect(c->move_usb), c->move_usb = nullptr;
                    // Now that USB is gone, clear the controller's USB flag
                    c->controller.usb = 0;
                } else {
                    c->connected = false;
                }
            }
        }
    }
}

void
psmoveapi_init(EventReceiver *receiver, void *user_data)
{
    if (g_psmove_api == nullptr) {
        g_psmove_api = new PSMoveAPI(receiver, user_data);
    }
}

void
psmoveapi_update()
{
    if (g_psmove_api != nullptr) {
        g_psmove_api->update();
    }
}

void
psmoveapi_quit()
{
    delete g_psmove_api;
    g_psmove_api = nullptr;
}
