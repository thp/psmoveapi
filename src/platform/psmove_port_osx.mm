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


#include "psmove_port.h"
#include "psmove_sockets.h"
#include "psmove_private.h"

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

#define OSXPAIR_DEBUG(msg, ...) \
        psmove_PRINTF("PAIRING OSX", msg, ## __VA_ARGS__)

struct ScopedNSAutoreleasePool {
    ScopedNSAutoreleasePool()
        : pool([[NSAutoreleasePool alloc] init])
    {}

    ~ScopedNSAutoreleasePool()
    {
        [pool release];
    }

private:
    NSAutoreleasePool *pool;
};

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

static char *
macosx_get_btaddr()
{
    ScopedNSAutoreleasePool pool;

    char *result;

    macosx_bluetooth_set_powered(1);

    IOBluetoothHostController *controller =
        [IOBluetoothHostController defaultController];

    NSString *addr = [controller addressAsString];
    psmove_return_val_if_fail(addr != NULL, NULL);

    result = strdup([addr UTF8String]);
    psmove_return_val_if_fail(result != NULL, NULL);

    char *tmp = result;
    while (*tmp) {
        if (*tmp == '-') {
            *tmp = ':';
        }

        tmp++;
    }

    return result;
}

static int
macosx_blued_running()
{
    FILE *fp = popen("ps -axo comm", "r");
    char command[1024];
    int running = 0;

    while (fgets(command, sizeof(command), fp)) {
        /* Remove trailing newline */
        command[strlen(command)-1] = '\0';

        if (strcmp(command, "/usr/sbin/blued") == 0) {
            running = 1;
        }
    }

    pclose(fp);

    return running;
}

static int
macosx_blued_is_paired(const std::string &btaddr)
{
    FILE *fp = popen("defaults read " OSX_BT_CONFIG_PATH " HIDDevices", "r");
    char line[1024];
    int found = 0;

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

    while (fgets(line, sizeof(line), fp)) {
        char *entry = strchr(line, '"');
        if (entry) {
            entry++;
            char *delim = strchr(entry, '"');
            if (delim) {
                *delim = '\0';
                if (strcmp(entry, btaddr.c_str()) == 0) {
                    found = 1;
                }
            }
        }
    }

    pclose(fp);
    return found;
}

static int
macosx_get_minor_version()
{
    char tmp[1024];
    int major, minor, patch = 0;
    FILE *fp;

    fp = popen("sw_vers -productVersion", "r");
    psmove_return_val_if_fail(fp != NULL, -1);
    psmove_return_val_if_fail(fgets(tmp, sizeof(tmp), fp) != NULL, -1);
    pclose(fp);

    int assigned = sscanf(tmp, "%d.%d.%d", &major, &minor, &patch);

    /**
     * On Mac OS X 10.8.0, the command returns "10.8", so we allow parsing
     * only the first two numbers of the triplet, leaving the patch version
     * to the default (0) set above.
     *
     * See: https://github.com/thp/psmoveapi/issue/32
     **/
    psmove_return_val_if_fail(assigned == 2 || assigned == 3, -1);

    return minor;
}

namespace {

std::string format(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

std::string
format(const char *fmt, ...)
{
    std::string result;

    va_list ap;
    va_start(ap, fmt);

    char *tmp = 0;
    vasprintf(&tmp, fmt, ap);
    va_end(ap);

    result = tmp;
    free(tmp);

    return result;
}

std::string
cstring_to_stdstring_free(char *cstring)
{
    std::string result = cstring ?: "";
    free(cstring);
    return result;
}

};

enum PSMove_Bool
psmove_port_register_psmove(const char *addr, const char *host, enum PSMove_Model_Type model)
{
    enum PSMove_Bool result = PSMove_True;

    // TODO: FIXME: If necessary, handle different controller models differently.

    ScopedNSAutoreleasePool pool;
    std::string btaddr = cstring_to_stdstring_free(_psmove_normalize_btaddr(addr, 1, '-'));

    if (btaddr.length() == 0) {
        OSXPAIR_DEBUG("Not a valid Bluetooth address: %s\n", addr);
        return PSMove_False;
    }

    int minor_version = macosx_get_minor_version();
    if (minor_version == -1) {
        OSXPAIR_DEBUG("Cannot detect Mac OS X version.\n");
        return PSMove_False;
    } else if (minor_version < 7) {
        OSXPAIR_DEBUG("No need to add entry for OS X before 10.7.\n");
        return PSMove_False;
    } else {
        OSXPAIR_DEBUG("Detected: Mac OS X 10.%d\n", minor_version);
    }

    std::string command = format("defaults write %s HIDDevices -array-add %s",
                                 OSX_BT_CONFIG_PATH, btaddr.c_str());

    if (macosx_blued_is_paired(btaddr)) {
        OSXPAIR_DEBUG("Entry for %s already present.\n", btaddr.c_str());
        return PSMove_True;
    }

    if (minor_version < 10)
    {
        if (!macosx_bluetooth_set_powered(0)) {
            OSXPAIR_DEBUG("Cannot shutdown Bluetooth (shut it down manually).\n");
        }

        int i = 0;
        OSXPAIR_DEBUG("Waiting for blued shutdown (takes ca. 42s) ...\n");
        while (macosx_blued_running()) {
            psmove_port_sleep_ms(1000);
            i++;
        }
        OSXPAIR_DEBUG("blued successfully shutdown.\n");
    }

    if (geteuid() != 0) {
        // Not running using setuid or sudo, must use osascript to gain privileges
        command = format("osascript -e 'do shell script \"%s\" with administrator privileges'",
                         command.c_str());
    }

    OSXPAIR_DEBUG("Running: '%s'\n", command.c_str());
    if (system(command.c_str()) != 0) {
        OSXPAIR_DEBUG("Could not run the command.");
        result = PSMove_False;
    }

    if (minor_version < 10)
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
    return macosx_get_btaddr();
}
