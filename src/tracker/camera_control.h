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
#include "../psmove_private.h"

struct _CameraControl;
typedef struct _CameraControl CameraControl;

int
camera_control_get_preferred_camera();

CameraControl *
camera_control_new_with_settings(int cameraID, int width, int height, int framerate);

int
camera_control_count_connected();

void
camera_control_read_calibration(CameraControl* cc,
        char* intrinsicsFile, char* distortionFile);

void
camera_control_set_deinterlace(CameraControl *cc,
        enum PSMove_Bool enabled);

IplImage *
camera_control_query_frame(CameraControl* cc);

void
camera_control_delete(CameraControl* cc);



/* Platform-specific functions (implement in camera_control_<os>.c) */


/**

// if a negative value is passed, that means it is not changed

*/

/**
 * Set the camera parameters used during capturing
 *
 * cc         - the camera control to modify
 * autoE      - value range [0-0xFFFF]
 * autoG      - value range [0-0xFFFF]
 * autoWB     - value range [0-0xFFFF]
 * exposure   - value range [0-0xFFFF]
 * gain       - value range [0-0xFFFF]
 * wbRed      - value range [0-0xFFFF]
 * wbGreen    - value range [0-0xFFFF]
 * wbBlue     - value range [0-0xFFFF]
 * contrast   - value range [0-0xFFFF]
 * brightness - value range [0-0xFFFF]
 **/
void
camera_control_set_parameters(CameraControl* cc,
        int autoE, int autoG, int autoWB,
        int exposure, int gain,
        int wbRed, int wbGreen, int wbBlue,
		int contrast, int brightness, enum PSMove_Bool h_flip);

struct CameraControlFrameLayout {
    int capture_width; /**< raw capture device width */
    int capture_height; /**< raw capture device height */

    int crop_x; /**< absolute frame top left X coordinate */
    int crop_y; /**< absolute frame top left Y coordinate */
    int crop_width; /**< cropped frame width */
    int crop_height; /**< cropped frame height */
};

/**
 * Get capture frame size and cropping information of camera
 *
 * \param cc Camera control object
 * \param width User-requested width, or -1 if unset
 * \param height User-requested height, or -1 if unset
 * \param layout Frame layout information [OUT]
 * \return true if the requested size is valid, false otherwise
 **/
bool
camera_control_get_frame_layout(CameraControl *cc, int width, int height, struct CameraControlFrameLayout *layout);

/* Opaque structure for storing system settings */

struct CameraControlSystemSettings;

struct CameraControlSystemSettings *
camera_control_backup_system_settings(CameraControl* cc);

void
camera_control_restore_system_settings(CameraControl* cc,
        struct CameraControlSystemSettings *settings);

#ifdef __cplusplus
}
#endif
