#pragma once
/**
 * PS Move API - An interface for the PS Move Motion Controller
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

#include "opencv2/core/core_c.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "psmove.h"
#include "psmove_tracker.h"
#include "../psmove_private.h"

struct CameraControl;
typedef struct CameraControl CameraControl;

CameraControl *
camera_control_new_with_settings(int cameraID, int width, int height, int framerate);

int
camera_control_count_connected();

void
camera_control_read_calibration(CameraControl* cc,
        char* intrinsicsFile, char* distortionFile);

void
camera_control_set_deinterlace(CameraControl *cc,
        bool enabled);

IplImage *
camera_control_query_frame(CameraControl* cc);

void
camera_control_delete(CameraControl* cc);

/**
 * Set the camera parameters used during capturing
 *
 * cc         - the camera control to modify
 * exposure   - exposure (0.0 .. 1.0)
 * mirror     - whether to mirror the image horizontally
 **/
void
camera_control_set_parameters(CameraControl* cc, float exposure, bool mirror);

/* Opaque structure for storing system settings */

struct CameraControlSystemSettings;

struct CameraControlSystemSettings *
camera_control_backup_system_settings(CameraControl* cc);

void
camera_control_restore_system_settings(CameraControl* cc,
        struct CameraControlSystemSettings *settings);

struct PSMoveCameraInfo
camera_control_get_camera_info(CameraControl *cc);

#ifdef __cplusplus
}
#endif
