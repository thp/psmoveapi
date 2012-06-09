/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2011 Thomas Perl <m@thp.io>
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "psmove.h"
#include <windows.h>
#include <time.h>

int exitCondition = 0;
int color = 0;
int newColor = 0xFF0000;

long micros() {
	FILETIME ft;
	memset(&ft, 0, sizeof(ft));
	GetSystemTimeAsFileTime(&ft);
	return (ft.dwLowDateTime | ft.dwHighDateTime << 32) / 10;
}

struct threadParams {
	int param1;
	int param2;
};

static DWORD WINAPI colorInputThread(void* threadParams) {
	struct threadParams* params = (struct threadParams*) threadParams;

	while (!exitCondition) {
		char colorString[256];
		printf("Enter the new color as 6 HEX Digits and press [ENTER].\n");
		gets(colorString);
		sscanf(colorString,"%x",&newColor);
	}

	return 0;
}

int main(int argc, char* argv[]) {
	PSMove *move;
	enum PSMove_Connection_Type ctype;
	int i;

	i = psmove_count_connected();
	printf("Connected controllers: %d\n", i);

	move = psmove_connect();

	if (move == NULL) {
		printf("Could not connect to default Move controller.\n"
				"Please connect one via USB or Bluetooth.\n");
		exit(1);
	}

	ctype = psmove_connection_type(move);
	switch (ctype) {
	case Conn_USB:
		printf("Connected via USB.\n");
		break;
	case Conn_Bluetooth:
		printf("Connected via Bluetooth.\n");
		break;
	case Conn_Unknown:
		printf("Unknown connection type.\n");
		break;
	}

	if (ctype == Conn_USB) {
		PSMove_Data_BTAddr addr;
		psmove_get_btaddr(move, &addr);
		printf("Current BT Host: ");
		for (i = 0; i < 6; i++) {
			printf("%02x ", addr[i]);
		}
		printf("\n");
	}

	// start the thread for entering colors
	DWORD threadDescriptor;
	struct threadParams params1 = { 1, 2 };
	CreateThread(NULL, 0, colorInputThread, (void*) &params1, 0,
			&threadDescriptor);

	// proceed with the loop that actually applys the colors
	int SWITCH = 0;
	int r, g, b;
	while (!exitCondition) {
		psmove_update_leds(move);
		if (newColor != color) {
			color = newColor;
			r = (0xFF0000 & color) >> 16;
			g = (0xFF00 & color) >> 8;
			b = (0xFF & color) >> 0;
			printf("setting color to: %s(%d,%d,%d)\n", argv[1], r, g, b);
		}
		// go and update & sleep forever
		SWITCH = !SWITCH;
		if(SWITCH)
			psmove_set_leds(move, r, g, b);
		else
			psmove_set_leds(move, 0,0,0);
			
		psmove_update_leds(move);
		//usleep(1000000 - 1);
		usleep(1000000 - 1);
	}
	exitCondition = 1;
	psmove_disconnect(move);
	return 0;
}

