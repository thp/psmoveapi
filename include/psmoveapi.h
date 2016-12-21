#pragma once

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


#ifdef __cplusplus
extern "C" {
#endif

#include "psmove.h"

struct Vec3 {
    float x;
    float y;
    float z;
};

struct RGB {
    float r;
    float g;
    float b;
};

struct Controller {
    int index;
    const char *serial;
    int bluetooth;
    int usb;

    struct RGB color;
    float rumble;

    int buttons;
    int pressed;
    int released;
    float trigger;
    struct Vec3 accelerometer;
    struct Vec3 gyroscope;
    struct Vec3 magnetometer;
    int battery;

    // User data pointer that can be set and used by the application
    // (not related to user_data pointer in EventReceiver)
    void *user_data;
};

struct EventReceiver {
    void (*connect)(struct Controller *controller, void *user_data);
    void (*update)(struct Controller *controller, void *user_data);
    void (*disconnect)(struct Controller *controller, void *user_data);
};

ADDAPI void
ADDCALL psmoveapi_init(EventReceiver *receiver, void *user_data);

ADDAPI void
ADDCALL psmoveapi_update();

ADDAPI void
ADDCALL psmoveapi_quit();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

namespace psmoveapi {

class Handler {
public:
    virtual ~Handler() {}

    virtual void connect(Controller *controller) {}
    virtual void update(Controller *controller) {}
    virtual void disconnect(Controller *controller) {}
};

namespace {

void
_handler_connect(struct Controller *controller, void *user_data)
{
    static_cast<Handler *>(user_data)->connect(controller);
}

void
_handler_update(struct Controller *controller, void *user_data)
{
    static_cast<Handler *>(user_data)->update(controller);
}

void
_handler_disconnect(struct Controller *controller, void *user_data)
{
    static_cast<Handler *>(user_data)->disconnect(controller);
}

EventReceiver
_handler_receiver = {
    _handler_connect,
    _handler_update,
    _handler_disconnect,
};

}; // end anonymous namespace

class PSMoveAPI {
public:
    PSMoveAPI(Handler *handler) { psmoveapi_init(&_handler_receiver, handler); }
    ~PSMoveAPI() { psmoveapi_quit(); }

    void update() { psmoveapi_update(); }
};

}; // namespace psmoveapi

#endif
