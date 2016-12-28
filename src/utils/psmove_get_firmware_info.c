#include <stdio.h>
#include <stdlib.h>

#include "psmove.h"
#include "../psmove_private.h"


int main(int argc, char* argv[])
{
    PSMove *move;
    size_t i;
    
    if (psmove_count_connected() == 0) {
        printf("No controllers connected.\n");
        return 1;
    }

    move = psmove_connect();
    if (move == NULL) {
        printf("Cannot connect to controller.\n");
        return 1;
    }

#ifdef __APPLE__
    if (psmove_connection_type(move) != Conn_USB) {
        printf("Controller must be connected via USB.\n");
        psmove_disconnect(move);
        return 1;
    }
#endif

    printf("Serial: %s\n", psmove_get_serial(move));

    PSMove_Firmware_Info* info = _psmove_get_firmware_info(move);
    if (info == NULL) {
        printf("Failed to retrieve firmware info.\n");
        psmove_disconnect(move);
        return 1;
    }

    printf("firmware version:    0x%04x, rev. %d\n", info->version, info->revision);
    printf("BT firmware version: 0x%04x\n", info->bt_version);
    printf("unknown bytes:       ");
    for (i = 0; i < sizeof(info->_unknown); ++i) {
        printf("%02x ", info->_unknown[i]);
    }
    printf("\n");

    free( info );

    psmove_disconnect(move);
    return 0;
}

