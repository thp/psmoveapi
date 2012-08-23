
#if defined(SWIGJAVA)

/* Java */
%include "enums.swg"
%javaconst(1);
%rename PSMove_Button Button;
%rename PSMove_Battery_Level BatteryLevel;
%rename PSMove_Connection_Type ConnectionType;
%rename PSMove_Frame Frame;
%module psmoveapi

#else

/* Python et al. */
%module psmove

#endif /* defined(SWIGJAVA) */

%pragma(java) jniclasscode=%{
    static {
        try {
            System.loadLibrary("psmove_java");
        } catch (UnsatisfiedLinkError e) {
            System.err.println("Failed to load native 'psmove' library: " + e);
            System.exit(1);
        }
    }
%}

%{

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


#include "psmove.h"

%}

%include "psmove.h"

typedef struct {} PSMove;

int count_connected();

void reinit();

%extend PSMove {
    /* Connection type */
    const int connection_type;

    /* Accelerometer */
    const int ax;
    const int ay;
    const int az;

    /* Gyroscope */
    const int gx;
    const int gy;
    const int gz;

    /* Magnetometer */
    const int mx;
    const int my;
    const int mz;

    PSMove() {
        return psmove_connect_by_id(0);
    }

    PSMove(int id) {
        return psmove_connect_by_id(id);
    }

    void get_accelerometer_frame(enum PSMove_Frame frame,
        float *OUTPUT, float *OUTPUT, float *OUTPUT);

    void get_gyroscope_frame(enum PSMove_Frame frame,
        float *OUTPUT, float *OUTPUT, float *OUTPUT);

    void set_leds(int r, int g, int b) {
        psmove_set_leds($self, r, g, b);
    }

    void set_rumble(int rumble) {
        psmove_set_rumble($self, rumble);
    }

    int update_leds() {
        return psmove_update_leds($self);
    }

    void set_rate_limiting(int enabled) {
        psmove_set_rate_limiting($self, enabled);
    }

    int pair() {
        return psmove_pair($self);
    }

    int pair_custom(const char *btaddr) {
        return psmove_pair_custom($self, btaddr);
    }

    const char *get_serial() {
        static char *result = NULL;
        if (result != NULL) {
            free(result);
        }
        result = psmove_get_serial($self);
        return result;
    }

    int is_remote() {
        return psmove_is_remote($self);
    }

    int poll() {
        return psmove_poll($self);
    }

    int get_buttons() {
        return psmove_get_buttons($self);
    }

    int get_battery() {
        return psmove_get_battery($self);
    }

    int get_temperature() {
        return psmove_get_temperature($self);
    }

    int get_trigger() {
        return psmove_get_trigger($self);
    }

    ~PSMove() {
        psmove_disconnect($self);
    }

}


%{

void
PSMove_get_accelerometer_frame(PSMove *move, enum PSMove_Frame frame,
    float *x, float *y, float *z) {
    psmove_get_accelerometer_frame(move, frame, x, y, z);
}

void
PSMove_get_gyroscope_frame(PSMove *move, enum PSMove_Frame frame,
    float *x, float *y, float *z) {
    psmove_get_gyroscope_frame(move, frame, x, y, z);
}

int
PSMove_connection_type_get(PSMove *move) {
    return psmove_connection_type(move);
}

int
PSMove_ax_get(PSMove *move) {
    int result;
    psmove_get_accelerometer(move, &result, NULL, NULL);
    return result;
}

int
PSMove_ay_get(PSMove *move) {
    int result;
    psmove_get_accelerometer(move, NULL, &result, NULL);
    return result;
}

int
PSMove_az_get(PSMove *move) {
    int result;
    psmove_get_accelerometer(move, NULL, NULL, &result);
    return result;
}

int
PSMove_gx_get(PSMove *move) {
    int result;
    psmove_get_gyroscope(move, &result, NULL, NULL);
    return result;
}

int
PSMove_gy_get(PSMove *move) {
    int result;
    psmove_get_gyroscope(move, NULL, &result, NULL);
    return result;
}

int
PSMove_gz_get(PSMove *move) {
    int result;
    psmove_get_gyroscope(move, NULL, NULL, &result);
    return result;
}


int
PSMove_mx_get(PSMove *move) {
    int result;
    psmove_get_magnetometer(move, &result, NULL, NULL);
    return result;
}

int
PSMove_my_get(PSMove *move) {
    int result;
    psmove_get_magnetometer(move, NULL, &result, NULL);
    return result;
}

int
PSMove_mz_get(PSMove *move) {
    int result;
    psmove_get_magnetometer(move, NULL, NULL, &result);
    return result;
}

int count_connected()
{
    return psmove_count_connected();
}

void reinit()
{
    return psmove_reinit();
}

%}


