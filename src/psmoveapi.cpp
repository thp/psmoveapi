
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

#include "psmove_port.h"
#include "psmove_private.h"
#include "daemon/moved_monitor.h"

#include <vector>
#include <map>
#include <string>

#include <stdlib.h>
#include <string.h>

namespace {

struct ControllerGlue {
    ControllerGlue(int index, const std::string &serial);
    ~ControllerGlue();

    void add_handle(PSMove *handle);
    void update_connection_flags();

    ControllerGlue(const ControllerGlue &other) = delete;
    Controller &operator=(const ControllerGlue &other) = delete;

    PSMove *read_move() { return move_bluetooth; }
    PSMove *write_move() { return move_usb ? move_usb : move_bluetooth; }

    PSMove *move_bluetooth;
    PSMove *move_usb;
    std::string serial;
    struct Controller controller;
    bool connected;
    bool api_connected;
};

struct PSMoveAPI {
    PSMoveAPI(EventReceiver *receiver, void *user_data);
    ~PSMoveAPI();

    void update();

    static void on_monitor_event(enum MonitorEvent event, enum MonitorEventDeviceType device_type, const char *path, const wchar_t *serial, void *user_data);

    EventReceiver *receiver;
    void *user_data;
    std::vector<ControllerGlue *> controllers;
    moved_monitor *monitor;
};

PSMoveAPI *
g_psmove_api = nullptr;

}; // end anonymous namespace

ControllerGlue::ControllerGlue(int index, const std::string &serial)
    : move_bluetooth(nullptr)
    , move_usb(nullptr)
    , serial(serial)
    , controller()
    , connected(false)
    , api_connected(false)
{
    memset(&controller, 0, sizeof(controller));
    controller.index = index;
    controller.serial = this->serial.c_str();
}

void
ControllerGlue::add_handle(PSMove *handle)
{
    if (psmove_connection_type(handle) == Conn_USB) {
        if (move_usb != nullptr) {
            psmove_WARNING("USB handle already exists for this controller");
            psmove_disconnect(move_usb);
        }

        move_usb = handle;
    } else {
        if (move_bluetooth != nullptr) {
            psmove_WARNING("Bluetooth handle already exists for this controller -- leaking");
            psmove_disconnect(move_bluetooth);
        }

        move_bluetooth = handle;
    }
}

void
ControllerGlue::update_connection_flags()
{
    controller.usb = (move_usb != nullptr);
    controller.bluetooth = (move_bluetooth != nullptr);

    if (controller.usb && !controller.bluetooth) {
        controller.battery = Batt_CHARGING;
    }

    connected = (controller.usb || controller.bluetooth);
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
    , monitor(nullptr)
{
    std::map<std::string, std::vector<PSMove *>> moves;

    int n = psmove_count_connected();
    for (int i=0; i<n; i++) {
        PSMove *move = psmove_connect_by_id(i);
        if (!move) {
            psmove_WARNING("Failed to connect to controller #%d", i);
            continue;
        }

        char *tmp = psmove_get_serial(move);
        if (!tmp) {
            psmove_WARNING("Failed to get serial for controller #%d", i);
            continue;
        }

        std::string serial(tmp);
        free(tmp);

        moves[serial].emplace_back(move);
    }

    int i = 0;
    for (auto &kv: moves) {
        auto c = new ControllerGlue(i++, kv.first);
        for (auto &handle: kv.second) {
            c->add_handle(handle);
        }
        controllers.emplace_back(c);
    }

#ifndef _WIN32
    monitor = moved_monitor_new(PSMoveAPI::on_monitor_event, this);
#endif
}

PSMoveAPI::~PSMoveAPI()
{
#ifndef _WIN32
    moved_monitor_free(monitor);
#endif

    for (auto &c: controllers) {
        if (c->api_connected) {
            if (receiver->disconnect != nullptr) {
                // Send disconnect event
                receiver->disconnect(&c->controller, user_data);
            }

            c->api_connected = false;
        }

        delete c;
    }
}

void
PSMoveAPI::update()
{
#ifndef _WIN32
    if (moved_monitor_get_fd(monitor) == -1) {
        moved_monitor_poll(monitor);
    } else {
        struct pollfd pfd;
        pfd.fd = moved_monitor_get_fd(monitor);
        pfd.events = POLLIN;
        while (poll(&pfd, 1, 0) > 0) {
            moved_monitor_poll(monitor);
        }
    }
#endif

    for (auto &c: controllers) {
        c->update_connection_flags();

        if (c->connected && !c->api_connected) {
            if (receiver->connect != nullptr) {
                // Send initial connect event
                receiver->connect(&c->controller, user_data);
            }
            c->api_connected = c->connected;
        }

        if (!c->connected && c->api_connected) {
            if (receiver->disconnect != nullptr) {
                // Send disconnect event
                receiver->disconnect(&c->controller, user_data);
            }
            c->api_connected = c->connected;
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
                    int previous = c->controller.buttons;
                    c->controller.buttons = psmove_get_buttons(read_move);
                    c->controller.pressed = c->controller.buttons & ~previous;
                    c->controller.released = previous & ~c->controller.buttons;
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
            psmove_update_leds(write_move);
        }
    }
}

void
PSMoveAPI::on_monitor_event(enum MonitorEvent event, enum MonitorEventDeviceType device_type, const char *path, const wchar_t *serial, void *user_data)
{
    auto self = static_cast<PSMoveAPI *>(user_data);

    switch (event) {
        case EVENT_DEVICE_ADDED:
            {
                psmove_DEBUG("on_monitor_event(event=EVENT_DEVICE_ADDED, device_type=0x%08x, path=\"%s\", serial=%p)",
                       device_type, path, serial);

                for (auto &c: self->controllers) {
                    if ((c->move_bluetooth != nullptr && strcmp(_psmove_get_device_path(c->move_bluetooth), path) == 0) ||
                            (c->move_usb != nullptr && strcmp(_psmove_get_device_path(c->move_usb), path) == 0)) {
                        psmove_WARNING("This controller is already active!");
                        return;
                    }
                }

                // TODO: FIXME: This should use the device's actual USB product ID.
                // HACK: We rely on this invalid PID being translated to a
                //       valid controller model (the old ZCM1, by default).
                unsigned short pid = 0;
                PSMove *move = psmove_connect_internal(serial, path, -1, pid);
                if (move == nullptr) {
                    psmove_CRITICAL("Cannot open move for retrieving serial!");
                    return;
                }

                char *serial_number = psmove_get_serial(move);

                bool found = false;
                for (auto &c: self->controllers) {
                    if (strcmp(c->serial.c_str(), serial_number) == 0) {
                        c->add_handle(move);
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    auto c = new ControllerGlue(self->controllers.size(), std::string(serial_number));
                    c->add_handle(move);
                    self->controllers.emplace_back(c);
                }

                free(serial_number);
            }
            break;
        case EVENT_DEVICE_REMOVED:
            {
                psmove_DEBUG("on_monitor_event(event=EVENT_DEVICE_REMOVED, device_type=0x%08x, path=\"%s\", serial=%p)",
                       device_type, path, serial);

                bool found = false;
                for (auto &c: self->controllers) {
                    if (c->move_bluetooth != nullptr) {
                        const char *devpath = _psmove_get_device_path(c->move_bluetooth);
                        if (devpath != nullptr && strcmp(devpath, path) == 0) {
                            psmove_disconnect(c->move_bluetooth), c->move_bluetooth = nullptr;
                            found = true;
                            break;
                        }
                    }

                    if (c->move_usb != nullptr) {
                        const char *devpath = _psmove_get_device_path(c->move_usb);
                        if (devpath != nullptr && strcmp(devpath, path) == 0) {
                            psmove_disconnect(c->move_usb), c->move_usb = nullptr;
                            found = true;
                            break;
                        }
                    }
                }

                if (!found) {
                    psmove_CRITICAL("Did not find device for removal\n");
                }
            }
            break;
        default:
            psmove_CRITICAL("Invalid event");
            break;
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
