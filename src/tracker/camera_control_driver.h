#pragma once

/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012, 2023 Thomas Perl <m@thp.io>
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

#include "psmove.h"

#include "opencv2/core/core_c.h"
#include <opencv2/videoio.hpp>

#include "camera_control.h"

#include <string>

struct CameraControlFrameLayout {
    int capture_width; /**< raw capture device width */
    int capture_height; /**< raw capture device height */

    int crop_x; /**< absolute frame top left X coordinate */
    int crop_y; /**< absolute frame top left Y coordinate */
    int crop_width; /**< cropped frame width */
    int crop_height; /**< cropped frame height */
};

struct CameraControl {
    CameraControl(int camera_id, int width, int height, int framerate);
    virtual ~CameraControl();

    virtual CameraControlFrameLayout get_frame_layout(int width, int height);
    virtual CameraControlSystemSettings *backup_system_settings() { return nullptr; }
    virtual void restore_system_settings(CameraControlSystemSettings *settings) { }

    virtual IplImage *query_frame() = 0;
    virtual void set_parameters(float exposure, bool mirror) = 0;
    virtual PSMoveCameraInfo get_camera_info() = 0;

    int cameraID;
    CameraControlFrameLayout layout;

    IplImage *frame { nullptr };

    IplImage *frame3chUndistort { nullptr };
    bool undistort { false };
    cv::Mat mapx;
    cv::Mat mapy;

    bool deinterlace { false };
};

struct CameraControlOpenCV : public CameraControl {
    CameraControlOpenCV(int camera_id, int width, int height, int framerate);
    virtual ~CameraControlOpenCV();

    virtual IplImage *query_frame() override;

    cv::VideoCapture *capture { nullptr };
};

struct CameraControlVideoFile : public CameraControlOpenCV {
    CameraControlVideoFile(const char *filename, int width, int height, int framerate);
    virtual ~CameraControlVideoFile();

    // No-op for video file playback
    virtual void set_parameters(float exposure, bool mirror) override {}

    virtual PSMoveCameraInfo get_camera_info() override;

    std::string filename;
};


#ifdef __cplusplus
extern "C" {
#endif

ADDAPI CameraControl *
ADDCALL camera_control_driver_new(int camera_id, int width, int height, int framerate);

ADDAPI int
ADDCALL camera_control_driver_get_preferred_camera();

ADDAPI int
ADDCALL camera_control_driver_count_connected();

#ifdef __cplusplus
}
#endif
