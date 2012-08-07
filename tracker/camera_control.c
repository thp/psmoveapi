#include "camera_control.h"

#include "dictionary.h"
#include "iniparser.h"

#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#if defined(WIN32)
#	include <windows.h>
#	include "CLEyeMulticam.h"
#elif defined(__APPLE__)
/* TODO: OSX Includes */
#else
#	include <fcntl.h>
#	include <linux/videodev2.h>
#	include <libv4l2.h>
#endif

struct _CameraControl {
#if defined(WIN32)
	GUID device;		// used to open the camera on windows
	CLEyeCameraInstance camera;
	IplImage* frame;
	IplImage* frame3ch;
	PBYTE pCapBuffer;
#else
	char device[256];	// used to open the camera on linux
	int open_cv_device;
	CvCapture* capture;
#endif

	IplImage* frame3chUndistort;

	// if a negative value is passed, that means don't touch
	int auto_exp; 	// value range [0-0xFFFF]
	int auto_wb; 	// value range [0-0xFFFF]
	int auto_gain;  // value range [0-0xFFFF]
	int gain; 		// value range [0-0xFFFF]
	int exposure;   // value range [0-0xFFFF]
	int wb_red;     // value range [0-0xFFFF]
	int wb_green;   // value range [0-0xFFFF]
	int wb_blue;    // value range [0-0xFFFF]
	int contrast;   // value range [0-0xFFFF]
	int brightness; // value range [0-0xFFFF]

	IplImage* mapx;
	IplImage* mapy;
};

#ifdef WIN32
void cc_backup_sytem_settings_win(CameraControl* cc, const char* file);
void cc_restore_sytem_settings_win(CameraControl* cc, const char* file);
void cc_set_parameters_win(CameraControl* cc, int autoE, int autoG, int autoWB, int exposure, int gain, int wbRed, int wbGreen, int wbBlue, int contrast,
		int brightness);
#else
void cc_backup_sytem_settings_linux(CameraControl* cc, const char* file);
void cc_restore_sytem_settings_linux(CameraControl* cc, const char* file);
void cc_set_parameters_linux(CameraControl* cc, int autoE, int autoG, int autoWB, int exposure, int gain, int wbRed, int wbGreen, int wbBlue, int contrast,
		int brightness);
#endif

void cc_init_members(CameraControl* cc);

CameraControl* camera_control_new() {
#ifdef WIN32
	int cams = CLEyeGetCameraCount();
	if (cams <= 0) {
		// TODO: outsource CRITICAL macro
		//tracker_CRITICAL("there are no cl-eye cameras to open.");
	}
	assert(cams);
	GUID cguid = CLEyeGetCameraUUID(0);
	return camera_control_new_ex(cguid);
#else
	return camera_control_new_ex("/dev/video1", 1);
#endif
}

#ifndef WIN32
CameraControl* camera_control_new_ex(const char* device, int open_cv_device) {
	CameraControl* cc = (CameraControl*) calloc(1, sizeof(CameraControl));
	sprintf(cc->device, "%s", device);
	cc->open_cv_device=open_cv_device;
	cc_init_members(cc);
	cc->capture = cvCaptureFromCAM(open_cv_device);
	cvSetCaptureProperty(cc->capture, CV_CAP_PROP_FRAME_WIDTH, 640);
	cvSetCaptureProperty(cc->capture, CV_CAP_PROP_FRAME_HEIGHT, 480);
	// not working
	//cvSetCaptureProperty(cc->capture, CV_CAP_PROP_FPS, 60);
	return cc;
}
#endif

#ifdef WIN32
CameraControl* camera_control_new_ex(GUID device) {
	CameraControl* cc = (CameraControl*) calloc(1, sizeof(CameraControl));
	cc->device = device;
	cc_init_members(cc);

	cc->camera = CLEyeCreateCamera(device, CLEYE_COLOR_PROCESSED, CLEYE_VGA, 60);
	int w = 0;
	int h = 0;
	CLEyeCameraGetFrameDimensions(cc->camera, &w, &h);
	// Depending on color mode chosen, create the appropriate OpenCV image
	cc->frame = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 4);
	cc->frame3ch = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 3);
	CLEyeCameraStart(cc->camera);
	return cc;
}
#endif

void cc_init_members(CameraControl* cc) {
	cc->auto_exp = 0;
	cc->auto_wb = 0;
	cc->auto_gain = 0;
	cc->gain = 0;
	cc->exposure = 0;
	cc->wb_red = 0;
	cc->wb_green = 0;
	cc->wb_blue = 0;
	cc->contrast = 0;
	cc->brightness = 0;

	cc->mapx = 0;
	cc->mapy = 0;
}

void camera_control_read_calibration(CameraControl* cc, char* intrinsicsFile, char* distortionFile) {
	CvMat *intrinsic = (CvMat*) cvLoad(intrinsicsFile, 0, 0, 0);
	CvMat *distortion = (CvMat*) cvLoad(distortionFile, 0, 0, 0);

	if (cc->mapx != 0x0)
		cvReleaseImage(&cc->mapx);
	if (cc->mapy != 0x0)
		cvReleaseImage(&cc->mapy);

	printf("\n%s\n", "### Trying to read camera calibration...");
	if (intrinsic != 0 && distortion != 0) {
		if (cc->frame3chUndistort == 0x0)
			cc->frame3chUndistort = cvCloneImage(camera_control_query_frame(cc));

		cc->mapx = cvCreateImage(cvSize(640, 480), IPL_DEPTH_32F, 1);
		cc->mapy = cvCreateImage(cvSize(640, 480), IPL_DEPTH_32F, 1);
		cvInitUndistortMap(intrinsic, distortion, cc->mapx, cc->mapy);

		printf("%s\n", "OK");
	} else {
		printf("%s\n", "Warning");
		printf("%s\n", "--> Unable to read camera calibration files.\n");
		printf("--> Make sure that both \"%s\" and \"%s\" exist.\n", intrinsicsFile, distortionFile);
	}
}

IplImage* camera_control_query_frame(CameraControl* cc) {
	IplImage* retVal;
#ifdef WIN32
	// assign buffer-pointer to address of buffer
	cvGetRawData(cc->frame, &cc->pCapBuffer, 0, 0);
	// read image
	CLEyeCameraGetFrame(cc->camera, cc->pCapBuffer, 2000);
	// convert 4ch image to 3ch image
	const int from_to[] = { 0, 0, 1, 1, 2, 2 };
	const CvArr** src = (const CvArr**) &cc->frame;
	CvArr** dst = (CvArr**) &cc->frame3ch;
	cvMixChannels(src, 1, dst, 1, from_to, 3);
	// return image
	retVal = cc->frame3ch;
#else
	retVal= cvQueryFrame(cc->capture);
#endif

	//IplImage *t = cvCloneImage(retv);
	//cvShowImage("Calibration", image); // Show raw image
	// undistort image
	if (cc->mapx != 0x0 && cc->mapy != 0x0) {
		cvRemap(retVal, cc->frame3chUndistort, cc->mapx, cc->mapy, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
		retVal = cc->frame3chUndistort;
	}

	return retVal;
}

void camera_control_backup_sytem_settings(CameraControl* cc, const char* file) {
#ifdef WIN32
	cc_backup_sytem_settings_win(cc, file);
#else
	cc_backup_sytem_settings_linux(cc,file);
#endif
}

void camera_control_restore_sytem_settings(CameraControl* cc, const char* file) {
#ifdef WIN32
	cc_restore_sytem_settings_win(cc, file);
#else
	cc_restore_sytem_settings_linux(cc,file);
#endif
}

void camera_control_delete(CameraControl** cc) {
	free(*cc);
	*cc = 0;
}

void camera_control_set_parameters(CameraControl* cc, int autoE, int autoG, int autoWB, int exposure, int gain, int wbRed, int wbGreen, int wbBlue,
		int contrast, int brightness) {
#ifdef WIN32
	cc_set_parameters_win(cc, autoE, autoG, autoWB, exposure, gain, wbRed, wbGreen, wbBlue, contrast, brightness);
#else
	cc_set_parameters_linux(cc,autoE, autoG,autoWB,exposure,gain,wbRed,wbGreen,wbBlue,contrast,brightness);
#endif
}

/// INTERNAL FUNCTIONS ///////////////////////////////////////////////////////////////////
void cc_backup_sytem_settings_win(CameraControl* cc, const char* file) {
#ifdef WIN32
	HKEY hKey;
	DWORD l = sizeof(DWORD);
	DWORD AutoAEC = 0;
	DWORD AutoAGC = 0;
	DWORD AutoAWB = 0;
	DWORD Exposure = 0;
	DWORD Gain = 0;
	DWORD wbB = 0;
	DWORD wbG = 0;
	DWORD wbR = 0;
	char* PATH = "Software\\PS3EyeCamera\\Settings";
	int err = RegOpenKeyEx(HKEY_CURRENT_USER, PATH, 0, KEY_ALL_ACCESS, &hKey);
	if (err != ERROR_SUCCESS) {
		printf("Error: %d Unable to open reg-key:  [HKCU]\%s!", err, PATH);
		return;
	}
	RegQueryValueEx(hKey, "AutoAEC", NULL, NULL, (LPBYTE) &AutoAEC, &l);
	RegQueryValueEx(hKey, "AutoAGC", NULL, NULL, (LPBYTE) &AutoAGC, &l);
	RegQueryValueEx(hKey, "AutoAWB", NULL, NULL, (LPBYTE) &AutoAWB, &l);
	RegQueryValueEx(hKey, "Exposure", NULL, NULL, (LPBYTE) &Exposure, &l);
	RegQueryValueEx(hKey, "Gain", NULL, NULL, (LPBYTE) &Gain, &l);
	RegQueryValueEx(hKey, "WhiteBalanceB", NULL, NULL, (LPBYTE) &wbB, &l);
	RegQueryValueEx(hKey, "WhiteBalanceG", NULL, NULL, (LPBYTE) &wbG, &l);
	RegQueryValueEx(hKey, "WhiteBalanceR", NULL, NULL, (LPBYTE) &wbR, &l);

	dictionary* ini = dictionary_new(0);
	iniparser_set(ini, "PSEye", 0);
	iniparser_set_int(ini, "PSEye:AutoAEC", AutoAEC);
	iniparser_set_int(ini, "PSEye:AutoAGC", AutoAGC);
	iniparser_set_int(ini, "PSEye:AutoAWB", AutoAWB);
	iniparser_set_int(ini, "PSEye:Exposure", Exposure);
	iniparser_set_int(ini, "PSEye:Gain", Gain);
	iniparser_set_int(ini, "PSEye:WhiteBalanceB", wbB);
	iniparser_set_int(ini, "PSEye:WhiteBalanceG", wbG);
	iniparser_set_int(ini, "PSEye:WhiteBalanceR", wbG);
	iniparser_save_ini(ini, file);
	dictionary_del(ini);
#endif
}

void cc_backup_sytem_settings_linux(CameraControl* cc, const char* file) {
#if defined(__linux)

	int AutoAEC = 0;
	int AutoAGC = 0;
	int Gain = 0;
	int Exposure = 0;
	int Contrast = 0;
	int Brightness = 0;

	int fd = v4l2_open(cc->device, O_RDWR, 0);

	if (fd != -1) {
		AutoAEC = v4l2_get_control(fd, V4L2_CID_EXPOSURE_AUTO);
		AutoAGC = v4l2_get_control(fd, V4L2_CID_AUTOGAIN);
		Gain = v4l2_get_control(fd, V4L2_CID_GAIN);
		Exposure = v4l2_get_control(fd, V4L2_CID_EXPOSURE);
		Contrast = v4l2_get_control(fd, V4L2_CID_CONTRAST);
		Brightness = v4l2_get_control(fd, V4L2_CID_BRIGHTNESS);
		v4l2_close(fd);

		dictionary* ini = dictionary_new(0);
		iniparser_set(ini, "PSEye", 0);
		iniparser_set_int(ini, "PSEye:AutoAEC", AutoAEC);
		iniparser_set_int(ini, "PSEye:AutoAGC", AutoAGC);
		iniparser_set_int(ini, "PSEye:Gain", Gain);
		iniparser_set_int(ini, "PSEye:Exposure", Exposure);
		iniparser_set_int(ini, "PSEye:Contrast", Contrast);
		iniparser_set_int(ini, "PSEye:Brightness", Brightness);
		iniparser_save_ini(ini, file);
		dictionary_del(ini);
	}
#endif
}

void cc_restore_sytem_settings_win(CameraControl* cc, const char* file) {
#ifdef WIN32
	int NOT_FOUND = -1;
	int val;
	HKEY hKey;
	DWORD l = sizeof(DWORD);

	char* PATH = "Software\\PS3EyeCamera\\Settings";

	int err = RegOpenKeyEx(HKEY_CURRENT_USER, PATH, 0, KEY_ALL_ACCESS, &hKey);
	if (err != ERROR_SUCCESS) {
		printf("Error: %d Unable to open reg-key:  [HKCU]\%s!", err, PATH);
		return;
	}

	dictionary* ini = iniparser_load(file);
	val = iniparser_getint(ini, "PSEye:AutoAEC", NOT_FOUND);
	if (val != NOT_FOUND)
		RegSetValueExA(hKey, "AutoAEC", 0, REG_DWORD, (CONST BYTE*) &val, l);

	val = iniparser_getint(ini, "PSEye:AutoAGC", NOT_FOUND);
	if (val != NOT_FOUND)
		RegSetValueExA(hKey, "AutoAGC", 0, REG_DWORD, (CONST BYTE*) &val, l);

	val = iniparser_getint(ini, "PSEye:AutoAWB", NOT_FOUND);
	if (val != NOT_FOUND)
		RegSetValueExA(hKey, "AutoAWB", 0, REG_DWORD, (CONST BYTE*) &val, l);

	val = iniparser_getint(ini, "PSEye:Exposure", NOT_FOUND);
	if (val != NOT_FOUND)
		RegSetValueExA(hKey, "Exposure", 0, REG_DWORD, (CONST BYTE*) &val, l);

	val = iniparser_getint(ini, "PSEye:Gain", NOT_FOUND);
	if (val != NOT_FOUND)
		RegSetValueExA(hKey, "Gain", 0, REG_DWORD, (CONST BYTE*) &val, l);

	val = iniparser_getint(ini, "PSEye:WhiteBalanceB", NOT_FOUND);
	if (val != NOT_FOUND)
		RegSetValueExA(hKey, "WhiteBalanceB", 0, REG_DWORD, (CONST BYTE*) &val, l);

	val = iniparser_getint(ini, "PSEye:WhiteBalanceG", NOT_FOUND);
	if (val != NOT_FOUND)
		RegSetValueExA(hKey, "WhiteBalanceG", 0, REG_DWORD, (CONST BYTE*) &val, l);

	val = iniparser_getint(ini, "PSEye:WhiteBalanceR", NOT_FOUND);
	if (val != NOT_FOUND)
		RegSetValueExA(hKey, "WhiteBalanceR", 0, REG_DWORD, (CONST BYTE*) &val, l);

	iniparser_freedict(ini);
#endif
}

void cc_restore_sytem_settings_linux(CameraControl* cc, const char* file) {
#if defined(__linux)
	int NOT_FOUND = -1;
	int AutoAEC = 0;
	int AutoAGC = 0;
	int Gain = 0;
	int Exposure = 0;
	int Contrast = 0;
	int Brightness = 0;

	int fd = v4l2_open(cc->device, O_RDWR, 0);

	if (fd != -1) {
		dictionary* ini = iniparser_load(file);
		AutoAEC = iniparser_getint(ini, "PSEye:AutoAEC", NOT_FOUND);
		AutoAGC = iniparser_getint(ini, "PSEye:AutoAGC", NOT_FOUND);
		Gain = iniparser_getint(ini, "PSEye:Gain", NOT_FOUND);
		Exposure = iniparser_getint(ini, "PSEye:Exposure", NOT_FOUND);
		Contrast = iniparser_getint(ini, "PSEye:Contrast", NOT_FOUND);
		Brightness = iniparser_getint(ini, "PSEye:Brightness", NOT_FOUND);
		iniparser_freedict(ini);

		if (AutoAEC != NOT_FOUND)
		v4l2_set_control(fd, V4L2_CID_EXPOSURE_AUTO, AutoAEC);
		if (AutoAGC != NOT_FOUND)
		v4l2_set_control(fd, V4L2_CID_AUTOGAIN, AutoAGC);
		if (Gain != NOT_FOUND)
		v4l2_set_control(fd, V4L2_CID_GAIN, Gain);
		if (Exposure != NOT_FOUND)
		v4l2_set_control(fd, V4L2_CID_EXPOSURE, Exposure);
		if (Contrast != NOT_FOUND)
		v4l2_set_control(fd, V4L2_CID_CONTRAST, Contrast);
		if (Brightness != NOT_FOUND)
		v4l2_set_control(fd, V4L2_CID_BRIGHTNESS, Brightness);
	}
#endif
}

void cc_set_parameters_win(CameraControl* cc, int autoE, int autoG, int autoWB, int exposure, int gain, int wbRed, int wbGreen, int wbBlue, int contrast,
		int brightness) {
#ifdef WIN32
	if (autoE >= 0)
		CLEyeSetCameraParameter(cc->camera, CLEYE_AUTO_EXPOSURE, autoE > 0);
	if (autoG >= 0)
		CLEyeSetCameraParameter(cc->camera, CLEYE_AUTO_GAIN, autoG > 0);
	if (autoWB >= 0)
		CLEyeSetCameraParameter(cc->camera, CLEYE_AUTO_WHITEBALANCE, autoWB > 0);
	if (exposure >= 0)
		CLEyeSetCameraParameter(cc->camera, CLEYE_EXPOSURE, round((511 * exposure) / 0xFFFF));
	if (gain >= 0)
		CLEyeSetCameraParameter(cc->camera, CLEYE_GAIN, round((79 * gain) / 0xFFFF));
	if (wbRed >= 0)
		CLEyeSetCameraParameter(cc->camera, CLEYE_WHITEBALANCE_RED, round((255 * wbRed) / 0xFFFF));
	if (wbGreen >= 0)
		CLEyeSetCameraParameter(cc->camera, CLEYE_WHITEBALANCE_GREEN, round((255 * wbGreen) / 0xFFFF));
	if (wbBlue >= 0)
		CLEyeSetCameraParameter(cc->camera, CLEYE_WHITEBALANCE_BLUE, round((255 * wbBlue) / 0xFFFF));
#endif
}
void cc_set_parameters_linux(CameraControl* cc, int autoE, int autoG, int autoWB, int exposure, int gain, int wbRed, int wbGreen, int wbBlue, int contrast,
		int brightness) {
#if defined(__linux)
	int fd = v4l2_open(cc->device, O_RDWR, 0);

	if (fd != -1) {
		if (autoE >= 0)
		v4l2_set_control(fd, V4L2_CID_EXPOSURE_AUTO, autoE);
		if (autoG >= 0)
		v4l2_set_control(fd, V4L2_CID_AUTOGAIN, autoG);
		if (gain >= 0)
		v4l2_set_control(fd, V4L2_CID_GAIN, gain);
		if (exposure >= 0)
		v4l2_set_control(fd, V4L2_CID_EXPOSURE, exposure);
		if (contrast >= 0)
		v4l2_set_control(fd, V4L2_CID_CONTRAST, contrast);
		if (brightness >= 0)
		v4l2_set_control(fd, V4L2_CID_BRIGHTNESS, brightness);
		v4l2_close(fd);
	}
#endif
}
