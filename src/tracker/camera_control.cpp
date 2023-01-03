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

#include "psmove_config.h"
#include "camera_control.h"
#include "psmove_tracker.h"

#include "../psmove_private.h"

#include <stdio.h>
#include <stdint.h>

#include "camera_control_driver.h"

#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"


CameraControl *
camera_control_new_with_settings(int cameraID, int width, int height, int framerate)
{
    int camera_env = psmove_util_get_env_int(PSMOVE_TRACKER_CAMERA_ENV);
    if (camera_env != -1) {
        cameraID = camera_env;
        PSMOVE_DEBUG("Using camera %d (%s is set)", cameraID, PSMOVE_TRACKER_CAMERA_ENV);
    }

    if (cameraID == -1) {
        cameraID = camera_control_driver_get_preferred_camera();

        if (cameraID == -1) {
            /* Could not find the PSEye - fallback to first camera */
            PSMOVE_INFO("No preferred camera found, using first available camera");
            cameraID = 0;
        }
    }

    if (width == -1) {
        width = psmove_util_get_env_int(PSMOVE_TRACKER_WIDTH_ENV);
    }

    if (height == -1) {
        height = psmove_util_get_env_int(PSMOVE_TRACKER_HEIGHT_ENV);
    }

    if (framerate == -1) {
        framerate = 60;
    }

    CameraControl *cc = nullptr;

    const char *video = getenv(PSMOVE_TRACKER_FILENAME_ENV);
    if (video) {
        cc = new CameraControlVideoFile(video, width, height, framerate);
    } else {
        cc = camera_control_driver_new(cameraID, width, height, framerate);
    }

    return cc;
}

void
camera_control_set_deinterlace(CameraControl *cc, bool enabled)
{
    cc->deinterlace = enabled;
}

void
camera_control_read_calibration(CameraControl* cc, const char *filename)
{
    if (filename == nullptr) {
        filename = getenv(PSMOVE_TRACKER_CAMERA_CALIBRATION_ENV);
    }

    if (filename == nullptr) {
        PSMOVE_INFO("No camera calibration");
        return;
    }

    cv::Mat intrinsic_matrix(3, 3, CV_32FC1);
    cv::Mat distortion_coeffs(5, 1, CV_32FC1);

    PSMOVE_INFO("Reading camera calibration from %s", filename);
    cv::FileStorage in(filename, cv::FileStorage::READ);
    in["intrinsic_matrix"] >> intrinsic_matrix;
    in["distortion_coeffs"] >> distortion_coeffs;
    in.release();

    CvSize size = cvSize(cc->layout.crop_width, cc->layout.crop_height);

    // Build the undistort map that we will use for all subsequent frames
    cc->mapx.create(size, IPL_DEPTH_32F);
    cc->mapy.create(size, IPL_DEPTH_32F);

    cv::Mat R;
    cv::initUndistortRectifyMap(intrinsic_matrix, distortion_coeffs, R, intrinsic_matrix, size, CV_32FC1, cc->mapx, cc->mapy);
    cc->undistort = true;

    if (!cc->frame3chUndistort) {
        cc->frame3chUndistort = cvCreateImage(size, 8, 3);
    }
}

IplImage *
camera_control_query_frame(CameraControl *cc)
{
    IplImage *result = cc->query_frame();

    if (cc->deinterlace) {
        /**
         * Dirty hack follows:
         *  - Clone image
         *  - Hack internal variables to make an image of all odd lines
         **/
        IplImage *tmp = cvCloneImage(result);
        tmp->imageData += tmp->widthStep; // odd lines
        tmp->widthStep *= 2;
        tmp->height /= 2;

        /**
         * Use nearest-neighbor to be faster. In my tests, this does not
         * cause a speed disadvantage, and tracking quality is still good.
         *
         * This will scale the half-height image "tmp" to the original frame
         * size by doubling lines (so we can still do normal circle tracking).
         **/
        cvResize(tmp, result, CV_INTER_NN);

        /**
         * Need to revert changes in tmp from above, otherwise the call
         * to cvReleaseImage would cause a crash.
         **/
        tmp->height = result->height;
        tmp->widthStep = result->widthStep;
        tmp->imageData -= tmp->widthStep; // odd lines
        cvReleaseImage(&tmp);
    }

    // undistort image
    if (cc->undistort) {
        cv::remap(cv::cvarrToMat(result), cv::cvarrToMat(cc->frame3chUndistort), cc->mapx, cc->mapy, cv::INTER_LINEAR);
        result = cc->frame3chUndistort;
    }

#if defined(CAMERA_CONTROL_DEBUG_CAPTURED_IMAGE)
    cvShowImage("camera input", result);
    cvWaitKey(1);
#endif

    return result;
}

struct PSMoveCameraInfo
camera_control_get_camera_info(CameraControl *cc)
{
    return cc->get_camera_info();
}

void
camera_control_set_parameters(CameraControl *cc, float exposure, bool mirror)
{
    cc->set_parameters(exposure, mirror);
}

struct CameraControlSystemSettings *
camera_control_backup_system_settings(CameraControl *cc)
{
    return cc->backup_system_settings();
}

void
camera_control_restore_system_settings(CameraControl* cc,
        struct CameraControlSystemSettings *settings)
{
    cc->restore_system_settings(settings);
}

void
camera_control_delete(CameraControl* cc)
{
    delete cc;
}

int
camera_control_count_connected()
{
    return camera_control_driver_count_connected();
}
