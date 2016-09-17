
#include "../psmove_private.h"
#include "psmove_linuxsupport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define LINUXPAIR_DEBUG(msg, ...) \
        psmove_PRINTF("PAIRING LINUX", msg, ## __VA_ARGS__)


#define CLASSES_ENTRY " 0x002508"
#define DID_ENTRY " 0000 054C 03D5 0000"
#define FEATURES_ENTRY " BC04827E08080080"

// Don't write "lastused" entry (Bluez will take care of that)
// Don't write "manufacturers" entry (I've seen "10 3 6611" and "10 3 7221" so far)

#define NAMES_ENTRY " Motion Controller"
#define PROFILES_ENTRY " 00001124-0000-1000-8000-00805f9b34fb"
#define SDP_ENTRY "#00010000 " \
    "3601920900000A000100000900013503191124090004350D35061901000" \
    "900113503190011090006350909656E09006A0901000900093508350619" \
    "112409010009000D350F350D35061901000900133503190011090100251" \
    "3576972656C65737320436F6E74726F6C6C65720901012513576972656C" \
    "65737320436F6E74726F6C6C6572090102251B536F6E7920436F6D70757" \
    "4657220456E7465727461696E6D656E7409020009010009020109010009" \
    "02020800090203082109020428010902052801090206359A35980822259" \
    "405010904A101A102850175089501150026FF0081037501951315002501" \
    "3500450105091901291381027501950D0600FF8103150026FF000501090" \
    "1A10075089504350046FF0009300931093209358102C005017508952709" \
    "0181027508953009019102750895300901B102C0A102850275089530090" \
    "1B102C0A10285EE750895300901B102C0A10285EF750895300901B102C0" \
    "C0090207350835060904090901000902082800090209280109020A28010" \
    "9020B09010009020C093E8009020D280009020E2800"
#define TRUSTS_ENTRY " [all]"

#define BLUEZ_CONFIG_DIR "/var/lib/bluetooth/"


// Bluez 5.x support
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

struct entry_list_t {
    char *addr;
    char *entry;
    struct entry_list_t *next;
};

int linux_bluez5_register_psmove(struct linux_info_t *, char *, char *);

struct entry_list_t *
read_entry_list(const char *filename)
{
    struct entry_list_t *result = NULL;
    char tmp[2048];

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }

    while (!feof(fp)) {
        if (fgets(tmp, sizeof(tmp), fp)) {
            if (tmp[strlen(tmp)-1] == '\n') {
                // strip trailing newline
                tmp[strlen(tmp)-1] = '\0';
            }

            struct entry_list_t *entry = calloc(1, sizeof(struct entry_list_t));
            entry->addr = calloc(1, 17 + 1);
            memcpy(entry->addr, tmp, 17);
            entry->entry = strdup(tmp + 17);
            entry->next = result;
            result = entry;
        }
    }

    fclose(fp);

    return result;
}

void
free_entry_list(struct entry_list_t *list)
{
    struct entry_list_t *cur;

    while (list != NULL) {
        cur = list;
        list = list->next;

        free(cur->addr);
        free(cur->entry);

        free(cur);
    }
}

int
set_entry_list(struct entry_list_t **list, const char *addr, const char *entry)
{
    int found = 0;
    struct entry_list_t *cur = *list;

    while (cur != NULL) {
        if (strcmp(addr, cur->addr) == 0) {
            if (strcmp(entry, cur->entry) != 0) {
                // replace existing entry with different value
                free(cur->entry);
                cur->entry = strdup(entry);
                return 1;
            } else {
                found = 1;
            }
        }

        if (cur->next == NULL) {
            break;
        }
        cur = cur->next;
    }

    if (!found) {
        struct entry_list_t *next = calloc(1, sizeof(struct entry_list_t));
        next->addr = strdup(addr);
        next->entry = strdup(entry);
        if (cur) {
            cur->next = next;
        } else {
            *list = next;
        }
        return 1;
    }

    return 0;
}

int
write_entry_list(const char *filename, struct entry_list_t *list)
{
    // XXX: Create backup file of entry list

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        LINUXPAIR_DEBUG("Can't open file for writing: '%s'\n", filename);
        return 1;
    }

    struct entry_list_t *cur = list;

    while (cur != NULL) {
        fprintf(fp, "%s%s\n", cur->addr, cur->entry);
        cur = cur->next;
    }

    fclose(fp);
    return 0;
}

int
process_file_entry(const char *base, const char *filename,
        const char *addr, const char *entry, int write_file)
{
    int errors = 0;
    // base + '/' + filename + '\0'
    char *fn = malloc(strlen(base) + 1 + strlen(filename) + 1);
    strcpy(fn, base);
    strcat(fn, "/");
    strcat(fn, filename);

    struct entry_list_t *entries;

    entries = read_entry_list(fn);
    if (set_entry_list(&entries, addr, entry)) {
        if (write_file) {
            errors += write_entry_list(fn, entries);
        } else {
            errors += 1;
        }
    }
    free_entry_list(entries);

    free(fn);
    return errors;
}

int
check_entry_in_file(const char *base, const char *filename,
        const char *addr, const char *entry)
{
    return process_file_entry(base, filename, addr, entry, 0);
}

int
write_entry_to_file(const char *base, const char *filename,
        const char *addr, const char *entry)
{
    return process_file_entry(base, filename, addr, entry, 1);
}

typedef int (*for_all_entries_func)(const char *base, const char *filename,
        const char *addr, const char *entry);

int
for_all_entries(for_all_entries_func func, const char *base, const char *addr)
{
    int errors = 0;

    errors += func(base, "classes", addr, CLASSES_ENTRY);
    errors += func(base, "did", addr, DID_ENTRY);
    errors += func(base, "features", addr, FEATURES_ENTRY);
    errors += func(base, "names", addr, NAMES_ENTRY);
    errors += func(base, "profiles", addr, PROFILES_ENTRY);
    errors += func(base, "sdp", addr, SDP_ENTRY);
    errors += func(base, "trusts", addr, TRUSTS_ENTRY);

    return errors;
}

static char *
bt_path_join(const char *directory, const char *filename)
{
    char *result = (char *)malloc(strlen(directory) + 1 + strlen(filename) + 1);
    sprintf(result, "%s/%s", directory, filename);
    return result;
}

void linux_info_init(struct linux_info_t *info)
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

void linux_bluetoothd_control(struct linux_info_t *info, int start)
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
}

int
linux_bluez_register_psmove(char *addr, char *host)
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

    base = malloc(strlen(BLUEZ_CONFIG_DIR) + strlen(host_addr) + 1);
    strcpy(base, BLUEZ_CONFIG_DIR);
    strcat(base, host_addr);

    struct stat st;
    if (stat(base, &st) != 0 || !S_ISDIR(st.st_mode)) {
        LINUXPAIR_DEBUG("Not a directory: %s\n", base);
        errors++;
        goto cleanup;
    }

#ifdef PSMOVE_BLUEZ5_SUPPORT
    errors = linux_bluez5_register_psmove(&linux_info, controller_addr, base);
    goto cleanup;
#endif

    // First, let's check if the entries are already okay..
    errors = for_all_entries(check_entry_in_file, base, controller_addr);

    if (errors) {
        // In this case, we have missing or invalid values and need to update
        // the Bluetooth configuration files and restart Bluez' bluetoothd

        linux_bluetoothd_control(&linux_info, 0);

        errors = for_all_entries(write_entry_to_file, base, controller_addr);

        linux_bluetoothd_control(&linux_info, 1);
    }

cleanup:
    free(base);
    free(host_addr);
    free(controller_addr);

    return (errors == 0);
}

int linux_bluez5_write_entry(char *path, char *contents)
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

int linux_bluez5_write_info(char *info_dir)
{
    int errors = 0;
    char *info_file = bt_path_join(info_dir, BLUEZ5_INFO_FILE);

    errors = linux_bluez5_write_entry(info_file, BLUEZ5_INFO_ENTRY);

    free(info_file);
    return errors;
}

int linux_bluez5_write_cache(struct linux_info_t *linux_info, char *cache_dir, char *addr)
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

// Bluez 5.x has new storage structure, incompatible with Bluez 4.x
int linux_bluez5_register_psmove(struct linux_info_t *linux_info,
                                 char *addr,
                                 char *bluetooth_dir)
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
