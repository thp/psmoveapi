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

#include "../../../external/iniparser/iniparser.h"

#include "../camera_control.h"
#include "../camera_control_private.h"

void camera_control_backup_system_settings(CameraControl* cc, const char* file) {
#if !defined(CAMERA_CONTROL_USE_CL_DRIVER) && defined(PSMOVE_USE_PSEYE)
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
    char* PATH = CL_DRIVER_REG_PATH;
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

void camera_control_restore_system_settings(CameraControl* cc, const char* file) {
#if !defined(CAMERA_CONTROL_USE_CL_DRIVER) && defined(PSMOVE_USE_PSEYE)
    int NOT_FOUND = -1;
    int val;
    HKEY hKey;
    DWORD l = sizeof(DWORD);

    char* PATH = CL_DRIVER_REG_PATH;
    int err = RegOpenKeyEx(HKEY_CURRENT_USER, PATH, 0, KEY_ALL_ACCESS, &hKey);
    if (err != ERROR_SUCCESS) {
        printf("Error: %d Unable to open reg-key:  [HKCU]\%s!", err, PATH);
        return;
    }

    dictionary* ini = iniparser_load(file);
    val = iniparser_getint(ini, "PSEye:AutoAEC", NOT_FOUND);
    if (val != NOT_FOUND){
        RegSetValueExA(hKey, "AutoAEC", 0, REG_DWORD, (CONST BYTE*) &val, l);
    }

    val = iniparser_getint(ini, "PSEye:AutoAGC", NOT_FOUND);
    if (val != NOT_FOUND){
        RegSetValueExA(hKey, "AutoAGC", 0, REG_DWORD, (CONST BYTE*) &val, l);
    }

	val = iniparser_getint(ini, "PSEye:AutoAWB", NOT_FOUND);
    if (val != NOT_FOUND){
        RegSetValueExA(hKey, "AutoAWB", 0, REG_DWORD, (CONST BYTE*) &val, l);
    }

	val = iniparser_getint(ini, "PSEye:Exposure", NOT_FOUND);
    if (val != NOT_FOUND){
        RegSetValueExA(hKey, "Exposure", 0, REG_DWORD, (CONST BYTE*) &val, l);
    }

	val = iniparser_getint(ini, "PSEye:Gain", NOT_FOUND);
    if (val != NOT_FOUND){
        RegSetValueExA(hKey, "Gain", 0, REG_DWORD, (CONST BYTE*) &val, l);
    }

	val = iniparser_getint(ini, "PSEye:WhiteBalanceR", NOT_FOUND);
    if (val != NOT_FOUND){
        RegSetValueExA(hKey, "WhiteBalanceR", 0, REG_DWORD, (CONST BYTE*) &val, l);
    }

	val = iniparser_getint(ini, "PSEye:WhiteBalanceB", NOT_FOUND);
    if (val != NOT_FOUND){
        RegSetValueExA(hKey, "WhiteBalanceB", 0, REG_DWORD, (CONST BYTE*) &val, l);
    }

	val = iniparser_getint(ini, "PSEye:WhiteBalanceG", NOT_FOUND);
    if (val != NOT_FOUND){
        RegSetValueExA(hKey, "WhiteBalanceG", 0, REG_DWORD, (CONST BYTE*) &val, l);
    }

	iniparser_freedict(ini);
#endif
}

void camera_control_set_parameters(CameraControl* cc, int autoE, int autoG, int autoWB, int exposure, int gain, int wbRed, int wbGreen, int wbBlue, int contrast,
		int brightness) {
#if defined(CAMERA_CONTROL_USE_CL_DRIVER)
    if (autoE >= 0){
        CLEyeSetCameraParameter(cc->camera, CLEYE_AUTO_EXPOSURE, autoE > 0);
    }
    if (autoG >= 0){
        CLEyeSetCameraParameter(cc->camera, CLEYE_AUTO_GAIN, autoG > 0);
    }
    if (autoWB >= 0){
        CLEyeSetCameraParameter(cc->camera, CLEYE_AUTO_WHITEBALANCE, autoWB > 0);
    }
    if (exposure >= 0){
        CLEyeSetCameraParameter(cc->camera, CLEYE_EXPOSURE, round((511 * exposure) / 0xFFFF));
    }
    if (gain >= 0){
        CLEyeSetCameraParameter(cc->camera, CLEYE_GAIN, round((79 * gain) / 0xFFFF));
    }
    if (wbRed >= 0){
        CLEyeSetCameraParameter(cc->camera, CLEYE_WHITEBALANCE_RED, round((255 * wbRed) / 0xFFFF));
    }
    if (wbGreen >= 0){
        CLEyeSetCameraParameter(cc->camera, CLEYE_WHITEBALANCE_GREEN, round((255 * wbGreen) / 0xFFFF));
    }
    if (wbBlue >= 0){
        CLEyeSetCameraParameter(cc->camera, CLEYE_WHITEBALANCE_BLUE, round((255 * wbBlue) / 0xFFFF));
    }
#elif defined(PSMOVE_USE_PSEYE)
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
    val = round((511 * exposure) / 0xFFFF);
    RegSetValueExA(hKey, "Exposure", 0, REG_DWORD, (CONST BYTE*) &val, l);
    val = round((79 * gain) / 0xFFFF);
    RegSetValueExA(hKey, "Gain", 0, REG_DWORD, (CONST BYTE*) &val, l);
    val = round((255 * wbRed) / 0xFFFF);
    RegSetValueExA(hKey, "WhiteBalanceR", 0, REG_DWORD, (CONST BYTE*) &val, l);
    val = round((255 * wbGreen) / 0xFFFF);
    RegSetValueExA(hKey, "WhiteBalanceG", 0, REG_DWORD, (CONST BYTE*) &val, l);
    val = round((255 * wbBlue) / 0xFFFF);
    RegSetValueExA(hKey, "WhiteBalanceB", 0, REG_DWORD, (CONST BYTE*) &val, l);

    // restart the camera capture with openCv
    if (cc->capture) {
            cvReleaseCapture(&cc->capture);
        }
    
    ps3eye_set_parameters(cc->eye, autoG > 0, autoWB > 0, gain, exposure, contrast, brightness);
    
    int width, height;
    get_metrics(&width, &height);

    cc->capture = cvCaptureFromCAM(cc->cameraID);
    cvSetCaptureProperty(cc->capture, CV_CAP_PROP_FRAME_WIDTH, width);
    cvSetCaptureProperty(cc->capture, CV_CAP_PROP_FRAME_HEIGHT, height);
#endif
}

