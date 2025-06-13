/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2016, 2023, 2025 Thomas Perl <m@thp.io>
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


#include "psmove_port.h"
#include "psmove_sockets.h"
#include "psmove_private.h"
#include "psmove_format.h"

#include <unistd.h>
#include <sys/time.h>

#include <string>
#include <iostream>
#include <cstdarg>
#include <cstdio>


#include <IOBluetooth/objc/IOBluetoothHostController.h>

#import <Foundation/NSAutoreleasePool.h>


/* Location for the plist file that we want to modify */
#define OSX_BT_CONFIG_PATH "/Library/Preferences/com.apple.Bluetooth"

/* Function declarations for IOBluetooth private API */
extern "C" {
void IOBluetoothPreferenceSetControllerPowerState(int);
int IOBluetoothPreferenceGetControllerPowerState();
};

#define OSXPAIR_DEBUG(...) PSMOVE_INFO(__VA_ARGS__)

namespace {

struct ScopedNSAutoreleasePool {
    ScopedNSAutoreleasePool()
        : pool([[NSAutoreleasePool alloc] init])
    {}

    ~ScopedNSAutoreleasePool()
    {
        [pool release];
    }

private:
    ScopedNSAutoreleasePool(const ScopedNSAutoreleasePool &) = delete;
    ScopedNSAutoreleasePool &operator=(const ScopedNSAutoreleasePool &) = delete;

    NSAutoreleasePool *pool;
};

std::vector<std::string>
subprocess(const std::string &cmdline)
{
    FILE *fp = popen(cmdline.c_str(), "r");
    if (!fp) {
        return {};
    }

    std::vector<std::string> result;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        // Remove trailing newline
        line[strlen(line)-1] = '\0';
        result.emplace_back(line);
    }

    pclose(fp);
    return result;
}

static int
macosx_bluetooth_set_powered(int powered)
{
    // Inspired by blueutil from Frederik Seiffert <ego@frederikseiffert.de>
    int state = IOBluetoothPreferenceGetControllerPowerState();

    if (state == powered) {
        return 1;
    }

    OSXPAIR_DEBUG("Switching Bluetooth %s...\n", powered?"on":"off");
    IOBluetoothPreferenceSetControllerPowerState(powered);

    // Wait a bit for Bluetooth to be (de-)activated
    psmove_port_sleep_ms(2000);

    if (IOBluetoothPreferenceGetControllerPowerState() != powered) {
        // Happened to me once while Bluetooth devices were connected
        return 0;
    }

    return 1;
}

static bool
macosx_blued_running()
{
    for (const auto &line: subprocess("/bin/ps -axo comm")) {
        if (line == "/usr/sbin/blued") {
            return true;
        }
    }

    return false;
}

static bool
macosx_blued_is_paired(const std::string &btaddr)
{
    /**
     * Example output that we need to parse:
     *
     * (
     *     "e0-ae-5e-00-00-00",
     *     "e0-ae-5e-aa-bb-cc",
     *     "00-06-f7-22-11-00",
     * )
     *
     **/

    for (const auto &line: subprocess("/usr/bin/defaults read " OSX_BT_CONFIG_PATH " HIDDevices")) {
        size_t pos = line.find('"');
        if (pos != std::string::npos) {
            ++pos;
            size_t rpos = line.rfind('"');
            if (rpos != std::string::npos) {
                std::string entry = line.substr(pos, rpos - pos);
                if (entry == btaddr) {
                    return true;
                }
            }
        }
    }

    return false;
}

struct MacOSVersion {
    static MacOSVersion
    running()
    {
        for (const auto &line: subprocess("/usr/bin/sw_vers -productVersion")) {
            int major, minor, patch = 0;
            int assigned = sscanf(line.c_str(), "%d.%d.%d", &major, &minor, &patch);

            /**
             * On Mac OS X 10.8.0, the command returns "10.8", so we allow parsing
             * only the first two numbers of the triplet, leaving the patch version
             * to the default (0) set above.
             *
             * See: https://github.com/thp/psmoveapi/issues/32
             **/
            if (assigned == 2 || assigned == 3) {
                return MacOSVersion {major, minor};
            }
        }

        return MacOSVersion {};
    }

    explicit MacOSVersion(int major=-1, int minor=-1) : major(major), minor(minor) {}

    bool operator<(const MacOSVersion &other) const {
        return major < other.major || (major == other.major && minor < other.minor);
    }

    bool operator>=(const MacOSVersion &other) const {
        return major > other.major || (major == other.major && minor >= other.minor);
    }

    bool valid() const { return major != -1 && minor != -1; }

    int major;
    int minor;
};

} // end anonymous namespace

bool
psmove_port_register_psmove(char *addr, char *host, enum PSMove_Model_Type model)
{
    // TODO: Host is ignored for now

    // TODO: FIXME: If necessary, handle different controller models differently.

    ScopedNSAutoreleasePool pool;
    std::string btaddr = _psmove_normalize_btaddr_inplace(addr, true, '-');

    if (btaddr.length() == 0) {
        OSXPAIR_DEBUG("Not a valid Bluetooth address: %s\n", addr);
        return false;
    }

    auto macos_version = MacOSVersion::running();
    if (!macos_version.valid()) {
        OSXPAIR_DEBUG("Cannot detect macOS version.\n");
        return false;
    } else if (macos_version >= MacOSVersion(13, 0)) {
        PSMOVE_WARNING("Pairing not yet supported on macOS Ventura, see https://github.com/thp/psmoveapi/issues/457");
        return false;
    } else if (macos_version < MacOSVersion(10, 7)) {
        OSXPAIR_DEBUG("No need to add entry for macOS before 10.7.\n");
        return false;
    }

    OSXPAIR_DEBUG("Detected: macOS %d.%d\n", macos_version.major, macos_version.minor);

    if (macosx_blued_is_paired(btaddr)) {
        OSXPAIR_DEBUG("Entry for %s already present.\n", btaddr.c_str());
        return true;
    }

    if (macos_version < MacOSVersion(10, 10)) {
        if (!macosx_bluetooth_set_powered(0)) {
            OSXPAIR_DEBUG("Cannot shutdown Bluetooth (shut it down manually).\n");
        }

        OSXPAIR_DEBUG("Waiting for blued shutdown (takes ca. 42s) ...\n");
        while (macosx_blued_running()) {
            psmove_port_sleep_ms(1000);
        }
        OSXPAIR_DEBUG("blued successfully shutdown.\n");
    }

    std::string command = format("/usr/bin/defaults write %s HIDDevices -array-add %s",
                                 OSX_BT_CONFIG_PATH, btaddr.c_str());
    if (geteuid() != 0) {
        // Not running using setuid or sudo, must use osascript to gain privileges
        command = format("/usr/bin/osascript -e 'do shell script \"%s\" with administrator privileges'",
                         command.c_str());
    }

    OSXPAIR_DEBUG("Running: '%s'\n", command.c_str());
    bool result = true;
    if (system(command.c_str()) != 0) {
        OSXPAIR_DEBUG("Could not run the command.");
        result = false;
    }

    if (macos_version < MacOSVersion(10, 10))
    {
        // FIXME: In OS X 10.7 this might not work - fork() and call set_powered(1)
        // from a fresh process (e.g. like "blueutil 1") to switch Bluetooth on
        macosx_bluetooth_set_powered(1);
    }

    return result;
}

void
psmove_port_initialize_sockets()
{
    // Nothing to do on OS X
}

int
psmove_port_check_pairing_permissions()
{
    // We also request root permissions on macOS, so that we have GUI-less pairing.
    if (geteuid() != 0) {
        printf("This program must be run as root (or use sudo).\n");
        return 0;
    }

    return 1;
}

uint64_t
psmove_port_get_time_ms()
{
    static uint64_t startup_time = 0;
    uint64_t now;
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }

    now = (tv.tv_sec * 1000 + tv.tv_usec / 1000);

    /* The first time this function gets called, we init startup_time */
    if (startup_time == 0) {
        startup_time = now;
    }

    return (now - startup_time);
}

void
psmove_port_set_socket_timeout_ms(int socket, uint32_t timeout_ms)
{
    struct timeval receive_timeout = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (int)(timeout_ms % 1000) * 1000,
    };
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&receive_timeout, sizeof(receive_timeout));
}

void
psmove_port_sleep_ms(uint32_t duration_ms)
{
    usleep(duration_ms * 1000);
}

void
psmove_port_close_socket(int socket)
{
    close(socket);
}

char *
psmove_port_get_host_bluetooth_address()
{
    ScopedNSAutoreleasePool pool;

    macosx_bluetooth_set_powered(1);

    IOBluetoothHostController *controller =
        [IOBluetoothHostController defaultController];

    NSString *addr = [controller addressAsString];
    psmove_return_val_if_fail(addr != NULL, NULL);

    char *result = strdup([addr UTF8String]);
    psmove_return_val_if_fail(result != NULL, NULL);

    if (_psmove_normalize_btaddr_inplace(result, true, ':') == NULL) {
        free(result);
        return NULL;
    }

    return result;
}
