#ifdef __APPLE__

#include "camera_control.h"

#include <stdio.h>

void
camera_control_set_parameters(CameraControl* cc,
        int autoE, int autoG, int autoWB,
        int exposure, int gain,
        int wbRed, int wbGreen, int wbBlue,
        int contrast, int brightness)
{
    /* TODO: Implement for Mac OS X */
    fprintf(stderr, "<noimpl> camera_control_set_parameters\n");
}

void
camera_control_backup_system_settings(CameraControl* cc,
        const char* file)
{
    /* TODO: Implement for Mac OS X */
    fprintf(stderr, "<noimpl> camera_control_backup_system_settings\n");
}

void camera_control_restore_system_settings(CameraControl* cc,
        const char* file)
{
    /* TODO: Implement for Mac OS X */
    fprintf(stderr, "<noimpl> camera_control_restore_system_settings\n");
}

#endif
