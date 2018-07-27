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
#include <vector>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/ioctl.h>



#define LINUXPAIR_DEBUG(msg, ...) \
        psmove_PRINTF("PAIRING LINUX", msg, ## __VA_ARGS__)

namespace {

const std::string
BLUEZ5_INFO_ENTRY(unsigned short pid) {
    return
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
        "Product=" + std::to_string(pid) + "\n"
        "Version=1\n";
};

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
cstring_to_stdstring_free(char *cstring)
{
    std::string result { cstring ?: "" };
    free(cstring);
    return result;
}

bool
file_exists(const std::string &filename)
{
    struct stat st;
    return (stat(filename.c_str(), &st) == 0 && !S_ISDIR(st.st_mode));
}

struct BluetoothDaemon {
    ~BluetoothDaemon() { restart_if_needed(); }
    void restart_if_needed() { if (stopped) { control(true); } }
    void start() { control(true); }
    void stop() { if (!stopped) { control(false); } }
    void force_restart() { stop(); start(); }

private:
    bool started { false };
    bool stopped { false };

    void control(bool start) {
        std::list<std::string> services = { "bluetooth.service" };
        std::string verb { start ? "start" : "stop" };

        if (file_exists("/lib/systemd/system/bt_rtk_hciattach@.service")) {
            LINUXPAIR_DEBUG("Detected: Pocket C.H.I.P\n");

            if (start) {
                // Need to start this before bluetoothd
                services.push_front("bt_rtk_hciattach@ttyS1.service");
            } else {
                // Need to stop this after bluetoothd
                services.push_back("bt_rtk_hciattach@ttyS1.service");
            }
        }

        for (auto &service: services) {
            std::string cmd { "systemctl " + verb + " " + service };
            LINUXPAIR_DEBUG("Running: '%s'\n", cmd.c_str());
            if (system(cmd.c_str()) != 0) {
                LINUXPAIR_DEBUG("Automatic %s of %s failed.\n", verb.c_str(), service.c_str());
            }
            psmove_port_sleep_ms(500);
        }

        stopped = !start;
        started = start;
    }
};

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
linux_bluez5_update_file_content(BluetoothDaemon &bluetoothd, const std::string &path, const std::string &contents)
{
    bool must_update = false;

    if (file_exists(path)) {
        FILE *fp = fopen(path.c_str(), "r");

        if (fp == nullptr) {
            LINUXPAIR_DEBUG("Cannot open file for reading: %s\n", path.c_str());
            return false;
        }
        fseek(fp, 0, SEEK_END);
        size_t len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<char> buf(len);
        if (fread(buf.data(), buf.size(), 1, fp) != 1) {
            LINUXPAIR_DEBUG("Cannot read file: %s\n", path.c_str());
            fclose(fp);
            return false;
        }

        fclose(fp);

        if (std::string(buf.data(), buf.size()) == contents) {
            LINUXPAIR_DEBUG("File %s is already up to date.\n", path.c_str());
            return true;
        } else {
            LINUXPAIR_DEBUG("File %s needs updating.\n", path.c_str());
        }
    }

    // Stop daemon to update files
    bluetoothd.stop();

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

struct BTAddr {
    BTAddr(bdaddr_t &addr) { memcpy(&d, &addr, sizeof(d)); }

    std::string to_string() { return cstring_to_stdstring_free(_psmove_btaddr_to_string(d)); }

    PSMove_Data_BTAddr d;
};

static int
_psmove_linux_bt_dev_info(int socket, int dev_id, long arg)
{
    struct hci_dev_info di;
    di.dev_id = dev_id;

    auto btaddrs = static_cast<std::vector<BTAddr> *>((void *)(intptr_t)arg);

    if (ioctl(socket, HCIGETDEVINFO, (void *)&di) == 0) {
        btaddrs->emplace_back(di.bdaddr);
        LINUXPAIR_DEBUG("Found host adapter: %s (name=%s)\n", btaddrs->back().to_string().c_str(), di.name);
    } else {
        LINUXPAIR_DEBUG("ioctl(HCIGETDEVINFO): %s", strerror(errno));
    }

    return 0;
}


static std::string
_psmove_linux_get_bluetooth_address(int retries)
{
    std::vector<BTAddr> btaddrs;

    hci_for_each_dev(0, _psmove_linux_bt_dev_info, (intptr_t)&btaddrs);

    if (btaddrs.size() == 0) {
        // Only try to (re-)start the Bluetooth service if we have correct permissions
        if (psmove_port_check_pairing_permissions()) {
            if (retries == 2) {
                fprintf(stderr, "Bluetooth address not found, trying to start the service.\n");
                BluetoothDaemon().start();
                // Back off timer
                psmove_port_sleep_ms(1000);
                return _psmove_linux_get_bluetooth_address(retries - 1);
            } else if (retries == 1) {
                fprintf(stderr, "Bluetooth address still not found, trying a force-restart.\n");
                BluetoothDaemon().force_restart();
                // Back off even more
                psmove_port_sleep_ms(2000);
                return _psmove_linux_get_bluetooth_address(retries - 1);
            }
        }

        psmove_WARNING("Can't determine Bluetooth address. Make sure Bluetooth is turned on.\n");
        return "";
    } else if (btaddrs.size() > 1) {
        // TODO: Normalize prefer_addr using _psmove_normalize_btaddr()?
        std::string prefer_addr { getenv("PSMOVE_PREFER_BLUETOOTH_HOST_ADDRESS") ?: "" };
        if (!prefer_addr.empty()) {
            for (auto &btaddr: btaddrs) {
                if (btaddr.to_string() == prefer_addr) {
                    printf("Using preferred address %s from environment.\n", btaddr.to_string().c_str());
                    return btaddr.to_string();
                }
            }
        }

        printf("Multiple Bluetooth adapters found, you can choose one via:\n");
        for (auto &btaddr: btaddrs) {
            printf("    export PSMOVE_PREFER_BLUETOOTH_HOST_ADDRESS=%s\n", btaddr.to_string().c_str());
        }
    }

    return btaddrs[0].to_string();
}

char *
psmove_port_get_host_bluetooth_address()
{
    auto result = _psmove_linux_get_bluetooth_address(2);
    return result.empty() ? nullptr : strdup(result.c_str());
}

void
psmove_port_register_psmove(const char *addr, const char *host, enum PSMove_Model_Type model)
{
    auto controller_addr = cstring_to_stdstring_free(_psmove_normalize_btaddr(addr, 0, ':'));
    auto host_addr = cstring_to_stdstring_free(_psmove_normalize_btaddr(host, 0, ':'));

    unsigned short pid = (model == Model_ZCM2) ? PSMOVE_PS4_PID : PSMOVE_PID;

    if (controller_addr.empty()) {
        LINUXPAIR_DEBUG("Cannot parse controller address: '%s'\n", addr);
        return;
    }

    if (host_addr.empty()) {
        LINUXPAIR_DEBUG("Cannot parse host address: '%s'\n", host);
        return;
    }

    std::string bluetooth_dir { "/var/lib/bluetooth/" + host_addr };
    if (!directory_exists(bluetooth_dir)) {
        LINUXPAIR_DEBUG("Not a directory: %s\n", bluetooth_dir.c_str());
        return;
    }

    BluetoothDaemon bluetoothd;

    std::string info_dir { bluetooth_dir + "/" + controller_addr };
    if (!directory_exists(info_dir)) {
        bluetoothd.stop();

        if (!make_directory(info_dir)) {
            LINUXPAIR_DEBUG("Cannot create directory: %s\n", info_dir.c_str());
            return;
        }
    }

    std::string cache_dir { bluetooth_dir + "/cache" };
    if (!directory_exists(cache_dir)) {
        bluetoothd.stop();

        if (!make_directory(cache_dir)) {
            LINUXPAIR_DEBUG("Cannot create directory: %s\n", cache_dir.c_str());
            return;
        }
    }

    if (!linux_bluez5_update_file_content(bluetoothd, info_dir + "/info", BLUEZ5_INFO_ENTRY(pid))) {
        return;
    }

    if (!linux_bluez5_update_file_content(bluetoothd, cache_dir + "/" + controller_addr, BLUEZ5_CACHE_ENTRY)) {
        return;
    }
}
