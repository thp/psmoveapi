
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2016 Thomas Perl <m@thp.io>
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



#include <stdio.h>
#include <stdlib.h>

#include "psmoveapi.h"


struct ExampleHandler : public psmoveapi::Handler {
    ExampleHandler() : color{0.f, 0.f, 0.f} , rumble(0.f), interactive(false) {}

    virtual void connect(Controller *controller) {
        printf("Connect: %s\n", controller->serial);
    }

    virtual void update(Controller *controller) {
        if (!interactive) {
            controller->color = color;
            controller->rumble = rumble;
        } else {
            if ((controller->buttons & Btn_TRIANGLE) != 0) {
                printf("Triangle pressed, with trigger value: %.2f\n", controller->trigger);
                controller->rumble = controller->trigger;
            } else {
                controller->rumble = 0.f;
            }

            controller->color = { 0.f, 0.f, controller->trigger };

            printf("Accel: %.2f, %.2f, %.2f\n", controller->accelerometer.x, controller->accelerometer.y, controller->accelerometer.z);
            printf("Gyro: %.2f, %.2f, %.2f\n", controller->gyroscope.x, controller->gyroscope.y, controller->gyroscope.z);
            printf("Magnetometer: %.2f, %.2f, %.2f\n", controller->magnetometer.x, controller->magnetometer.y, controller->magnetometer.z);
            printf("Connection [%c] USB [%c] Bluetooth; Buttons: 0x%04x\n", controller->usb ? 'x' : ' ', controller->bluetooth ? 'x' : ' ', controller->buttons);
        }
    }

    virtual void disconnect(Controller *controller) {
        printf("Disconnect: %s\n", controller->serial);
    }

    RGB color;
    float rumble;
    bool interactive;
};

int
main(int argc, char* argv[])
{
    ExampleHandler handler;
    psmoveapi::PSMoveAPI api(&handler);

    for (int i=0; i<10; i++) {
        handler.color.g = float(i % 3 == 0);
        handler.rumble = float(i % 2);
        api.update();
        psmove_util_sleep_ms(10*(i%10));
    }

    for (int i=250; i>=0; i-=5) {
        handler.color = { float(i/255.f), float(i/255.f), 0.f };
        handler.rumble = 0.f;
        api.update();
    }

    handler.color = { 0.f, 0.f, 0.f };
    handler.rumble = 0.f;
    api.update();

    handler.interactive = true;
    while (true) {
        api.update();
    }

    return 0;
}
