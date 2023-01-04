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

#include "../../psmove_private.h"

#include <linux/videodev2.h>
#include <libv4l2.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <glob.h>

int open_v4l2_device(int id)
{
    char device_file[512];
    snprintf(device_file, sizeof(device_file), "/dev/video%d", id);
    return v4l2_open(device_file, O_RDWR, 0);
}

enum PSCameraDevice {
    PS_CAMERA_UNKNOWN = 0,
    PS_CAMERA_PS3_EYE = 3,
    PS_CAMERA_PS4_CAMERA = 4,
    PS_CAMERA_PS5_CAMERA = 5,
};

static enum PSCameraDevice
identify_camera(int fd)
{
    struct v4l2_capability capability;
    memset(&capability, 0, sizeof(capability));

    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) == 0) {
        if (strcmp((const char *)(capability.driver), "ov534") == 0) {
            return PS_CAMERA_PS3_EYE;
        }

        if (strcmp((const char *)(capability.card), "USB Camera-OV580: USB Camera-OV") == 0) {
            // It's either a PS4 or PS5 camera, check by looking at supported resolutions

            enum PSCameraDevice result = PS_CAMERA_UNKNOWN;

            struct v4l2_fmtdesc fmt;
            memset(&fmt, 0, sizeof(fmt));
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            fmt.index = 0;

            if (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0) {
                PSMOVE_DEBUG("Video format: '%s'", fmt.description);
                if (fmt.pixelformat != V4L2_PIX_FMT_YUYV) {
                    PSMOVE_WARNING("Unexpected pixel format for OV580 camera: '%s'", fmt.description);
                    return PS_CAMERA_UNKNOWN;
                }

                struct v4l2_frmsizeenum frmsize;
                memset(&frmsize, 0, sizeof(frmsize));
                frmsize.index = 0;
                frmsize.pixel_format = fmt.pixelformat;
                while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
                    if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                        PSMOVE_DEBUG("Frame size %d: %ux%u", frmsize.index, frmsize.discrete.width, frmsize.discrete.height);

                        if (frmsize.discrete.width == 3448 && frmsize.discrete.height == 808) {
                            result = PS_CAMERA_PS4_CAMERA;
                        } else if (frmsize.discrete.width == 5148 && frmsize.discrete.height == 1088) {
                            result = PS_CAMERA_PS5_CAMERA;
                        }

                        struct v4l2_frmivalenum frmival;
                        memset(&frmival, 0, sizeof(frmival));
                        frmival.index = 0;
                        frmival.pixel_format = frmsize.pixel_format;
                        frmival.width = frmsize.discrete.width;
                        frmival.height = frmsize.discrete.height;
                        while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
                            if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                                PSMOVE_DEBUG(" - interval %d: %u/%u (%.5f seconds, %.3f FPS)", frmival.index,
                                        frmival.discrete.numerator, frmival.discrete.denominator,
                                        float(frmival.discrete.numerator) / float(frmival.discrete.denominator),
                                        1.f / (float(frmival.discrete.numerator) / float(frmival.discrete.denominator)));
                            } else {
                                PSMOVE_WARNING("Unexpected frame interval type: 0x%08x", frmival.type);
                            }

                            frmival.index++;
                        }
                    } else {
                        PSMOVE_WARNING("Unexpected frame size type: 0x%08x", frmsize.type);
                    }

                    frmsize.index++;
                }
            }

            return result;
        }
    }

    return PS_CAMERA_UNKNOWN;
}

int
camera_control_get_preferred_camera()
{
    int result = -1;

    glob_t g;
    if (glob("/dev/video*", 0, NULL, &g) == 0) {
        for (size_t i=0; result == -1 && i<g.gl_pathc; ++i) {
            int fd = open(g.gl_pathv[i], O_RDWR);

            if (fd != -1) {
                switch (identify_camera(fd)) {
                    case PS_CAMERA_PS3_EYE:
                    case PS_CAMERA_PS4_CAMERA:
                    case PS_CAMERA_PS5_CAMERA:
                        result = i;
                        break;
                    case PS_CAMERA_UNKNOWN:
                    default:
                        break;
                }

                close(fd);
            }
        }

        globfree(&g);
    }

    return result;
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
        auto settings = new CameraControlSystemSettings;
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

    delete settings;
}

static void
set_v4l2_ctrl(int fd, int cls, int id, int value)
{
    struct v4l2_ext_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));

    struct v4l2_ext_controls ctrls;
    memset(&ctrls, 0, sizeof(ctrls));

    ctrls.which = cls;
    ctrls.count = 1;
    ctrls.controls = &ctrl;

    ctrl.id = id;
    ctrl.value = value;

    int res = ioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls);
    if (res != 0) {
        PSMOVE_WARNING("Could not set V4L2 class 0x%04x, control 0x%08x value (res=%d)", cls, id, res);
    }
}

void
camera_control_set_parameters(CameraControl* cc, float exposure, bool mirror)
{
    int fd = open_v4l2_device(cc->cameraID);

    if (fd != -1) {
        enum PSCameraDevice camera_type = identify_camera(fd);
        switch (camera_type) {
            case PS_CAMERA_PS3_EYE:
                v4l2_set_control(fd, V4L2_CID_GAIN, 0);
                v4l2_set_control(fd, V4L2_CID_AUTOGAIN, 0);

                set_v4l2_ctrl(fd, V4L2_CTRL_CLASS_CAMERA, V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_MANUAL);
                set_v4l2_ctrl(fd, V4L2_CTRL_CLASS_USER, V4L2_CID_EXPOSURE, int(0xFF * std::min(1.f, std::max(0.f, exposure))));

                v4l2_set_control(fd, V4L2_CID_AUTO_WHITE_BALANCE, 0);
                break;
            case PS_CAMERA_PS4_CAMERA:
            case PS_CAMERA_PS5_CAMERA:
                set_v4l2_ctrl(fd, V4L2_CTRL_CLASS_CAMERA, V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_SHUTTER_PRIORITY);
                set_v4l2_ctrl(fd, V4L2_CTRL_CLASS_CAMERA, V4L2_CID_EXPOSURE_ABSOLUTE, int(330 * std::min(1.f, std::max(0.f, exposure = std::pow(exposure, 2.f)))));

                v4l2_set_control(fd, V4L2_CID_AUTO_WHITE_BALANCE, 0);
                break;
            case PS_CAMERA_UNKNOWN:
                v4l2_set_control(fd, V4L2_CID_GAIN, 0);
                v4l2_set_control(fd, V4L2_CID_AUTOGAIN, 0);

                v4l2_set_control(fd, V4L2_CID_EXPOSURE_AUTO, 0);
                v4l2_set_control(fd, V4L2_CID_EXPOSURE, exposure);

                v4l2_set_control(fd, V4L2_CID_AUTO_WHITE_BALANCE, 0);
                break;
        }

        v4l2_set_control(fd, V4L2_CID_HFLIP, mirror);
        v4l2_close(fd);
    }
}

bool
camera_control_get_frame_layout(CameraControl *cc, int width, int height, struct CameraControlFrameLayout *layout)
{
    int fd = open_v4l2_device(cc->cameraID);

    if (fd != -1) {
        enum PSCameraDevice camera_type = identify_camera(fd);
        switch (camera_type) {
            case PS_CAMERA_PS3_EYE:
                if (width == 320 && height == 240) {
                    layout->capture_width = width;
                    layout->capture_height = height;
                    layout->crop_x = 0;
                    layout->crop_y = 0;
                    layout->crop_width = width;
                    layout->crop_height = height;
                    return true;
                } else if ((width == 640 && height == 480) || (width == -1 && height == -1)) {
                    // TODO: Lower-resolution modes
                    layout->capture_width = 640;
                    layout->capture_height = 480;
                    layout->crop_x = 0;
                    layout->crop_y = 0;
                    layout->crop_width = layout->capture_width;
                    layout->crop_height = layout->capture_height;
                    return true;
                }

                PSMOVE_WARNING("Invalid resolution for PS3 Camera: %dx%d", width, height);
                return false;
            case PS_CAMERA_PS4_CAMERA:
                if ((width == 1280 && height == 800) || (width == -1 && height == -1)) {
                    // 3448x808 @ 60, 30, 15, 8
                    // first frame: x = 48 y = 0 w = 1280 h = 800
                    // second frame: x = 1328 y = 0 w = 1280 h = 800
                    layout->capture_width = 3448;
                    layout->capture_height = 808;
                    layout->crop_x = 48;
                    layout->crop_y = 0;
                    layout->crop_width = 1280;
                    layout->crop_height = 800;
                    return true;
                } else if (width == 640 && height == 400) {
                    // 1748x408 @ 120, 60, 30, 15, 8
                    // first frame: x = 48 y = 0 w = 640 h = 400
                    // second frame: x = 688 y = 0 w = 640 h = 400
                    layout->capture_width = 1748;
                    layout->capture_height = 408;
                    layout->crop_x = 48;
                    layout->crop_y = 0;
                    layout->crop_width = width;
                    layout->crop_height = height;
                    return true;
                } else if (width == 320 && height == 192) {
                    // 898x200 @ 240.004, 120, 60, 30
                    // first frame: x = 48 y = 0 w = 320 h = 192
                    // second frame: x = 368 y = 0 w = 320 h = 192
                    layout->capture_width = 898;
                    layout->capture_height = 200;
                    layout->crop_x = 48;
                    layout->crop_y = 0;
                    layout->crop_width = width;
                    layout->crop_height = height;
                    return true;
                }

                PSMOVE_WARNING("Invalid resolution for PS4 Camera: %dx%d", width, height);
                return false;
            case PS_CAMERA_PS5_CAMERA:
                // "Simple" Stereo Modes:
                // 3840x1080 (1920x1080 @ 2x) @ 30, 15, 8
                // 1920x520 (960x520 @ 2x) @ 60
                // 2560x800 (1280x800 @ 2x) @ 60, 30, 15, 8
                // 1280x376 (640x376 @ 2x) @ 120
                // 640x184 (320x184 @ 2x) @ 240.004
                // 896x256 (448x256 @ 2x) @ 120

                // Single Frame Modes:
                // 1920x1080 @ 30, 15, 8
                // 960x520 @ 60
                // 448x256 @ 120
                // 640x376 @ 120

                // "PS4-ish" Modes (Audio, mipmaps?):
                // 5148x1088 @ 30, 15, 8 --> PS4-ish "weird" layout

                // Unusable (shaking horizontally)
                // 1280x800 @ 60, 30, 15
                // 320x184 @ 240.004

                if ((width == 1920 && height == 1080) ||
                        (width == 960 && height == 520) ||
                        (width == 448 && height == 256) ||
                        (width == 640 && height == 376)) {
                    layout->capture_width = width;
                    layout->capture_height = height;
                    layout->crop_x = 0;
                    layout->crop_y = 0;
                    layout->crop_width = width;
                    layout->crop_height = height;
                    return true;
                } else if ((width == 1280 && height == 800) ||
                           (width == 320 && height == 184)) {
                    // Need to use the stereo modes for those two resolutions, as the
                    // non-stereo modes resulted in a horizontally-shaking picture
                    layout->capture_width = width * 2;
                    layout->capture_height = height;
                    layout->crop_x = 0;
                    layout->crop_y = 0;
                    layout->crop_width = width;
                    layout->crop_height = height;
                    return true;
                } else if (width == -1 && height == -1) {
                    layout->capture_width = 1280 * 2;
                    layout->capture_height = 800;
                    layout->crop_x = 0;
                    layout->crop_y = 0;
                    layout->crop_width = 1280;
                    layout->crop_height = 800;
                    return true;
                }

                PSMOVE_WARNING("Invalid resolution for PS5 Camera: %dx%d", width, height);
                return false;
            case PS_CAMERA_UNKNOWN:
                // TODO: Maybe query resolution from V4L2 (see above)
                break;
        }
        v4l2_close(fd);
    }

    return camera_control_fallback_frame_layout(cc, width, height, layout);
}

struct PSMoveCameraInfo
camera_control_get_camera_info(CameraControl *cc)
{
    const char *camera_name = "Unknown camera";

    int fd = open_v4l2_device(cc->cameraID);
    if (fd != -1) {
        switch (identify_camera(fd)) {
            case PS_CAMERA_PS3_EYE:
                camera_name = "PS3 Eye";
                break;
            case PS_CAMERA_PS4_CAMERA:
                camera_name = "PS4 Camera";
                break;
            case PS_CAMERA_PS5_CAMERA:
                camera_name = "PS5 Camera";
                break;
            case PS_CAMERA_UNKNOWN:
            default:
                break;
        }
        v4l2_close(fd);
    }

    return PSMoveCameraInfo {
        camera_name,
        "V4L2",
        cc->layout.crop_width,
        cc->layout.crop_height,
    };
}
