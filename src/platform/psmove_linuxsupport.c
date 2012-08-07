
#include "psmove_linuxsupport.h"

#include <linux/videodev2.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/limits.h>

#include "../psmove_private.h"

int
linux_find_pseye()
{
    int result = -1;

    char filename[PATH_MAX];
    struct stat st;
    struct v4l2_capability capability;
    int video_dev;
    int i = 0;

    while (1) {
        snprintf(filename, PATH_MAX, "/dev/video%d", i);

        if (stat(filename, &st) != 0) {
            break;
        }

        video_dev = open(filename, O_RDWR);
        psmove_return_val_if_fail(video_dev != -1, -1);

        memset(&capability, 0, sizeof(capability));
        psmove_return_val_if_fail(ioctl(video_dev, VIDIOC_QUERYCAP,
                    &capability) == 0, -1);

        if (strcmp((const char*)(capability.driver), "ov534") == 0) {
#ifdef PSMOVE_DEBUG
            fprintf(stderr, "[PSMOVE] Detected PSEye (ov534): %s\n", filename);
#endif
            result = i;
            break;
        }

        close(video_dev);

        i++;
    }

    return result;
}

