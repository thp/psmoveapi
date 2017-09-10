#pragma once

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



#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>

enum MonitorEvent {
    EVENT_DEVICE_ADDED,
    EVENT_DEVICE_REMOVED,
};

enum MonitorEventDeviceType {
    EVENT_DEVICE_TYPE_USB,
    EVENT_DEVICE_TYPE_BLUETOOTH,
    EVENT_DEVICE_TYPE_UNKNOWN,
};

typedef void (*moved_event_callback)(enum MonitorEvent event,
        enum MonitorEventDeviceType device_type, const char *path,
        const wchar_t *serial, void *user_data);

typedef struct _moved_monitor moved_monitor;

moved_monitor *
moved_monitor_new(moved_event_callback callback, void *user_data);

void
moved_monitor_poll(moved_monitor *monitor);

int
moved_monitor_get_fd(moved_monitor *monitor);

void
moved_monitor_free(moved_monitor *monitor);

#ifdef __cplusplus
}
#endif
