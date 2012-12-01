
#if defined(SWIGJAVA)

/* Java */
%include "enums.swg"
%include "typemaps.i"
%include "various.i"

%javaconst(1);
%rename PSMove_Button Button;
%rename PSMove_Battery_Level BatteryLevel;
%rename PSMove_Connection_Type ConnectionType;
%rename PSMove_Frame Frame;
%rename PSMoveTracker_Status Status;
%rename PSMoveTracker_Exposure Exposure;
%rename PSMove_RemoteConfig RemoteConfig;
%module psmoveapi

#elif defined (SWIGCSHARP)

/* C# */
%include "typemaps.i"

%rename PSMove_Button Button;
%rename PSMove_Battery_Level BatteryLevel;
%rename PSMove_Connection_Type ConnectionType;
%rename PSMove_Frame Frame;
%rename PSMoveTracker_Status Status;
%rename PSMoveTracker_Exposure Exposure;
%rename PSMove_RemoteConfig RemoteConfig;
%module psmoveapi_csharp

#else

/* Python et al. */
%module psmove

%include "cdata.i"

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
 * Copyright (c) 2011, 2012 Thomas Perl <m@thp.io>
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
#include "psmove_tracker.h"

%}

%include "psmove_config.h"

%include "psmove.h"
typedef struct {} PSMove;

#ifdef PSMOVE_BUILD_TRACKER
%include "psmove_tracker.h"
typedef struct {} PSMoveTracker;
#endif /* PSMOVE_BUILD_TRACKER */


void set_remote_config(enum PSMove_RemoteConfig config);

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

    void get_magnetometer_vector(float *OUTPUT, float *OUTPUT, float *OUTPUT);

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

    void get_button_events(unsigned int *OUTPUT, unsigned int *OUTPUT);

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


#ifdef PSMOVE_BUILD_TRACKER

%extend PSMoveTrackerRGBImage {
    const int size;

#ifdef SWIGJAVA
    /* Get bytes into a byte[] array in Java (must be big enough!) */
    void get_bytes(char *BYTE) {
        memcpy(BYTE, $self->data, 3 * $self->width * $self->height);
    }
#endif
}

%extend PSMoveTracker {
    /* Dimming factor */
    float dimming;

    /* Exposure mode */
    enum PSMoveTracker_Exposure exposure;

    PSMoveTracker() {
        return psmove_tracker_new();
    }

    PSMoveTracker(int camera) {
        return psmove_tracker_new_with_camera(camera);
    }

    enum PSMoveTracker_Status enable(PSMove *move) {
        return psmove_tracker_enable($self, move);
    }

    enum PSMoveTracker_Status enable_with_color(PSMove *move,
            int r, int g, int b) {
        return psmove_tracker_enable_with_color($self, move, r, g, b);
    }

    void
    disable(PSMove *move) {
        psmove_tracker_disable($self, move);
    }

    void set_auto_update_leds(PSMove *move, int auto_update_leds) {
        psmove_tracker_set_auto_update_leds($self, move, auto_update_leds);
    }

    int get_auto_update_leds(PSMove *move) {
        return psmove_tracker_get_auto_update_leds($self, move);
    }

    void get_color(PSMove *move, unsigned char *OUTPUT,
        unsigned char *OUTPUT, unsigned char *OUTPUT);

    void get_camera_color(PSMove *move, unsigned char *OUTPUT,
            unsigned char *OUTPUT, unsigned char *OUTPUT);

    int set_camera_color(PSMove *move, unsigned char r,
            unsigned char g, unsigned char b) {
        return psmove_tracker_set_camera_color($self, move, r, g, b);
    }

    void enable_deinterlace(int enabled) {
        psmove_tracker_enable_deinterlace($self,
            enabled?PSMove_True:PSMove_False);
    }

    void set_mirror(int enabled) {
        psmove_tracker_set_mirror($self,
            enabled?PSMove_True:PSMove_False);
    }

    int get_mirror() {
        return psmove_tracker_get_mirror($self);
    }

    enum PSMoveTracker_Status get_status(PSMove *move) {
        return psmove_tracker_get_status($self, move);
    }

    void update_image() {
        return psmove_tracker_update_image($self);
    }

    int update() {
        return psmove_tracker_update($self, NULL);
    }

    int update(PSMove *move) {
        return psmove_tracker_update($self, move);
    }

    PSMoveTrackerRGBImage get_image() {
        return psmove_tracker_get_image($self);
    }

    void get_position(PSMove *move, float *OUTPUT,
            float *OUTPUT, float *OUTPUT);

    void get_size(int *OUTPUT, int *OUTPUT);

    float distance_from_radius(float radius) {
        return psmove_tracker_distance_from_radius($self, radius);
    }

    void set_distance_parameters(float height, float center,
            float hwhm, float shape)
    {
        psmove_tracker_set_distance_parameters($self, height,
            center, hwhm, shape);
    }

    ~PSMoveTracker() {
        psmove_tracker_free($self);
    }

}

#endif /* PSMOVE_BUILD_TRACKER */


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

void
PSMove_get_magnetometer_vector(PSMove *move, float *x, float *y, float *z)
{
    psmove_get_magnetometer_vector(move, x, y, z);
}

void
PSMove_get_button_events(PSMove *move,
    unsigned int *pressed, unsigned int *released)
{
    psmove_get_button_events(move, pressed, released);
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

void set_remote_config(enum PSMove_RemoteConfig config)
{
    return psmove_set_remote_config(config);
}

int count_connected()
{
    return psmove_count_connected();
}

void reinit()
{
    return psmove_reinit();
}

#ifdef PSMOVE_BUILD_TRACKER

int
PSMoveTrackerRGBImage_size_get(PSMoveTrackerRGBImage *image)
{
    return 3 * image->width * image->height;
}

float
PSMoveTracker_dimming_get(PSMoveTracker *tracker)
{
    return psmove_tracker_get_dimming(tracker);
}

void
PSMoveTracker_dimming_set(PSMoveTracker *tracker, float dimming)
{
    psmove_tracker_set_dimming(tracker, dimming);
}

void
PSMoveTracker_exposure_set(PSMoveTracker *tracker,
    enum PSMoveTracker_Exposure exposure)
{
    psmove_tracker_set_exposure(tracker, exposure);
}

enum PSMoveTracker_Exposure
PSMoveTracker_exposure_get(PSMoveTracker *tracker)
{
    return psmove_tracker_get_exposure(tracker);
}

void
PSMoveTracker_get_color(PSMoveTracker *tracker, PSMove *move,
    unsigned char *r, unsigned char *g, unsigned char *b)
{
    psmove_tracker_get_color(tracker, move, r, g, b);
}

void
PSMoveTracker_get_camera_color(PSMoveTracker *tracker, PSMove *move,
    unsigned char *r, unsigned char *g, unsigned char *b)
{
    psmove_tracker_get_camera_color(tracker, move, r, g, b);
}

void
PSMoveTracker_get_position(PSMoveTracker *tracker, PSMove *move,
    float *x, float *y, float *radius)
{
    psmove_tracker_get_position(tracker, move, x, y, radius);
}

void
PSMoveTracker_get_size(PSMoveTracker *tracker, int *width, int *height)
{
    return psmove_tracker_get_size(tracker, width, height);
}


#endif /* PSMOVE_BUILD_TRACKER */

%}


