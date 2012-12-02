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

#include "camera_control.h"
#include "psmove_tracker.h"

#include "../external/iniparser/dictionary.h"
#include "../external/iniparser/iniparser.h"

#include "../psmove_private.h"

#include <stdio.h>

#include "camera_control_private.h"

void
get_metrics(int *width, int *height)
{
    *width = psmove_util_get_env_int(PSMOVE_TRACKER_WIDTH_ENV);
    *height = psmove_util_get_env_int(PSMOVE_TRACKER_HEIGHT_ENV);

    if (*width == -1) {
        *width = PSMOVE_TRACKER_DEFAULT_WIDTH;
    }

    if (*height == -1) {
        *height = PSMOVE_TRACKER_DEFAULT_HEIGHT;
    }
}


CameraControl *
camera_control_new(int cameraID)
{
	CameraControl* cc = (CameraControl*) calloc(1, sizeof(CameraControl));
	cc->cameraID = cameraID;

#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
	int w, h;
	int cams = CLEyeGetCameraCount();

	if (cams <= cameraID) {
            free(cc);
            return NULL;
	}

	GUID cguid = CLEyeGetCameraUUID(cameraID);
	cc->camera = CLEyeCreateCamera(cguid,
                CLEYE_COLOR_PROCESSED, CLEYE_VGA, 60);

	CLEyeCameraGetFrameDimensions(cc->camera, &w, &h);

	// Depending on color mode chosen, create the appropriate OpenCV image
	cc->frame4ch = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 4);
	cc->frame3ch = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 3);

	CLEyeCameraStart(cc->camera);
#else
        char *video = psmove_util_get_env_string(PSMOVE_TRACKER_FILENAME_ENV);

        if (video) {
            psmove_DEBUG("Using '%s' as video input.\n", video);
            cc->capture = cvCaptureFromFile(video);
            free(video);
        } else {
            cc->capture = cvCaptureFromCAM(cc->cameraID);

            int width, height;
            get_metrics(&width, &height);

            cvSetCaptureProperty(cc->capture, CV_CAP_PROP_FRAME_WIDTH, width);
            cvSetCaptureProperty(cc->capture, CV_CAP_PROP_FRAME_HEIGHT, height);
        }
#endif

        cc->deinterlace = PSMove_False;

	return cc;
}

void
camera_control_set_deinterlace(CameraControl *cc,
        enum PSMove_Bool enabled)
{
    psmove_return_if_fail(cc != NULL);

    cc->deinterlace = enabled;
}

void
camera_control_read_calibration(CameraControl* cc,
        char* intrinsicsFile, char* distortionFile)
{
    CvMat *intrinsic = (CvMat*) cvLoad(intrinsicsFile, 0, 0, 0);
    CvMat *distortion = (CvMat*) cvLoad(distortionFile, 0, 0, 0);

    if (cc->mapx) {
        cvReleaseImage(&cc->mapx);
    }
    if (cc->mapy) {
        cvReleaseImage(&cc->mapy);
    }

    if (intrinsic && distortion) {
        if (!cc->frame3chUndistort) {
            cc->frame3chUndistort = cvCloneImage(
                    camera_control_query_frame(cc, NULL, NULL));
        }

        int width, height;
        get_metrics(&width, &height);

        cc->mapx = cvCreateImage(cvSize(width, height), IPL_DEPTH_32F, 1);
        cc->mapy = cvCreateImage(cvSize(width, height), IPL_DEPTH_32F, 1);

        cvInitUndistortMap(intrinsic, distortion, cc->mapx, cc->mapy);

        // TODO: Shouldn't we free intrinsic and distortion here?
    } else {
        fprintf(stderr, "Warning: No lens calibration files found.\n");
    }
}

IplImage *
camera_control_query_frame(CameraControl* cc,
        PSMove_timestamp *ts_grab, PSMove_timestamp *ts_retrieve)
{
    IplImage* result;

#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
    // assign buffer-pointer to address of buffer
    cvGetRawData(cc->frame4ch, &cc->pCapBuffer, 0, 0);

    CLEyeCameraGetFrame(cc->camera, cc->pCapBuffer, 2000);

    // convert 4ch image to 3ch image
    const int from_to[] = { 0, 0, 1, 1, 2, 2 };
    const CvArr** src = (const CvArr**) &cc->frame4ch;
    CvArr** dst = (CvArr**) &cc->frame3ch;
    cvMixChannels(src, 1, dst, 1, from_to, 3);

    result = cc->frame3ch;
#else
    cvGrabFrame(cc->capture);
    if (ts_grab != NULL) {
        *ts_grab = _psmove_timestamp();
    }
    result = cvRetrieveFrame(cc->capture, 0);
    if (ts_retrieve != NULL) {
        *ts_retrieve = _psmove_timestamp();
    }
#endif

    if (cc->deinterlace == PSMove_True) {
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
    if (cc->mapx && cc->mapy) {
        cvRemap(result, cc->frame3chUndistort,
                cc->mapx, cc->mapy,
                CV_INTER_LINEAR | CV_WARP_FILL_OUTLIERS,
                cvScalarAll(0));
        result = cc->frame3chUndistort;
    }


#if defined(CAMERA_CONTROL_DEBUG_CAPTURED_IMAGE)
    cvShowImage("camera input", result);
    cvWaitKey(1);
#endif

    return result;
}

void
camera_control_delete(CameraControl* cc)
{
#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
    if (cc->frame3ch != 0x0)
        cvReleaseImage(&cc->frame3ch);

    if (cc->frame4ch != 0x0)
		cvReleaseImage(&cc->frame4ch);

    CLEyeDestroyCamera(cc->camera);
#else
    // linux, others and windows opencv only
    cvReleaseCapture(&cc->capture);
#endif

    if (cc->frame3chUndistort) {
        cvReleaseImage(&cc->frame3chUndistort);
    }

    if (cc->mapx) {
        cvReleaseImage(&cc->mapx);
    }

    if (cc->mapy) {
        cvReleaseImage(&cc->mapy);
    }

    free(cc);
}

