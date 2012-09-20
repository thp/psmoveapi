
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

#include "psmove_osxsupport.h"
#include "../psmove_private.h"

#include <IOBluetooth/objc/IOBluetoothHostController.h>

/* Location for the plist file that we want to modify */
#define OSX_BT_CONFIG_PATH "/Library/Preferences/com.apple.Bluetooth"

/* Function declarations for IOBluetooth private API */
void IOBluetoothPreferenceSetControllerPowerState(int);
int IOBluetoothPreferenceGetControllerPowerState();

#define OSXPAIR_DEBUG(msg, ...) \
        psmove_PRINTF("PAIRING OSX", msg, ## __VA_ARGS__)

int
macosx_bluetooth_set_powered(int powered)
{
    // Inspired by blueutil from Frederik Seiffert <ego@frederikseiffert.de>
    int state = IOBluetoothPreferenceGetControllerPowerState();
    if (powered != state) {
        OSXPAIR_DEBUG("Switching Bluetooth %s...\n", powered?"on":"off");
        IOBluetoothPreferenceSetControllerPowerState(powered);

        // Wait a bit for Bluetooth to be (de-)activated
        usleep(1000000);

        if (IOBluetoothPreferenceGetControllerPowerState() != powered) {
            // Happened to me once while Bluetooth devices were connected
            return 0;
        }
    }

    return 1;
}

char *
macosx_get_btaddr()
{
    const char *result;

    macosx_bluetooth_set_powered(1);

    IOBluetoothHostController *controller =
        [IOBluetoothHostController defaultController];

    NSString *addr = [controller addressAsString];
    psmove_return_val_if_fail(addr != NULL, NULL);

    result = [addr UTF8String];
    psmove_return_val_if_fail(result != NULL, NULL);

    return strdup(result);
}

int
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

int
macosx_blued_is_paired(char *btaddr)
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
                if (strcmp(entry, btaddr) == 0) {
                    found = 1;
                }
            }
        }
    }

    pclose(fp);
    return found;
}

int
macosx_blued_register_psmove(char *addr)
{
    char cmd[1024];
    char *btaddr = _psmove_normalize_btaddr(addr, 1, '-');

    if (macosx_blued_is_paired(btaddr)) {
        OSXPAIR_DEBUG("Entry for %s already present.\n", btaddr);
        return 1;
    }

    if (!macosx_bluetooth_set_powered(0)) {
        OSXPAIR_DEBUG("Cannot shutdown Bluetooth.\n");
        return 0;
    }

    int i = 0;
    OSXPAIR_DEBUG("Waiting for blued shutdown (takes ca. 42s) ...\n");
    while (macosx_blued_running()) {
        usleep(1000000);
        i++;
    }
    OSXPAIR_DEBUG("blued successfully shutdown.\n");

    snprintf(cmd, sizeof(cmd), "osascript -e 'do shell script "
            "\"defaults write " OSX_BT_CONFIG_PATH
                " HIDDevices -array-add \\\"%s\\\"\""
            " with administrator privileges'", btaddr);
    OSXPAIR_DEBUG("Running: '%s'\n", cmd);
    if (system(cmd) != 0) {
        OSXPAIR_DEBUG("Could not run the command.");
    }
    macosx_bluetooth_set_powered(1);
    free(btaddr);

    return 1;
}

