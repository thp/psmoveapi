/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * Copyright (c) 2026 Aaron Angert
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

#include <windows.h>
#include <cfgmgr32.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "hidapi.h"
#include "moved_monitor.h"
#include "../psmove_private.h"


namespace {

// Periodic rescans provide a fallback when a device notification is missed
// or the notification API is unavailable.
const ULONGLONG RESCAN_INTERVAL_MS = 500;

struct MoveDevice {
    std::string path;
    std::wstring serial;
    unsigned short product_id;
    enum MonitorEventDeviceType device_type;
};

using MoveDevices = std::map<std::string, MoveDevice>;
using CMRegisterNotification = CONFIGRET (WINAPI *)(
        PCM_NOTIFY_FILTER, PVOID, PCM_NOTIFY_CALLBACK, PHCMNOTIFICATION);
using CMUnregisterNotification = CONFIGRET (WINAPI *)(HCMNOTIFICATION);

std::string
lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::wstring
lowercase(std::wstring value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return value;
}

bool
replace_once(std::string &value, const char *from, const char *to)
{
    const auto position = value.find(from);
    if (position == std::string::npos) {
        return false;
    }
    value.replace(position, strlen(from), to);
    return true;
}

bool
contains_device_identifier(const std::wstring &path, const wchar_t *format,
        unsigned short identifier)
{
    // Windows embeds VID/PID values in symbolic paths using fragments such as
    // "vid_054c" or "vid&0002054c". Format the shared numeric identifier into
    // the requested path fragment before searching the normalized path.
    wchar_t value[16] = {};
    const auto length = std::swprintf(
            value,
            sizeof(value) / sizeof(value[0]),
            format,
            static_cast<unsigned int>(identifier));
    return length > 0 && path.find(value) != std::wstring::npos;
}

bool
is_move_interface_path(const wchar_t *path)
{
    if (path == nullptr) {
        return false;
    }

    // Windows reports device interfaces as symbolic HID paths. Normalize the
    // casing before matching the identifiers embedded in the path.
    const auto normalized = lowercase(std::wstring(path));
    // Accept both Windows VID/PID path formats and both Move generations:
    // ZCM1 (03d5) and ZCM2 (0c5e).
    const bool sony =
            contains_device_identifier(normalized, L"vid_%04x", PSMOVE_VID) ||
            contains_device_identifier(normalized, L"vid&0002%04x", PSMOVE_VID);
    const bool move =
            contains_device_identifier(normalized, L"pid_%04x", PSMOVE_PID) ||
            contains_device_identifier(normalized, L"pid&%04x", PSMOVE_PID) ||
            contains_device_identifier(normalized, L"pid_%04x", PSMOVE_PS4_PID) ||
            contains_device_identifier(normalized, L"pid&%04x", PSMOVE_PS4_PID);
    return sony && move;
}

MoveDevices
enumerate_move_devices()
{
    struct Candidate {
        Candidate(const char *candidate_path, const wchar_t *candidate_serial,
                unsigned short candidate_product_id)
            : path(candidate_path)
            , serial(candidate_serial == nullptr ? L"" : candidate_serial)
            , product_id(candidate_product_id)
        {
        }

        std::string path;
        std::wstring serial;
        unsigned short product_id;
    };

    std::set<std::string> all_paths;
    std::vector<Candidate> candidates;
    const unsigned short product_ids[] = {PSMOVE_PID, PSMOVE_PS4_PID};

    for (const auto product_id : product_ids) {
        auto devices = hid_enumerate(PSMOVE_VID, product_id);
        for (auto device = devices; device != nullptr; device = device->next) {
            if (device->path == nullptr) {
                continue;
            }

            const auto normalized_path = lowercase(std::string(device->path));
            all_paths.insert(normalized_path);
            // A Move exposes multiple HID collections. Track collection 1
            // and verify that its companion collection is also present below.
            if (normalized_path.find("&col01#") == std::string::npos) {
                continue;
            }

            candidates.emplace_back(
                device->path,
                device->serial_number,
                product_id);
        }
        hid_free_enumeration(devices);
    }

    MoveDevices result;
    for (const auto &candidate : candidates) {
        const auto normalized_path = lowercase(candidate.path);
        auto address_path = normalized_path;
        if (!replace_once(address_path, "&col01#", "&col02#") ||
                !replace_once(address_path, "&0000#", "&0001#") ||
                all_paths.count(address_path) == 0) {
            // psmove_connect_internal() needs both collections on Windows.
            continue;
        }

        result.emplace(normalized_path, MoveDevice{
            candidate.path,
            candidate.serial,
            candidate.product_id,
            // Windows exposes the controller serial over Bluetooth, while
            // the USB interface has no usable serial.
            candidate.serial.size() > 1
                    ? EVENT_DEVICE_TYPE_BLUETOOTH
                    : EVENT_DEVICE_TYPE_USB,
        });
    }
    return result;
}

} // namespace


struct _moved_monitor {
    moved_event_callback event_callback;
    void *event_callback_user_data;
    std::atomic<bool> rescan_requested;
    ULONGLONG next_rescan;
    MoveDevices known_devices;

    HMODULE cfgmgr32;
    HCMNOTIFICATION notification;
    CMUnregisterNotification unregister_notification;
};


static DWORD CALLBACK
on_device_notification(HCMNOTIFICATION, PVOID context,
        CM_NOTIFY_ACTION action, PCM_NOTIFY_EVENT_DATA event_data, DWORD)
{
    auto monitor = static_cast<moved_monitor *>(context);
    if (monitor == nullptr || event_data == nullptr ||
            event_data->FilterType != CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE) {
        return ERROR_SUCCESS;
    }

    if ((action == CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL ||
            action == CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL) &&
            is_move_interface_path(event_data->u.DeviceInterface.SymbolicLink)) {
        monitor->rescan_requested.store(true);
    }
    return ERROR_SUCCESS;
}


moved_monitor *
moved_monitor_new(moved_event_callback callback, void *user_data)
{
    auto monitor = new moved_monitor();
    monitor->event_callback = callback;
    monitor->event_callback_user_data = user_data;
    monitor->rescan_requested.store(true);
    monitor->next_rescan = GetTickCount64();
    // Load notifications dynamically so monitoring still works through the
    // periodic rescan when the API is unavailable.
    monitor->cfgmgr32 = LoadLibraryW(L"CfgMgr32.dll");
    monitor->notification = nullptr;
    monitor->unregister_notification = nullptr;

    if (monitor->cfgmgr32 == nullptr) {
        PSMOVE_WARNING("Could not load CfgMgr32.dll; using periodic HID rescans");
        return monitor;
    }

    auto register_notification = reinterpret_cast<CMRegisterNotification>(
            GetProcAddress(monitor->cfgmgr32, "CM_Register_Notification"));
    monitor->unregister_notification = reinterpret_cast<CMUnregisterNotification>(
            GetProcAddress(monitor->cfgmgr32, "CM_Unregister_Notification"));

    if (register_notification == nullptr ||
            monitor->unregister_notification == nullptr) {
        PSMOVE_WARNING(
                "CfgMgr32 device notification functions are unavailable; "
                "using periodic HID rescans");
        return monitor;
    }

    CM_NOTIFY_FILTER filter = {};
    filter.cbSize = sizeof(filter);
    filter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
    filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    const auto result = register_notification(
            &filter, monitor, on_device_notification, &monitor->notification);
    if (result != CR_SUCCESS) {
        monitor->notification = nullptr;
        PSMOVE_WARNING("Could not register for Windows device notifications (0x%lx)",
                static_cast<unsigned long>(result));
    }

    return monitor;
}


int
moved_monitor_get_fd(moved_monitor *)
{
    // Windows notifications arrive through a callback instead of a pollable fd.
    return -1;
}


void
moved_monitor_poll(moved_monitor *monitor)
{
    psmove_return_if_fail(monitor != nullptr);

    const auto now = GetTickCount64();
    if (now >= monitor->next_rescan) {
        monitor->rescan_requested.store(true);
        monitor->next_rescan = now + RESCAN_INTERVAL_MS;
    }
    if (!monitor->rescan_requested.exchange(false)) {
        return;
    }

    auto current_devices = enumerate_move_devices();

    for (const auto &known : monitor->known_devices) {
        if (current_devices.count(known.first) == 0) {
            monitor->event_callback(
                    EVENT_DEVICE_REMOVED,
                    known.second.device_type,
                    known.second.path.c_str(),
                    nullptr,
                    0,
                    monitor->event_callback_user_data);
        }
    }

    for (const auto &current : current_devices) {
        if (monitor->known_devices.count(current.first) == 0) {
            monitor->event_callback(
                    EVENT_DEVICE_ADDED,
                    current.second.device_type,
                    current.second.path.c_str(),
                    current.second.serial.empty() ? nullptr : current.second.serial.c_str(),
                    current.second.product_id,
                    monitor->event_callback_user_data);
        }
    }

    monitor->known_devices = std::move(current_devices);
}


void
moved_monitor_free(moved_monitor *monitor)
{
    psmove_return_if_fail(monitor != nullptr);

    // Notification setup can fail while periodic rescanning remains active.
    if (monitor->notification != nullptr &&
            monitor->unregister_notification != nullptr) {
        monitor->unregister_notification(monitor->notification);
    }
    if (monitor->cfgmgr32 != nullptr) {
        FreeLibrary(monitor->cfgmgr32);
    }
    delete monitor;
}
