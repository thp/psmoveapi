
#include "camera_control.h"
#include "psmove_tracker.h"

#include "../external/iniparser/dictionary.h"
#include "../external/iniparser/iniparser.h"

#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#include <stdio.h>

#if defined(WIN32)
#    include <windows.h>
/* Comment out the next line to use pure OpenCV on Windows, too */
#    define CAMERA_CONTROL_USE_CL_DRIVER
#endif

#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
#    define CL_DRIVER_REG_PATH "Software\\PS3EyeCamera\\Settings"
#    include "../external/CLEye/CLEyeMulticam.h"
#endif


struct _CameraControl {
	int cameraID;
	IplImage* frame;
	IplImage* frame3chUndistort;

#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
	CLEyeCameraInstance camera;
	IplImage* frame3ch;
	PBYTE pCapBuffer;
#endif

	CvCapture* capture;

	// if a negative value is passed, that means it is not changed
	int auto_exp; // value range [0-0xFFFF]
	int auto_wb; // value range [0-0xFFFF]
	int auto_gain; // value range [0-0xFFFF]
	int gain; // value range [0-0xFFFF]
	int exposure; // value range [0-0xFFFF]
	int wb_red; // value range [0-0xFFFF]
	int wb_green; // value range [0-0xFFFF]
	int wb_blue; // value range [0-0xFFFF]
	int contrast; // value range [0-0xFFFF]
	int brightness; // value range [0-0xFFFF]

	IplImage* mapx;
	IplImage* mapy;
};

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
	cc->frame = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 4);
	cc->frame3ch = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 3);

	CLEyeCameraStart(cc->camera);
#else
	cc->capture = cvCaptureFromCAM(cc->cameraID);
	cvSetCaptureProperty(cc->capture,
                CV_CAP_PROP_FRAME_WIDTH, PSMOVE_TRACKER_POSITION_X_MAX);
	cvSetCaptureProperty(cc->capture,
                CV_CAP_PROP_FRAME_HEIGHT, PSMOVE_TRACKER_POSITION_Y_MAX);
#endif

	return cc;
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
                    camera_control_query_frame(cc));
        }

        cc->mapx = cvCreateImage(cvSize(PSMOVE_TRACKER_POSITION_X_MAX,
                    PSMOVE_TRACKER_POSITION_Y_MAX), IPL_DEPTH_32F, 1);
        cc->mapy = cvCreateImage(cvSize(PSMOVE_TRACKER_POSITION_X_MAX,
                    PSMOVE_TRACKER_POSITION_Y_MAX), IPL_DEPTH_32F, 1);

        cvInitUndistortMap(intrinsic, distortion, cc->mapx, cc->mapy);

        // TODO: Shouldn't we free intrinsic and distortion here?
    } else {
        fprintf(stderr, "Warning: No lens calibration files found.\n");
    }
}

IplImage *
camera_control_query_frame(CameraControl* cc)
{
    IplImage* result;

#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
    // assign buffer-pointer to address of buffer
    cvGetRawData(cc->frame, &cc->pCapBuffer, 0, 0);

    CLEyeCameraGetFrame(cc->camera, cc->pCapBuffer, 2000);

    // convert 4ch image to 3ch image
    const int from_to[] = { 0, 0, 1, 1, 2, 2 };
    const CvArr** src = (const CvArr**) &cc->frame;
    CvArr** dst = (CvArr**) &cc->frame3ch;
    cvMixChannels(src, 1, dst, 1, from_to, 3);

    result = cc->frame3ch;
#else
    result = cvQueryFrame(cc->capture);
#endif

    // undistort image
    if (cc->mapx && cc->mapy) {
        cvRemap(result, cc->frame3chUndistort,
                cc->mapx, cc->mapy,
                CV_INTER_LINEAR | CV_WARP_FILL_OUTLIERS,
                cvScalarAll(0));
        result = cc->frame3chUndistort;
    }

    return result;
}

void
camera_control_delete(CameraControl* cc)
{
#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
    if (cc->frame3ch != 0x0)
        cvReleaseImage(&cc->frame3ch);
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

