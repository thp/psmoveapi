#ifdef __linux

#include "camera_control.h"

#include <linux/videodev2.h>
#include <libv4l2.h>
#include <fcntl.h>


int open_v4l2_device(int id)
{
    char device_file[512];
    snprintf(device_file, sizeof(device_file), "/dev/video%d", id);
    return v4l2_open(device_file, O_RDWR, 0);
}


void cc_backup_system_settings(CameraControl* cc, const char* file) {
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

void cc_restore_system_settings(CameraControl* cc, const char* file) {
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

void cc_set_parameters(CameraControl* cc, int autoE, int autoG, int autoWB, int exposure, int gain, int wbRed, int wbGreen, int wbBlue, int contrast,
		int brightness) {
	int fd = open_v4l2_device(cc->cameraID);

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
}

#endif
