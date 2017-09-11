/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2016, 2017 Thomas Perl <m@thp.io>
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


#include "psmove_config.h"
#include "psmove_port.h"
#include "psmove_sockets.h"
#include "psmove_private.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <list>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/ioctl.h>



#define LINUXPAIR_DEBUG(msg, ...) \
        psmove_PRINTF("PAIRING LINUX", msg, ## __VA_ARGS__)

namespace {

const std::string
BLUEZ5_INFO_ENTRY {
"[General]\n"
"Name=Motion Controller\n"
"Class=0x002508\n"
"SupportedTechnologies=BR/EDR\n"
"Trusted=true\n"
"Blocked=false\n"
"Services=00001124-0000-1000-8000-00805f9b34fb;\n"
"\n"
"[DeviceID]\n"
"Source=1\n"
"Vendor=1356\n"
"Product=981\n"
"Version=1\n" };

const std::string
BLUEZ5_CACHE_ENTRY {
"[General]\n"
"Name=Motion Controller\n"
"\n"
"[ServiceRecords]\n"
"0x00010000=3601920900000A000100000900013503191124090004350D"
"35061901000900113503190011090006350909656E09006A09010009000"
"93508350619112409010009000D350F350D350619010009001335031900"
"110901002513576972656C65737320436F6E74726F6C6C6572090101251"
"3576972656C65737320436F6E74726F6C6C6572090102251B536F6E7920"
"436F6D707574657220456E7465727461696E6D656E74090200090100090"
"2010901000902020800090203082109020428010902052801090206359A"
"35980822259405010904A101A102850175089501150026FF00810375019"
"513150025013500450105091901291381027501950D0600FF8103150026"
"FF0005010901A10075089504350046FF0009300931093209358102C0050"
"175089527090181027508953009019102750895300901B102C0A1028502"
"750895300901B102C0A10285EE750895300901B102C0A10285EF7508953"
"00901B102C0C00902073508350609040909010009020828000902092801"
"09020A280109020B09010009020C093E8009020D280009020E2800\n" };

std::string
bt_path_join(const std::string &directory, const std::string &filename)
{
    return directory + "/" + filename;
}

std::string
cstring_to_stdstring_free(char *cstring)
{
    std::string result { cstring ?: "" };
    free(cstring);
    return result;
}

struct bluetoothd {
    static void start() { control(true); }
    static void stop() { control(false); }

private:
    static void control(bool start) {
        std::list<std::string> services = { "bluetooth.service" };
        std::string verb { start ? "start" : "stop" };

#if defined(PSMOVE_USE_POCKET_CHIP)
        if (start) {
            // Need to start this before bluetoothd
            services.push_front("bt_rtk_hciattach@ttyS1.service");
        } else {
            // Need to stop this after bluetoothd
            services.push_back("bt_rtk_hciattach@ttyS1.service");
        }
#endif

        for (auto &service: services) {
            std::string cmd { "systemctl " + verb + " " + service };
            LINUXPAIR_DEBUG("Running: '%s'\n", cmd.c_str());
            if (system(cmd.c_str()) != 0) {
                LINUXPAIR_DEBUG("Automatic %s of %s failed.\n", verb.c_str(), service.c_str());
            }
            psmove_port_sleep_ms(500);
        }
    }
};

bool
file_exists(const std::string &filename)
{
    struct stat st;
    return (stat(filename.c_str(), &st) == 0 && !S_ISDIR(st.st_mode));
}

bool
directory_exists(const std::string &filename)
{
    struct stat st;
    return (stat(filename.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

bool
make_directory(const std::string &filename)
{
    return (mkdir(filename.c_str(), 0700) == 0);
}

bool
linux_bluez5_write_entry(const std::string &path, const std::string &contents)
{
    FILE *fp = fopen(path.c_str(), "w");

    if (fp == nullptr) {
        LINUXPAIR_DEBUG("Cannot open file for writing: %s\n", path.c_str());
        return false;
    }

    LINUXPAIR_DEBUG("Writing file: %s\n", path.c_str());
    fwrite(contents.c_str(), 1, contents.length(), fp);
    fclose(fp);

    return true;
}

bool
linux_bluez5_register_psmove(const std::string &addr, const std::string &bluetooth_dir)
{
    auto info_dir = bt_path_join(bluetooth_dir, addr);

    if (!directory_exists(info_dir)) {
        bluetoothd::stop();

        if (!make_directory(info_dir)) {
            LINUXPAIR_DEBUG("Cannot create directory: %s\n", info_dir.c_str());
            return false;
        }
    }

    auto info_file = bt_path_join(info_dir, "info");

    bluetoothd::stop();

    // Always write the info file, even if it exists already
    if (!linux_bluez5_write_entry(info_file, BLUEZ5_INFO_ENTRY)) {
        return false;
    }

    auto cache_dir = bt_path_join(bluetooth_dir, "cache");
    if (!directory_exists(cache_dir)) {
        bluetoothd::stop();

        if (!make_directory(cache_dir)) {
            LINUXPAIR_DEBUG("Cannot create directory: %s\n", cache_dir.c_str());
            return false;
        }
    }

    auto cache_file = bt_path_join(cache_dir, addr);
    if (!file_exists(cache_file)) {
        bluetoothd::stop();

        if (!linux_bluez5_write_entry(cache_file, BLUEZ5_CACHE_ENTRY)) {
            return false;
        }
    }

    bluetoothd::start();

    return true;
}

} // end anonymous namespace

void
psmove_port_initialize_sockets()
{
    // Nothing to do on Linux
}

int
psmove_port_check_pairing_permissions()
{
    /**
     * In order to be able to start/stop bluetoothd and to
     * add new entries to the Bluez configuration files, we
     * need to run as root (platform/psmove_linuxsupport.c)
     **/
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
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    now = (ts.tv_sec * 1000 + ts.tv_nsec / (1000 * 1000));

    /* The first time this function gets called, we init startup_time */
    if (startup_time == 0) {
        startup_time = now;
    }

    return (now - startup_time);
}

void
psmove_port_set_socket_timeout_ms(int socket, uint32_t timeout_ms)
{
    struct timeval receive_timeout;
    receive_timeout.tv_sec = timeout_ms / 1000u;
    receive_timeout.tv_usec = (timeout_ms % 1000u) * 1000u;
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

static int
_psmove_linux_bt_dev_info(int s, int dev_id, long arg)
{
    struct hci_dev_info di;
    di.dev_id = dev_id;

    unsigned char *btaddr = (unsigned char *)arg;

    if (ioctl(s, HCIGETDEVINFO, (void *)&di) == 0) {
        for (int i=0; i<6; i++) {
            btaddr[i] = di.bdaddr.b[i];
        }
    } else {
        LINUXPAIR_DEBUG("ioctl(HCIGETDEVINFO): %s", strerror(errno));
    }

    return 0;
}

static char *
_psmove_linux_get_bluetooth_address(bool try_restart)
{
    PSMove_Data_BTAddr btaddr;
    PSMove_Data_BTAddr blank;

    memset(blank, 0, sizeof(PSMove_Data_BTAddr));
    memset(btaddr, 0, sizeof(PSMove_Data_BTAddr));

    hci_for_each_dev(HCI_UP, _psmove_linux_bt_dev_info, (intptr_t)btaddr);

    if(memcmp(btaddr, blank, sizeof(PSMove_Data_BTAddr))==0) {
        if (try_restart) {
            // Force bluetooth restart
            bluetoothd::stop();
            bluetoothd::start();

            return _psmove_linux_get_bluetooth_address(false);
        }

        fprintf(stderr, "WARNING: Can't determine Bluetooth address.\n"
                "Make sure Bluetooth is turned on.\n");
        return NULL;
    }

    return _psmove_btaddr_to_string(btaddr);
}

char *
psmove_port_get_host_bluetooth_address()
{
    return _psmove_linux_get_bluetooth_address(true);
}

void
psmove_port_register_psmove(const char *addr, const char *host)
{
    auto controller_addr = cstring_to_stdstring_free(_psmove_normalize_btaddr(addr, 0, ':'));
    auto host_addr = cstring_to_stdstring_free(_psmove_normalize_btaddr(host, 0, ':'));

    if (controller_addr.empty()) {
        LINUXPAIR_DEBUG("Cannot parse controller address: '%s'\n", addr);
        return;
    }

    if (host_addr.empty()) {
        LINUXPAIR_DEBUG("Cannot parse host address: '%s'\n", host);
        return;
    }

    std::string base { "/var/lib/bluetooth/" + host_addr };
    if (!directory_exists(base)) {
        LINUXPAIR_DEBUG("Not a directory: %s\n", base.c_str());
        return;
    }

    linux_bluez5_register_psmove(controller_addr, base);
}
