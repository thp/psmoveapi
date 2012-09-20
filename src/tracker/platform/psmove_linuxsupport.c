
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
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

#include "psmove_linuxsupport.h"

#include <linux/videodev2.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/limits.h>

#include "../../psmove_private.h"

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
            psmove_DEBUG("Detected PSEye (ov534): %s\n", filename);
            result = i;
            break;
        }

        close(video_dev);

        i++;
    }

    return result;
}

