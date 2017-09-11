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

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/ioctl.h>



#define LINUXPAIR_DEBUG(msg, ...) \
        psmove_PRINTF("PAIRING LINUX", msg, ## __VA_ARGS__)

#define BLUEZ_CONFIG_DIR "/var/lib/bluetooth/"


#define BLUEZ5_INFO_ENTRY "[General]\n" \
    "Name=Motion Controller\n" \
    "Class=0x002508\n" \
    "SupportedTechnologies=BR/EDR\n" \
    "Trusted=true\n" \
    "Blocked=false\n" \
    "Services=00001124-0000-1000-8000-00805f9b34fb;\n" \
    "\n" \
    "[DeviceID]\n" \
    "Source=1\n" \
    "Vendor=1356\n" \
    "Product=981\n" \
    "Version=1\n" \

#define BLUEZ5_INFO_FILE "info"

#define BLUEZ5_CACHE_ENTRY "[General]\n" \
    "Name=Motion Controller\n" \
    "\n" \
    "[ServiceRecords]\n" \
    "0x00010000=3601920900000A000100000900013503191124090004350D" \
    "35061901000900113503190011090006350909656E09006A09010009000" \
    "93508350619112409010009000D350F350D350619010009001335031900" \
    "110901002513576972656C65737320436F6E74726F6C6C6572090101251" \
    "3576972656C65737320436F6E74726F6C6C6572090102251B536F6E7920" \
    "436F6D707574657220456E7465727461696E6D656E74090200090100090" \
    "2010901000902020800090203082109020428010902052801090206359A" \
    "35980822259405010904A101A102850175089501150026FF00810375019" \
    "513150025013500450105091901291381027501950D0600FF8103150026" \
    "FF0005010901A10075089504350046FF0009300931093209358102C0050" \
    "175089527090181027508953009019102750895300901B102C0A1028502" \
    "750895300901B102C0A10285EE750895300901B102C0A10285EF7508953" \
    "00901B102C0C00902073508350609040909010009020828000902092801" \
    "09020A280109020B09010009020C093E8009020D280009020E2800\n"

#define BLUEZ5_CACHE_DIR "cache"

enum linux_init_type {
    LINUX_SYSTEMD = 0,
    LINUX_UPSTART,
    LINUX_SYSVINIT
};

struct linux_info_t {
    enum linux_init_type init_type;
    int bluetoothd_stopped;
};

static char *
bt_path_join(const char *directory, const char *filename)
{
    char *result = (char *)malloc(strlen(directory) + 1 + strlen(filename) + 1);
    sprintf(result, "%s/%s", directory, filename);
    return result;
}

static void
linux_info_init(struct linux_info_t *info)
{
    FILE *fp = NULL;
    char str[512];

    info->bluetoothd_stopped = 0;
    info->init_type = LINUX_SYSVINIT;   // sysvinit by default

    // determine distro and thus init system type by reading /etc/os-release
    fp = fopen("/etc/os-release", "r");
    if (fp != NULL) {
        while (fgets(str, 512, fp) != NULL) {
            char *p, *q, *value;

            // ignore comments
            if (str[0] == '#') {
                continue;
            }

            // split into name=value
            p = strchr(str, '=');
            if (!p) {
                continue;
            }
            *p++ = 0;

            if (strcmp(str, "NAME")) {
                // we're interested only in NAME, so we don't handle other values
                continue;
            }

            // remove quotes and newline; un-escape
            value = p;
            q = value;
            while (*p) {
                if (*p == '\\') {
                    ++p;
                    if (!*p)
                        break;
                    *q++ = *p++;
                } else if ((*p == '\'') || (*p == '"') || (*p == '\n')) {
                    ++p;
                } else {
                    *q++ = *p++;
                }
            }
            *q = 0;

            if ((!strcmp(value, "openSUSE")) ||
                (!strcmp(value, "Fedora")) ||
                (!strcmp(value, "Arch Linux"))) {
                    // all recent versions of openSUSE and Fedora use
                    // systemd
                    info->init_type = LINUX_SYSTEMD;
            }
            else if (!strcmp(value, "Ubuntu")) {
                    // Ubuntu uses upstart now, but in the future it is
                    // going to switch to systemd
                    info->init_type = LINUX_UPSTART;
            }
            break;
        }

        fclose(fp);
    }
}

static void
linux_bluetoothd_control(struct linux_info_t *info, int start)
{
    char *cmd = NULL;

    if (start) {
        // start request
        if (!(info->bluetoothd_stopped)) {
            // already running
            return;
        }
        info->bluetoothd_stopped = 0;
        LINUXPAIR_DEBUG("Trying to start bluetoothd...\n");
    }
    else {
        // stop request
        if (info->bluetoothd_stopped) {
            // already stopped
            return;
        }
        info->bluetoothd_stopped = 1;
        LINUXPAIR_DEBUG("Trying to stop bluetoothd...\n");
    }

#if defined(PSMOVE_USE_POCKET_CHIP)
    const char *start_services[] = {
        "bt_rtk_hciattach@ttyS1.service",
        "bluetooth.service",
        NULL,
    };
    const char *stop_services[] = {
        "bluetooth.service",
        "bt_rtk_hciattach@ttyS1.service",
        NULL,
    };

    const char **services = start ? start_services : stop_services;
    const char *verb = start ? "start" : "stop";

    LINUXPAIR_DEBUG("Using systemd with Pocket C.H.I.P quirk...\n");
    for (int i=0; services[i] != NULL; i++) {
        asprintf(&cmd, "systemctl %s %s", verb, services[i]);
        LINUXPAIR_DEBUG("Running: '%s'\n", cmd);
        if (system(cmd) != 0) {
            LINUXPAIR_DEBUG("Automatic %s of %s failed.\n", verb, services[i]);
        }
        free(cmd), cmd = NULL;
        usleep(500000);
    }
#else
    switch (info->init_type) {
    case LINUX_SYSTEMD:
        cmd = start ? "systemctl start bluetooth.service" :
                      "systemctl stop bluetooth.service";
        LINUXPAIR_DEBUG("Using systemd...\n");
        break;
    case LINUX_UPSTART:
        cmd = start ? "service bluetooth start" :
                      "service bluetooth stop";
        LINUXPAIR_DEBUG("Using upstart...\n");
        break;
    case LINUX_SYSVINIT:
        cmd = start ? "/etc/init.d/bluetooth start" :
                      "/etc/init.d/bluetooth stop";
        LINUXPAIR_DEBUG("Using sysvinit...\n");
    default:
        break;
    }

    if (system(cmd) != 0) {
        LINUXPAIR_DEBUG("Automatic starting/stopping of bluetoothd failed.\n"
               "You might have to start/stop it manually.\n");
    }
    else {
        LINUXPAIR_DEBUG("Succeeded\n");
    }
#endif
}

static int
linux_bluez5_write_entry(char *path, char *contents)
{
    FILE *fp = NULL;
    int errors = 0;

    fp = fopen(path, "w");
    if (fp == NULL) {
        LINUXPAIR_DEBUG("Cannot open file for writing: %s\n", path);
        errors++;
    }
    else {
        fwrite((const void *)contents, 1, strlen(contents), fp);
        fclose(fp);
    }

    return errors;
}

static int
linux_bluez5_write_info(char *info_dir)
{
    int errors = 0;
    char *info_file = bt_path_join(info_dir, BLUEZ5_INFO_FILE);

    errors = linux_bluez5_write_entry(info_file, BLUEZ5_INFO_ENTRY);

    free(info_file);
    return errors;
}

static int
linux_bluez5_write_cache(struct linux_info_t *linux_info, char *cache_dir, char *addr)
{
    int errors = 0;
    char *cache_file = bt_path_join(cache_dir, addr);
    struct stat st;

    // if cache file already exists, do nothing
    if (stat(cache_file, &st) != 0) {
        linux_bluetoothd_control(linux_info, 0);

        // no cache file, create it
        errors = linux_bluez5_write_entry(cache_file, BLUEZ5_CACHE_ENTRY);
    }

    free(cache_file);
    return errors;
}

static int
linux_bluez5_register_psmove(struct linux_info_t *linux_info, char *addr, char *bluetooth_dir)
{
    char *info_dir = NULL;
    char *cache_dir = NULL;
    int errors = 0;
    struct stat st;

    info_dir = bt_path_join(bluetooth_dir, addr);

    if (stat(info_dir, &st) != 0) {
        linux_bluetoothd_control(linux_info, 0);

        // create info directory
        if (mkdir(info_dir, 0700) != 0) {
            LINUXPAIR_DEBUG("Cannot create directory: %s\n", info_dir);
            errors++;
            goto cleanup;
        }

        errors += linux_bluez5_write_info(info_dir);
    }

    cache_dir = bt_path_join(bluetooth_dir, BLUEZ5_CACHE_DIR);

    if (stat(cache_dir, &st) != 0) {
        linux_bluetoothd_control(linux_info, 0);

        // create cache directory
        if (mkdir(cache_dir, 0700) != 0) {
            LINUXPAIR_DEBUG("Cannot create directory: %s\n", info_dir);
            errors++;
            goto cleanup;
        }
    }

    errors += linux_bluez5_write_cache(linux_info, cache_dir, addr);

    linux_bluetoothd_control(linux_info, 1);

cleanup:
    if (info_dir != NULL) {
        free(info_dir);
    }
    if (cache_dir != NULL) {
        free(cache_dir);
    }

    return (errors == 0);
}

static int
linux_bluez_register_psmove(const char *addr, const char *host)
{
    int errors = 0;
    char *base = NULL;
    struct linux_info_t linux_info;

    linux_info_init(&linux_info);

    char *controller_addr = _psmove_normalize_btaddr(addr, 0, ':');
    char *host_addr = _psmove_normalize_btaddr(host, 0, ':');

    if (controller_addr == NULL) {
        LINUXPAIR_DEBUG("Cannot parse controller address: '%s'\n", addr);
        errors++;
        goto cleanup;
    }

    if (host_addr == NULL) {
        LINUXPAIR_DEBUG("Cannot parse host address: '%s'\n", host);
        errors++;
        goto cleanup;
    }

    base = (char *)malloc(strlen(BLUEZ_CONFIG_DIR) + strlen(host_addr) + 1);
    strcpy(base, BLUEZ_CONFIG_DIR);
    strcat(base, host_addr);

    struct stat st;
    if (stat(base, &st) != 0 || !S_ISDIR(st.st_mode)) {
        LINUXPAIR_DEBUG("Not a directory: %s\n", base);
        errors++;
        goto cleanup;
    }

    errors = linux_bluez5_register_psmove(&linux_info, controller_addr, base);

cleanup:
    free(base);
    free(host_addr);
    free(controller_addr);

    return (errors == 0);
}

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
    struct timeval receive_timeout = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000,
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

static int
_psmove_linux_bt_dev_info(int s, int dev_id, long arg)
{
    struct hci_dev_info di = { .dev_id = dev_id };
    unsigned char *btaddr = (unsigned char *)arg;
    int i;

    if (ioctl(s, HCIGETDEVINFO, (void *) &di) == 0) {
        for (i=0; i<6; i++) {
            btaddr[i] = di.bdaddr.b[i];
        }
    }

    return 0;
}

static char *
_psmove_linux_get_bluetooth_address(int try_restart)
{
    PSMove_Data_BTAddr btaddr;
    PSMove_Data_BTAddr blank;

    memset(blank, 0, sizeof(PSMove_Data_BTAddr));
    memset(btaddr, 0, sizeof(PSMove_Data_BTAddr));

    hci_for_each_dev(HCI_UP, _psmove_linux_bt_dev_info, (long)btaddr);
    if(memcmp(btaddr, blank, sizeof(PSMove_Data_BTAddr))==0) {
        if (try_restart) {
            struct linux_info_t linux_info;
            linux_info_init(&linux_info);

            // Force bluetooth restart
            linux_bluetoothd_control(&linux_info, 0);
            linux_bluetoothd_control(&linux_info, 1);
            return _psmove_linux_get_bluetooth_address(0);
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
    return _psmove_linux_get_bluetooth_address(1);
}

void
psmove_port_register_psmove(const char *addr, const char *host)
{
    /* Add entry to Bluez' bluetoothd state file */
    linux_bluez_register_psmove(addr, host);
}
