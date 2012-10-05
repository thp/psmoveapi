
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

#include "psmove_osxsupport.h"
#include "../../psmove_private.h"

#import <Foundation/NSAutoreleasePool.h>
#import <AVFoundation/AVFoundation.h>

#define CAMEXPOSURE_DEBUG(msg, ...) \
        psmove_PRINTF("CAM EXPOSURE", msg, ## __VA_ARGS__)

int
macosx_camera_set_exposure_lock(int locked)
{
    /**
     * Exposure locking for the iSight camera on Apple computers.
     * Based on initial experimentations and code by Raphaël de Courville.
     **/

    int result = 0;

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    AVCaptureSession *session = [[AVCaptureSession alloc] init];
    AVCaptureDevice *device = nil;
    AVCaptureDeviceInput *input = nil;

    /* Configure the capture session */
    [session beginConfiguration];

    device = [AVCaptureDevice defaultDeviceWithMediaType: AVMediaTypeVideo];
    if (!device) {
        device = [AVCaptureDevice defaultDeviceWithMediaType: AVMediaTypeMuxed];
    }

    input = [AVCaptureDeviceInput deviceInputWithDevice: device error: nil];
    if (!input) {
        CAMEXPOSURE_DEBUG("Cannot get AVCaptureDeviceInput");
    }

    [session addInput: input];
    [session commitConfiguration];

    /* Lock the device (for configuration), set exposure mode, unlock */
    if ([device lockForConfiguration: nil]) {
        AVCaptureExposureMode mode = locked ?
            AVCaptureExposureModeLocked :
            AVCaptureExposureModeContinuousAutoExposure;

        if ([device isExposureModeSupported: mode]) {
            [device setExposureMode: mode];
        } else {
            CAMEXPOSURE_DEBUG("Device does not support requested exposure mode");
        }

        [device unlockForConfiguration];
        result = 1;
    } else {
        CAMEXPOSURE_DEBUG("Cannot lock device for configuration");
    }

    [session beginConfiguration];
    [session removeInput:input];
    [session commitConfiguration];

    [session autorelease];

    [pool release];
    return result;
}

