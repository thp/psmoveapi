#include <stdio.h>
#include <stdlib.h>

#include "psmove.h"
#include "../psmove_private.h"


int
main(int argc, char* argv[])
{
    int result = 0;

    int count = psmove_count_connected();

    if (count == 0) {
        printf("No controllers connected.\n");
        return 1;
    }

    for (int i=0; i<count; ++i) {
        PSMove *move = psmove_connect_by_id(i);

        if (move == NULL) {
            printf("Cannot connect to controller #%d.\n", i);
            result = 1;
            continue;
        }

#ifdef __APPLE__
        if (psmove_connection_type(move) != Conn_USB) {
            printf("Controller must be connected via USB.\n");
            psmove_disconnect(move);
            return 1;
        }
#endif

        printf("Controller #%d:\n", i);
        printf("Serial: %s\n", psmove_get_serial(move));

        PSMove_Firmware_Info *info = _psmove_get_firmware_info(move);
        if (info == NULL) {
            printf("Failed to retrieve firmware info for controller #%d.\n", i);
            psmove_disconnect(move);
            result = 1;
            continue;
        }

        printf("controller model:    ");
        switch (psmove_get_model(move)) {
            case Model_ZCM1:
                printf("CECH-ZCM1\n");
                break;
            case Model_ZCM2:
                printf("CECH-ZCM2\n");
                break;
            case Model_Unknown:
            default:
                printf("unknown\n");
        }

        printf("firmware version:    0x%04x, rev. %d\n", info->version, info->revision);
        printf("BT firmware version: 0x%04x\n", info->bt_version);
        printf("unknown bytes:       ");
        for (unsigned char ch: info->_unknown) {
            printf("%02x ", ch);
        }
        printf("\n\n");

        psmove_free_mem((char *)info);

        psmove_disconnect(move);
    }

    return result;
}

