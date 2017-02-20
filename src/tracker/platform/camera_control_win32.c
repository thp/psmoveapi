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


#include <stdio.h>

#include "psmove_tracker.h"

#include "../camera_control.h"
#include "../camera_control_private.h"

struct CameraControlSystemSettings {
    DWORD AutoAEC;
    DWORD AutoAGC;
    DWORD AutoAWB;
    DWORD Exposure;
    DWORD Gain;
    DWORD wbB;
    DWORD wbG;
    DWORD wbR;
};

struct CameraControlSystemSettings *
camera_control_backup_system_settings(CameraControl *cc)
{
#if !defined(CAMERA_CONTROL_USE_PS3EYE_DRIVER) && defined(PSMOVE_USE_PSEYE)
    HKEY hKey;
    DWORD l = sizeof(DWORD);

    int err = RegOpenKeyEx(HKEY_CURRENT_USER, CL_DRIVER_REG_PATH, 0, KEY_ALL_ACCESS, &hKey);
    if (err != ERROR_SUCCESS) {
        printf("Error: %d Unable to open reg-key:  [HKCU]\\%s!", err, CL_DRIVER_REG_PATH);
        return NULL;
    }

    struct CameraControlSystemSettings *settings = calloc(1, sizeof(struct CameraControlSystemSettings));
    RegQueryValueEx(hKey, "AutoAEC", NULL, NULL, (LPBYTE) &settings->AutoAEC, &l);
    RegQueryValueEx(hKey, "AutoAGC", NULL, NULL, (LPBYTE) &settings->AutoAGC, &l);
    RegQueryValueEx(hKey, "AutoAWB", NULL, NULL, (LPBYTE) &settings->AutoAWB, &l);
    RegQueryValueEx(hKey, "Exposure", NULL, NULL, (LPBYTE) &settings->Exposure, &l);
    RegQueryValueEx(hKey, "Gain", NULL, NULL, (LPBYTE) &settings->Gain, &l);
    RegQueryValueEx(hKey, "WhiteBalanceB", NULL, NULL, (LPBYTE) &settings->wbB, &l);
    RegQueryValueEx(hKey, "WhiteBalanceG", NULL, NULL, (LPBYTE) &settings->wbG, &l);
    RegQueryValueEx(hKey, "WhiteBalanceR", NULL, NULL, (LPBYTE) &settings->wbR, &l);
    RegCloseKey(hKey);

    return settings;
#else
    return NULL;
#endif
}
void
camera_control_restore_system_settings(CameraControl *cc,
        struct CameraControlSystemSettings *settings)
{
#if !defined(CAMERA_CONTROL_USE_PS3EYE_DRIVER) && defined(PSMOVE_USE_PSEYE)
    if (settings) {
        HKEY hKey;
        DWORD l = sizeof(DWORD);

        char* PATH = CL_DRIVER_REG_PATH;
        int err = RegOpenKeyEx(HKEY_CURRENT_USER, PATH, 0, KEY_ALL_ACCESS, &hKey);
        if (err != ERROR_SUCCESS) {
            printf("Error: %d Unable to open reg-key:  [HKCU]\\%s!", err, PATH);
            return;
        }

        RegSetValueEx(hKey, "AutoAEC", 0, REG_DWORD, (CONST BYTE *) &settings->AutoAEC, &l);
        RegSetValueEx(hKey, "AutoAGC", 0, REG_DWORD, (CONST BYTE *) &settings->AutoAGC, &l);
        RegSetValueEx(hKey, "AutoAWB", 0, REG_DWORD, (CONST BYTE *) &settings->AutoAWB, &l);
        RegSetValueEx(hKey, "Exposure", 0, REG_DWORD, (CONST BYTE *) &settings->Exposure, &l);
        RegSetValueEx(hKey, "Gain", 0, REG_DWORD, (CONST BYTE *) &settings->Gain, &l);
        RegSetValueEx(hKey, "WhiteBalanceB", 0, REG_DWORD, (CONST BYTE *) &settings->wbB, &l);
        RegSetValueEx(hKey, "WhiteBalanceG", 0, REG_DWORD, (CONST BYTE *) &settings->wbG, &l);
        RegSetValueEx(hKey, "WhiteBalanceR", 0, REG_DWORD, (CONST BYTE *) &settings->wbR, &l);
        RegCloseKey(hKey);

        free(settings);
    }
#endif
}

void camera_control_set_parameters(CameraControl* cc, int autoE, int autoG, int autoWB, int exposure, int gain, int wbRed, int wbGreen, int wbBlue, int contrast, int brightness, enum PSMove_Bool h_flip)
{
#if defined(CAMERA_CONTROL_USE_PS3EYE_DRIVER)
	//autoE... setAutoExposure not defined in ps3eye.h
	ps3eye_set_parameter(cc->eye, PS3EYE_AUTO_GAIN,				autoG > 0);
	ps3eye_set_parameter(cc->eye, PS3EYE_AUTO_WHITEBALANCE,		autoWB > 0);
	ps3eye_set_parameter(cc->eye, PS3EYE_EXPOSURE,				(int)((511 * exposure) / 0xFFFF));
	ps3eye_set_parameter(cc->eye, PS3EYE_GAIN,					(int)((79 * gain) / 0xFFFF));
	ps3eye_set_parameter(cc->eye, PS3EYE_BRIGHTNESS,			(int)((255 * brightness) / 0xFFFF));
	ps3eye_set_parameter(cc->eye, PS3EYE_HFLIP, 				h_flip);

	//ps3eye_set_parameter(cc->eye, PS3EYE_REDBALANCE, (int)((255 * wbRed) / 0xFFFF));
	//wbGreen... setGreenBalance not defined in ps3eye.h
	//ps3eye_set_parameter(cc->eye, PS3EYE_BLUEBALANCE, (int)((255 * wbBlue) / 0xFFFF));
	//ps3eye_set_parameter(cc->eye, PS3EYE_CONTRAST, contrast);  // Transform unknown.
	//ps3eye_set_parameter(cc->eye, PS3EYE_BRIGHTNESS, brightness);  // Transform unknown.

	/** The following parameters could be set but are not passed into this function:
	* ps3eye_set_parameter(cc->eye, PS3EYE_SHARPNESS, ??);
	* ps3eye_set_parameter(cc->eye, PS3EYE_HUE, ??);
	* ps3eye_set_parameter(cc->eye, PS3EYE_HFLIP, ??);
	* ps3eye_set_parameter(cc->eye, PS3EYE_VFLIP, ??);
	**/
#else
	int val;
	HKEY hKey;
	DWORD l = sizeof(DWORD);
	char* PATH = CL_DRIVER_REG_PATH;
	int err = RegOpenKeyEx(HKEY_CURRENT_USER, PATH, 0, KEY_ALL_ACCESS, &hKey);
	if (err != ERROR_SUCCESS) {
		printf("Error: %d Unable to open reg-key:  [HKCU]\%s!", err, PATH);
		return;
	}
	val = autoE > 0;
	RegSetValueExA(hKey, "AutoAEC", 0, REG_DWORD, (CONST BYTE*) &val, l);
	val = autoG > 0;
	RegSetValueExA(hKey, "AutoAGC", 0, REG_DWORD, (CONST BYTE*) &val, l);
	val = autoWB > 0;
	RegSetValueExA(hKey, "AutoAWB", 0, REG_DWORD, (CONST BYTE*) &val, l);
	val = (int)((511 * exposure) / 0xFFFF);
	RegSetValueExA(hKey, "Exposure", 0, REG_DWORD, (CONST BYTE*) &val, l);
	val = (int)((79 * gain) / 0xFFFF);
	RegSetValueExA(hKey, "Gain", 0, REG_DWORD, (CONST BYTE*) &val, l);
	val = (int)((255 * wbRed) / 0xFFFF);
	RegSetValueExA(hKey, "WhiteBalanceR", 0, REG_DWORD, (CONST BYTE*) &val, l);
	val = (int)((255 * wbGreen) / 0xFFFF);
	RegSetValueExA(hKey, "WhiteBalanceG", 0, REG_DWORD, (CONST BYTE*) &val, l);
	val = (int)((255 * wbBlue) / 0xFFFF);
	RegSetValueExA(hKey, "WhiteBalanceB", 0, REG_DWORD, (CONST BYTE*) &val, l);

	// restart the camera capture with openCv
	if (cc->capture) {
            cvReleaseCapture(&cc->capture);
        }
	 
	int width, height;
	get_metrics(&width, &height);

	cc->capture = cvCaptureFromCAM(cc->cameraID);
	cvSetCaptureProperty(cc->capture, CV_CAP_PROP_FRAME_WIDTH, width);
	cvSetCaptureProperty(cc->capture, CV_CAP_PROP_FRAME_HEIGHT, height);
#endif
}

