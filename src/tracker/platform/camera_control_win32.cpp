/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * Copyright (c) 2012 Benjamin Venditti <benjamin.venditti@gmail.com>
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

#include "psmove_tracker.h"

#include "../camera_control.h"
#include "../camera_control_private.h"

struct CameraControlSystemSettings *
camera_control_backup_system_settings(CameraControl *cc)
{
    return NULL;
}

void
camera_control_restore_system_settings(CameraControl *cc,
        struct CameraControlSystemSettings *settings)
{
}

void
camera_control_set_parameters(CameraControl *cc, float exposure, bool mirror)
{
    camera_control_ps3eyedriver_set_parameters(cc, exposure, mirror);
}

bool
camera_control_get_frame_layout(CameraControl *cc, int width, int height, struct CameraControlFrameLayout *layout)
{
    return camera_control_fallback_frame_layout(cc, width, height, layout);
}

int
camera_control_get_preferred_camera()
{
    return -1;
}

struct PSMoveCameraInfo
camera_control_get_camera_info(CameraControl *cc)
{
    return camera_control_fallback_get_camera_info(cc);
}
