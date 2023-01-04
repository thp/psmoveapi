/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012, 2023 Thomas Perl <m@thp.io>
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

#include "camera_control.h"
#include "camera_control_driver.h"

#include "ps3eye_capi.h"

struct CameraControlPS3EYEDriver : public CameraControl {
    CameraControlPS3EYEDriver(int camera_id, int width, int height, int framerate);
    virtual ~CameraControlPS3EYEDriver();

    virtual IplImage *query_frame() override;
    virtual void set_parameters(float exposure, bool mirror) override;
    virtual PSMoveCameraInfo get_camera_info() override;

    ps3eye_t *eye { nullptr };
    IplImage *framebgr { nullptr };
};

CameraControlPS3EYEDriver::CameraControlPS3EYEDriver(int camera_id, int width, int height, int framerate)
    : CameraControl(camera_id, width, height, framerate)
{
    layout = get_frame_layout(width, height);

    eye = ps3eye_open(camera_id, layout.capture_width, layout.capture_height, framerate, PS3EYE_FORMAT_BGR);

    if (eye == nullptr) {
        PSMOVE_FATAL("Could not open PS3Eye");
    }

    framebgr = cvCreateImage(cvSize(layout.capture_width, layout.capture_height), IPL_DEPTH_8U, 3);
}

CameraControlPS3EYEDriver::~CameraControlPS3EYEDriver()
{
    cvReleaseImage(&framebgr);

    ps3eye_close(eye);
    ps3eye_uninit();
}

IplImage *
CameraControlPS3EYEDriver::query_frame()
{
    unsigned char *cvpixels;
    cvGetRawData(framebgr, &cvpixels, 0, 0);

    ps3eye_grab_frame(eye, cvpixels);

    return framebgr;
}

void
CameraControlPS3EYEDriver::set_parameters(float exposure, bool mirror)
{
    ps3eye_set_parameter(eye, PS3EYE_AUTO_GAIN, 0);
    ps3eye_set_parameter(eye, PS3EYE_AUTO_WHITEBALANCE, 0);
    ps3eye_set_parameter(eye, PS3EYE_EXPOSURE, int(0x1FF * std::min(1.f, std::max(0.f, exposure))));
    ps3eye_set_parameter(eye, PS3EYE_GAIN, 0);
    ps3eye_set_parameter(eye, PS3EYE_HFLIP, mirror);
}

PSMoveCameraInfo
CameraControlPS3EYEDriver::get_camera_info()
{
    return PSMoveCameraInfo {
        "PS3 Eye",
        "PS3EYEDriver",
        layout.crop_width,
        layout.crop_height,
    };
}

CameraControl *
camera_control_driver_new(int camera_id, int width, int height, int framerate)
{
    ps3eye_init();

    if (camera_control_driver_count_connected() <= camera_id) {
        PSMOVE_WARNING("Invalid camera id: %d (%d connected)", camera_id, camera_control_driver_count_connected());
        return nullptr;
    }

    return new CameraControlPS3EYEDriver(camera_id, width, height, framerate);
}

int
camera_control_driver_get_preferred_camera()
{
    return 0;
}

int
camera_control_driver_count_connected()
{
    ps3eye_init();

    return ps3eye_count_connected();
}
