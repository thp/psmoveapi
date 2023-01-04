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

#include "camera_control_driver.h"


/* CameraControl */

CameraControl::CameraControl(int camera_id, int width, int height, int framerate)
    : cameraID(camera_id)
{
}

CameraControl::~CameraControl()
{
    cvReleaseImage(&frame3chUndistort);
    cvReleaseImage(&mapx);
    cvReleaseImage(&mapy);
}

CameraControlFrameLayout
CameraControl::get_frame_layout(int width, int height)
{
    if (width == -1) {
        width = 640;
    }

    if (height == -1) {
        height = 480;
    }

    return CameraControlFrameLayout {
        width,
        height,
        0,
        0,
        width,
        height,
    };
}


/* CameraControlOpenCV */

CameraControlOpenCV::CameraControlOpenCV(int camera_id, int width, int height, int framerate)
    : CameraControl(camera_id, width, height, framerate)
{
}

CameraControlOpenCV::~CameraControlOpenCV()
{
    cvReleaseImage(&frame);

    delete capture;
}

IplImage *
CameraControlOpenCV::query_frame()
{
    IplImage *result = nullptr;

    cv::Mat frame;
    if (capture->read(frame)) {
        cvReleaseImage(&(this->frame));

        IplImage tmp = cvIplImage(frame);

        cvSetImageROI(&tmp, cvRect(layout.crop_x, layout.crop_y, layout.crop_width, layout.crop_height));
        result = cvCreateImage(cvSize(layout.crop_width, layout.crop_height), IPL_DEPTH_8U, 3);

        cvCopy(&tmp, result);
        this->frame = result;
    }

    return result;
}


/* CameraControlVideoFile */

CameraControlVideoFile::CameraControlVideoFile(const char *filename, int width, int height, int framerate)
    : CameraControlOpenCV(0, width, height, framerate)
    , filename(filename)
{
    PSMOVE_INFO("Using '%s' as video input.", filename);
    capture = new cv::VideoCapture(filename);
}

CameraControlVideoFile::~CameraControlVideoFile()
{
}

PSMoveCameraInfo
CameraControlVideoFile::get_camera_info()
{
    return PSMoveCameraInfo {
        filename.c_str(),
        "File Playback",
        layout.crop_width,
        layout.crop_height,
    };
}
