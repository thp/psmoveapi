#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psmove.h"
#include "../psmove_private.h"


void dump_buffer(unsigned char *buf, int size)
{
    int i;

    for (i = 0; i < size; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

void send_and_receive(PSMove *move, PSMove_Data_AuthChallenge *challenge)
{
    printf("Sending challenge: ");
    dump_buffer(*challenge, sizeof(*challenge));

    if (_psmove_set_auth_challenge(move, challenge)) {
        PSMove_Data_AuthResponse *response = _psmove_get_auth_response(move);

        if (response != NULL) {
            printf("Received response: ");
            dump_buffer(*response, sizeof(*response));
            free(response);
        } else {
            printf("Failed to receive response.\n");
        }
    } else {
        printf("Failed to send challenge.\n");
    }
}

int main(int argc, char* argv[])
{
    PSMove *move;
    int i;

    if (psmove_count_connected() == 0) {
        printf("No controllers connected.\n");
        return 1;
    }

    move = psmove_connect();
    if (move == NULL) {
        printf("Cannot connect to controller.\n");
        return 2;
    }

    printf("Connection established.\n");
    printf("Serial: %s\n", psmove_get_serial(move));

    if (psmove_connection_type(move) != Conn_Bluetooth) {
        printf("Controller must be connected via Bluetooth.\n");
        return 3;
    }

    PSMove_Data_AuthChallenge challenge = {
        0x02, 0x00, 0x1D, 0x91, 0xE5, 0x81, 0x30, 0x6A,
        0x22, 0x9A, 0xAB, 0x2E, 0x80, 0xB4, 0xED, 0x2E,
        0xDE, 0x40, 0x0A, 0xF0, 0x02, 0xB0, 0x42, 0x8B,
        0x01, 0x41, 0xB2, 0xA4, 0x3D, 0xE7, 0xD4, 0xBF,
        0x05, 0x92
    };

    challenge[0] = 0;
    send_and_receive(move, &challenge);

    challenge[0] = 1;
    send_and_receive(move, &challenge);

    for (i = 1; i < 0x10; i++) {
        challenge[0] = 0;
		challenge[1] = (unsigned char)i;
        send_and_receive(move, &challenge);
    }

    memset(challenge, 0, sizeof(challenge));
    send_and_receive(move, &challenge);

    psmove_disconnect(move);
    return 0;
}

