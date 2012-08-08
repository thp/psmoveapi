#ifndef CAMERA_CONTROL_PRIV_H
#define CAMERA_CONTROL_PRIV_H

#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#if defined(WIN32)
#    include <windows.h>
/* Comment out the next line to use pure OpenCV on Windows, too */
//#    define CAMERA_CONTROL_USE_CL_DRIVER
#endif

#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
#    include "../external/CLEye/CLEyeMulticam.h"
#else
#    define CL_DRIVER_REG_PATH "Software\\PS3EyeCamera\\Settings"
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

#endif
