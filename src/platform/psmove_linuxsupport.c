
#include "psmove_linuxsupport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <ctype.h>


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

char *
get_bluez_config_base_path()
{
    char *result = NULL;
    glob_t g;

    if (glob(BLUEZ_CONFIG_DIR "*", 0, NULL, &g) != 0) {
        printf("Warning: could not find state directory\n");
        return NULL;
    }

    if (g.gl_pathc == 1) {
        result = strdup(g.gl_pathv[0]);
    } else {
        printf("Need exactly one BT adapter, found %d\n", g.gl_pathc);
    }

    globfree(&g);
    return result;
}


struct entry_list_t {
    char *addr;
    char *entry;
    struct entry_list_t *next;
};

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
        printf("Can't open file for writing: '%s'\n", filename);
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

char *
normalize_addr(const char *addr)
{
    int count = strlen(addr);

    if (count != 17) {
        printf("Invalid address: '%s'\n", addr);
        return NULL;
    }

    char *result = malloc(count + 1);
    int i;

    for (i=0; i<strlen(addr); i++) {
        if (addr[i] >= 'A' && addr[i] <= 'F' && i % 3 != 2) {
            result[i] = addr[i];
        } else if (addr[i] >= '0' && addr[i] <= '9' && i % 3 != 2) {
            result[i] = addr[i];
        } else if (addr[i] >= 'a' && addr[i] <= 'f' && i % 3 != 2) {
            result[i] = toupper(addr[i]);
        } else if ((addr[i] == ':' || addr[i] == '-') && i % 3 == 2) {
            result[i] = ':';
        } else {
            printf("Invalid character at pos %d: '%c'\n", i, addr[i]);
            free(result);
            return NULL;
        }
    }

    result[count] = '\0';
    return result;
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

int
linux_bluez_register_psmove(char *addr)
{
    int errors = 0;

    addr = normalize_addr(addr);
    if (addr == NULL) {
        printf("Cannot parse bluetooth address!\n");
        return 0;
    }

    char *base = get_bluez_config_base_path();
    if (base == NULL) {
        printf("Can't find Bluetooth directory in '" BLUEZ_CONFIG_DIR "'\n");
        return 0;
    }

    // First, let's check if the entries are already okay..
    errors = for_all_entries(check_entry_in_file, base, addr);

    if (errors) {
        // In this case, we have missing or invalid values and need to update
        // the Bluetooth configuration files and restart Bluez' bluetoothd

        // FIXME: This is Ubuntu-specific
        if (system("service bluetooth stop") != 0) {
            printf("Automatic stopping of bluetoothd failed.\n"
                   "You might have to stop it manually before pairing.\n");
        }

        errors = for_all_entries(write_entry_to_file, base, addr);

        // FIXME: This is Ubuntu-specific
        if (system("service bluetooth start") != 0) {
            printf("Automatic starting of bluetoothd failed.\n"
                   "You might have to start it manually after pairing.\n");
        }

        free(base);
        free(addr);
    }

    return (errors == 0);
}

