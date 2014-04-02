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

#ifndef CAMERA_CONTROL_PRIV_H
#define CAMERA_CONTROL_PRIV_H

#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#if defined(WIN32)
#    include <windows.h>
#endif

#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
#    include "../external/CLEye/CLEyeMulticam.h"
#else
#    define CL_DRIVER_REG_PATH "Software\\PS3EyeCamera\\Settings"
#endif

#if defined(CAMERA_CONTROL_USE_PS3EYE_DRIVER)
#    include "ps3eyedriver.h"
#endif

struct _CameraControl {
	int cameraID;
	IplImage* frame3chUndistort;

#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
	CLEyeCameraInstance camera;
	IplImage* frame3ch;
	IplImage* frame4ch;
	PBYTE pCapBuffer;
#endif

#if defined(CAMERA_CONTROL_USE_PS3EYE_DRIVER)
        ps3eye_t *eye;
        int width;
        int height;
        IplImage *framebgr;
#endif

	CvCapture* capture;

	IplImage* mapx;
	IplImage* mapy;

        enum PSMove_Bool deinterlace;
};

#endif

void
get_metrics(int *width, int *height);
