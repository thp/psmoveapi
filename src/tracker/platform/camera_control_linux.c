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

#include "../../../external/iniparser/iniparser.h"

#include <linux/videodev2.h>
#include <libv4l2.h>
#include <fcntl.h>


int open_v4l2_device(int id)
{
    char device_file[512];
    snprintf(device_file, sizeof(device_file), "/dev/video%d", id);
    return v4l2_open(device_file, O_RDWR, 0);
}


void camera_control_backup_system_settings(CameraControl* cc, const char* file) {
	int AutoAEC = 0;
	int AutoAGC = 0;
	int Gain = 0;
	int Exposure = 0;
	int Contrast = 0;
	int Brightness = 0;

	int fd = open_v4l2_device(cc->cameraID);

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
}

void camera_control_restore_system_settings(CameraControl* cc, const char* file) {
	int NOT_FOUND = -1;
	int AutoAEC = 0;
	int AutoAGC = 0;
	int Gain = 0;
	int Exposure = 0;
	int Contrast = 0;
	int Brightness = 0;

	int fd = open_v4l2_device(cc->cameraID);

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
}

void
camera_control_set_parameters(CameraControl* cc, int autoE, int autoG, int autoWB,
        int exposure, int gain,
        int wbRed, int wbGreen, int wbBlue,
        int contrast, int brightness)
{
    int fd = open_v4l2_device(cc->cameraID);

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
        v4l2_close(fd);
    }
}

