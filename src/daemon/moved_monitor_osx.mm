
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012, 2017 Thomas Perl <m@thp.io>
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <queue>

#include "moved_monitor.h"

#include "../src/psmove_private.h"

#import <IOKit/hid/IOHIDManager.h>
#import <CoreFoundation/CoreFoundation.h>

// Convenience functions copied from hidapi
#include "moved_monitor_osx_hidapi.mm"

struct ASyncDeviceEvent {
    enum MonitorEvent event;
    enum MonitorEventDeviceType device_type;
    char path[256];
    wchar_t serial_number[256];
};

struct _moved_monitor {
    _moved_monitor(moved_event_callback callback, void *user_data)
        : event_callback(callback)
        , event_callback_user_data(user_data)
    {
        mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
        if (!mgr) {
            psmove_CRITICAL("Could not create HID manager");
        }

        IOHIDManagerSetDeviceMatching(mgr, NULL);
        IOHIDManagerRegisterDeviceMatchingCallback(mgr, _moved_monitor::on_device_matching, this);
        IOHIDManagerRegisterDeviceRemovalCallback(mgr, _moved_monitor::on_device_removal, this);
        IOHIDManagerOpen(mgr, kIOHIDOptionsTypeNone);
        IOHIDManagerScheduleWithRunLoop(mgr, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    }

    ~_moved_monitor()
    {
        IOHIDManagerClose(mgr, kIOHIDOptionsTypeNone);
    }

    void pump_loop()
    {
        SInt32 res;
        do {
            res = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.001, FALSE);
        } while(res != kCFRunLoopRunFinished && res != kCFRunLoopRunTimedOut);


        while (!events.empty()) {
            auto event = events.front();
            event_callback(event.event, event.device_type,
                           event.path, event.serial_number,
                           event_callback_user_data);
            events.pop();
        }
    }

private:
    void
    make_event(enum MonitorEvent event, IOHIDDeviceRef device)
    {
        if (get_vendor_id(device) == PSMOVE_VID && (get_product_id(device) == PSMOVE_PID || get_product_id(device) == PSMOVE_PS4_PID)) {
            ASyncDeviceEvent ade;
            ade.event = event;
            ade.device_type = EVENT_DEVICE_TYPE_UNKNOWN;

            // Example paths:
            // e.g. "USB_054c_03d5_14100000"
            // or: "Bluetooth_054c_03d5_7794a680"
            make_path(device, ade.path, sizeof(ade.path));

            if (memcmp(ade.path, "USB_", 4) == 0) {
                ade.device_type = EVENT_DEVICE_TYPE_USB;
            } else if (memcmp(ade.path, "Bluetooth_", 10) == 0) {
                ade.device_type = EVENT_DEVICE_TYPE_BLUETOOTH;
            }

            get_serial_number(device, ade.serial_number, sizeof(ade.serial_number));

            events.push(ade);
        }
    }

    static void on_device_matching(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
    {
        _moved_monitor *monitor = (_moved_monitor *)(context);
        monitor->make_event(EVENT_DEVICE_ADDED, device);
    }

    static void on_device_removal(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
    {
        _moved_monitor *monitor = (_moved_monitor *)(context);
        monitor->make_event(EVENT_DEVICE_REMOVED, device);
    }

    moved_event_callback event_callback;
    void *event_callback_user_data;
    IOHIDManagerRef mgr;
    std::queue<ASyncDeviceEvent> events;
};


moved_monitor *
moved_monitor_new(moved_event_callback callback, void *user_data)
{
    return new _moved_monitor(callback, user_data);
}

int
moved_monitor_get_fd(moved_monitor *monitor)
{
    psmove_return_val_if_fail(monitor != NULL, -1);

    return -1;
}

void
moved_monitor_poll(moved_monitor *monitor)
{
    psmove_return_if_fail(monitor != NULL);

    monitor->pump_loop();
}

void
moved_monitor_free(moved_monitor *monitor)
{
    delete monitor;
}
