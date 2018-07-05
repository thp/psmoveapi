
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libudev.h>
#include <linux/input.h>

#include "moved_monitor.h"

#include "../src/psmove_private.h"


struct _moved_monitor {
    struct udev *udev_handle;
    struct udev_monitor *udev_monitor;
    moved_event_callback event_callback;
    void *event_callback_user_data;
};


/*
 * The caller is responsible for free()ing the (newly-allocated) character
 * strings pointed to by serial_number_utf8 and product_name_utf8 after use.
 */
int
parse_uevent_info(const char *uevent, int *bus_type,
        unsigned short *vendor_id, unsigned short *product_id,
        char **serial_number_utf8, char **product_name_utf8)
{
    char *tmp = strdup(uevent);
    char *saveptr = NULL;
    char *line;
    char *key;
    char *value;

    int found_id = 0;
    int found_serial = 0;
    int found_name = 0;

    line = strtok_r(tmp, "\n", &saveptr);
    while (line != NULL) {
        /* line: "KEY=value" */
        key = line;
        value = strchr(line, '=');
        if (!value) {
            goto next_line;
        }
        *value = '\0';
        value++;

        if (strcmp(key, "HID_ID") == 0) {
            /**
             *        type vendor   product
             * HID_ID=0003:000005AC:00008242
             **/
            int ret = sscanf(value, "%x:%hx:%hx", bus_type, vendor_id, product_id);
            if (ret == 3) {
                found_id = 1;
            }
        } else if (strcmp(key, "HID_NAME") == 0) {
            /* The caller has to free the product name */
            *product_name_utf8 = strdup(value);
            found_name = 1;
        } else if (strcmp(key, "HID_UNIQ") == 0) {
            /* The caller has to free the serial number */
            *serial_number_utf8 = strdup(value);
            found_serial = 1;
        }

next_line:
        line = strtok_r(NULL, "\n", &saveptr);
    }

    free(tmp);
    return (found_id && found_name && found_serial);
}

/**
 * Copied from hidapi
 * The caller must free the returned string with free().
 **/
static wchar_t *utf8_to_wchar_t(const char *utf8)
{
        wchar_t *ret = NULL;

        if (utf8) {
                size_t wlen = mbstowcs(NULL, utf8, 0);
                if (wlen < 0) {
                        return wcsdup(L"");
                }
                ret = calloc(wlen+1, sizeof(wchar_t));
                mbstowcs(ret, utf8, wlen+1);
                ret[wlen] = 0x0000;
        }

        return ret;
}

void
_moved_monitor_handle_device(moved_monitor *monitor, struct udev_device *dev)
{
    const char *action = udev_device_get_action(dev);
    const char *path = udev_device_get_devnode(dev);
    enum MonitorEventDeviceType device_type = EVENT_DEVICE_TYPE_UNKNOWN;

    if (strcmp(action, "add") == 0) {
        struct udev_device *hid_dev =
            udev_device_get_parent_with_subsystem_devtype(dev, "hid", NULL);
        psmove_return_if_fail(hid_dev != NULL);

        char *uevent = strdup(
                udev_device_get_sysattr_value(hid_dev, "uevent"));
        psmove_return_if_fail(uevent != NULL);

        int bus_type;
        unsigned short vendor_id, product_id;
        char *serial_number_utf8;
        char *product_name_utf8;

        if (parse_uevent_info(uevent, &bus_type, &vendor_id, &product_id,
                    &serial_number_utf8, &product_name_utf8)) {
            wchar_t *serial_number = utf8_to_wchar_t(serial_number_utf8);

            if (vendor_id == PSMOVE_VID && (product_id == PSMOVE_PID || product_id == PSMOVE_PS4_PID)) {
                if (bus_type == BUS_BLUETOOTH) {
                    device_type = EVENT_DEVICE_TYPE_BLUETOOTH;
                } else if (bus_type == BUS_USB) {
                    device_type = EVENT_DEVICE_TYPE_USB;
                }

                monitor->event_callback(EVENT_DEVICE_ADDED, device_type, path,
                        serial_number, monitor->event_callback_user_data);
            }

            free(serial_number);
            free(serial_number_utf8);
            free(product_name_utf8);
        }

        free(uevent);
    } else if (strcmp(action, "remove") == 0) {
        monitor->event_callback(EVENT_DEVICE_REMOVED, device_type, path,
                NULL, monitor->event_callback_user_data);
    }
}

moved_monitor *
moved_monitor_new(moved_event_callback callback, void *user_data)
{
    moved_monitor *monitor = calloc(1, sizeof(moved_monitor));

    monitor->udev_handle = udev_new();
    monitor->udev_monitor = udev_monitor_new_from_netlink(monitor->udev_handle,
            "udev");

    monitor->event_callback = callback;
    monitor->event_callback_user_data = user_data;

    udev_monitor_filter_add_match_subsystem_devtype(monitor->udev_monitor,
            "hidraw", NULL);
    udev_monitor_enable_receiving(monitor->udev_monitor);

    return monitor;
}

int
moved_monitor_get_fd(moved_monitor *monitor)
{
    psmove_return_val_if_fail(monitor != NULL, -1);

    return udev_monitor_get_fd(monitor->udev_monitor);
}

void
moved_monitor_poll(moved_monitor *monitor)
{
    psmove_return_if_fail(monitor != NULL);

    struct udev_device *device = udev_monitor_receive_device(
            monitor->udev_monitor);

    if (device) {
        _moved_monitor_handle_device(monitor, device);
        udev_device_unref(device);
    }
}

void
moved_monitor_free(moved_monitor *monitor)
{
    psmove_return_if_fail(monitor != NULL);

    udev_monitor_filter_remove(monitor->udev_monitor);
    udev_monitor_unref(monitor->udev_monitor);
    udev_unref(monitor->udev_handle);

    free(monitor);
}
