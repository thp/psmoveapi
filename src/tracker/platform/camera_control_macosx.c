/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
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

#include "../camera_control.h"
#include "../camera_control_private.h"
#include "psmove_osxsupport.h"

void
camera_control_initialize()
{
    macosx_camera_set_exposure_lock(0);
}

void
camera_control_set_parameters(CameraControl* cc,
        int autoE, int autoG, int autoWB,
        int exposure, int gain,
        int wbRed, int wbGreen, int wbBlue,
        int contrast, int brightness)
{
#if defined(CAMERA_CONTROL_USE_PS3EYE_DRIVER)
    ps3eye_set_parameter(cc->eye, PS3EYE_EXPOSURE, exposure);

    /** Many of the values passed to this function do not work with PS3EYEDriver.
     * comment out these functions until a sane value scheme is implemented.
     * ps3eye_set_parameter(cc->eye, PS3EYE_AUTO_GAIN, autoG);
     * ps3eye_set_parameter(cc->eye, PS3EYE_GAIN, gain);
     * ps3eye_set_parameter(cc->eye, PS3EYE_AUTO_WHITEBALANCE, autoWB);
     * ps3eye_set_parameter(cc->eye, PS3EYE_CONTRAST, contrast);
     * ps3eye_set_parameter(cc->eye, PS3EYE_BRIGHTNESS, brightness);
     * ps3eye_set_parameter(cc->eye, PS3EYE_REDBALANCE, wbRed);
     * ps3eye_set_parameter(cc->eye, PS3EYE_BLUEBALANCE, wbGreen);
     **/
    psmove_WARNING("Only PS3EYEDriver parameter 'exposure' set by ps3eye_set_parameter.\n");
    /** Some PS3EYEDriver parameters that could be set are not set by this function
     * ps3eye_set_parameter(cc->eye, PS3EYE_SHARPNESS, autoG);
     * ps3eye_set_parameter(cc->eye, PS3EYE_HUE, ???);
     * ps3eye_set_parameter(cc->eye, PS3EYE_HFLIP, ??);
     * ps3eye_set_parameter(cc->eye, PS3EYE_VFLIP, ??);
     **/

#else
    macosx_camera_set_exposure_lock(1);
#endif
}

void
camera_control_backup_system_settings(CameraControl* cc,
        const char* file)
{
    // Do nothing
}

void camera_control_restore_system_settings(CameraControl* cc,
        const char* file)
{
    // Do nothing
}

