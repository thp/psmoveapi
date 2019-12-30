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

#include "../camera_control.h"
#include "../camera_control_private.h"

#include <linux/videodev2.h>
#include <libv4l2.h>
#include <fcntl.h>


int open_v4l2_device(int id)
{
    char device_file[512];
    snprintf(device_file, sizeof(device_file), "/dev/video%d", id);
    return v4l2_open(device_file, O_RDWR, 0);
}

struct CameraControlSystemSettings {
    int AutoAEC;
    int AutoAGC;
    int Gain;
    int Exposure;
    int Contrast;
    int Brightness;
};

struct CameraControlSystemSettings *
camera_control_backup_system_settings(CameraControl *cc)
{
    struct CameraControlSystemSettings *settings = NULL;

    int fd = open_v4l2_device(cc->cameraID);
    if (fd != -1) {
        struct CameraControlSystemSettings *settings = calloc(1, sizeof(struct CameraControlSystemSettings));
        settings->AutoAEC = v4l2_get_control(fd, V4L2_CID_EXPOSURE_AUTO);
        settings->AutoAGC = v4l2_get_control(fd, V4L2_CID_AUTOGAIN);
        settings->Gain = v4l2_get_control(fd, V4L2_CID_GAIN);
        settings->Exposure = v4l2_get_control(fd, V4L2_CID_EXPOSURE);
        settings->Contrast = v4l2_get_control(fd, V4L2_CID_CONTRAST);
        settings->Brightness = v4l2_get_control(fd, V4L2_CID_BRIGHTNESS);

        v4l2_close(fd);
    }

    return settings;
}

void
camera_control_restore_system_settings(CameraControl *cc,
        struct CameraControlSystemSettings *settings)
{
    if (!settings) {
        return;
    }

    int fd = open_v4l2_device(cc->cameraID);
    if (fd != -1) {
        v4l2_set_control(fd, V4L2_CID_EXPOSURE_AUTO, settings->AutoAEC);
        v4l2_set_control(fd, V4L2_CID_AUTOGAIN, settings->AutoAGC);
        v4l2_set_control(fd, V4L2_CID_GAIN, settings->Gain);
        v4l2_set_control(fd, V4L2_CID_EXPOSURE, settings->Exposure);
        v4l2_set_control(fd, V4L2_CID_CONTRAST, settings->Contrast);
        v4l2_set_control(fd, V4L2_CID_BRIGHTNESS, settings->Brightness);

        v4l2_close(fd);
    }

    free(settings);
}

void
camera_control_set_parameters(CameraControl* cc, int autoE, int autoG, int autoWB,
        int exposure, int gain,
        int wbRed, int wbGreen, int wbBlue,
        int contrast, int brightness, enum PSMove_Bool h_flip)
{
    int fd = open_v4l2_device(cc->cameraID);

#if defined(PSMOVE_USE_PSEYE)
    /**
     * Force auto exposure off, workaround by peoro
     * https://github.com/thp/psmoveapi/issues/150
     **/
    autoE = 0xFFFF;
#endif

    if (fd != -1) {
        v4l2_set_control(fd, V4L2_CID_EXPOSURE, exposure);
        v4l2_set_control(fd, V4L2_CID_GAIN, gain);

        v4l2_set_control(fd, V4L2_CID_EXPOSURE_AUTO, autoE);
        v4l2_set_control(fd, V4L2_CID_AUTOGAIN, autoG);
        v4l2_set_control(fd, V4L2_CID_AUTO_WHITE_BALANCE, autoWB);
#if 0
        v4l2_set_control(fd, V4L2_CID_CONTRAST, contrast);
        v4l2_set_control(fd, V4L2_CID_BRIGHTNESS, brightness);
#endif
        v4l2_set_control(fd, V4L2_CID_HFLIP, h_flip);
        v4l2_close(fd);
    }
}

