/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2023 Thomas Perl <m@thp.io>
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

#include "camera_control.h"
#include "camera_control_driver.h"
#include "camera_control_layouts.h"

#include "../psmove_private.h"

#import "AVFoundation/AVFoundation.h"

#include <map>
#include <algorithm>
#include <string>


struct CameraControlAVFoundation : public CameraControlOpenCV {
    CameraControlAVFoundation(int camera_id, int width, int height, int framerate);
    virtual ~CameraControlAVFoundation();

    virtual CameraControlFrameLayout get_frame_layout(int width, int height) override;
    virtual void set_parameters(float exposure, bool mirror) override;
    virtual PSMoveCameraInfo get_camera_info() override;

    virtual CameraControlSystemSettings *backup_system_settings() override;
    virtual void restore_system_settings(CameraControlSystemSettings *settings) override;

    int default_width;
    int default_height;
};

CameraControlAVFoundation::CameraControlAVFoundation(int camera_id, int width, int height, int framerate)
    : CameraControlOpenCV(camera_id, width, height, framerate)
{
    capture = new cv::VideoCapture(cameraID);

    // Fall back to default capture width of the device if we don't detect a PS3/PS4/PS5 camera
    default_width = capture->get(cv::CAP_PROP_FRAME_WIDTH);
    default_height = capture->get(cv::CAP_PROP_FRAME_HEIGHT);

    layout = get_frame_layout(width, height);

    capture->set(cv::CAP_PROP_FPS, framerate);
}

CameraControlAVFoundation::~CameraControlAVFoundation()
{
}

struct DetectedCamera {
    DetectedCamera(const std::string &unique_id, const std::string &model_id, const std::string &localized_name)
        : unique_id(unique_id)
        , model_id(model_id)
        , localized_name(localized_name)
    {
    }

    enum PSCameraDevice device_type() const
    {
        if (model_id == "UVC Camera VendorID_1449 ProductID_1420") {
            return PS_CAMERA_PS5_CAMERA;
        } else if (model_id == "UVC Camera VendorID_1449 ProductID_1418") {
            return PS_CAMERA_PS4_CAMERA;
        }

        return PS_CAMERA_UNKNOWN;
    }

    std::string unique_id;
    std::string model_id;
    std::string localized_name;
};

struct DetectedCameras {
    DetectedCameras() = default;
    ~DetectedCameras() = default;

    void init()
    {
        if (inited) {
            return;
        }

        AVCaptureDeviceDiscoverySession *discovery_session = [AVCaptureDeviceDiscoverySession
            discoverySessionWithDeviceTypes: @[AVCaptureDeviceTypeExternalUnknown, AVCaptureDeviceTypeBuiltInWideAngleCamera]
            mediaType: AVMediaTypeVideo
            position: AVCaptureDevicePositionUnspecified];

        static std::map<AVCaptureExposureMode, std::string> MODE_TO_STRING = {
            { AVCaptureExposureModeLocked,                 "Locked" },
            { AVCaptureExposureModeAutoExpose,             "AutoExpose" },
            { AVCaptureExposureModeContinuousAutoExposure, "ContinuousAutoExposure" },
            { AVCaptureExposureModeCustom,                 "Custom" },
        };

        auto exposureModeToString = [] (AVCaptureExposureMode mode) -> std::string {
            auto it = MODE_TO_STRING.find(mode);
            if (it == MODE_TO_STRING.end()) {
                return std::to_string(mode);
            }

            return it->second;
        };

        for (AVCaptureDevice *capture_device in discovery_session.devices) {
            BOOL locked = [capture_device lockForConfiguration: nullptr];
            if (locked) {
                PSMOVE_INFO("Camera Device: %s", [capture_device.modelID UTF8String]);

                PSMOVE_INFO(" - Exposure mode: %s", exposureModeToString(capture_device.exposureMode).c_str());

                PSMOVE_INFO(" - Supported modes:");
                bool found = false;
                for (const auto &mode_string: MODE_TO_STRING) {
                    if ([capture_device isExposureModeSupported: mode_string.first]) {
                        PSMOVE_INFO("   - %s", mode_string.second.c_str());
                        found = true;
                    }
                }

                if (!found) {
                    PSMOVE_INFO("   (none)");
                }

                [capture_device unlockForConfiguration];
            }

            cameras.emplace_back([capture_device.uniqueID UTF8String],
                                 [capture_device.modelID UTF8String],
                                 [capture_device.localizedName UTF8String]);
        }

        // Sort cameras in the same way as OpenCV sorts it, so the indices match (see cap_avfoundation_mac.mm)
        std::sort(cameras.begin(), cameras.end(), [] (const DetectedCamera &a, const DetectedCamera &b) {
            return a.unique_id <= b.unique_id;
        });

        [discovery_session release];

        inited = true;
    }

    const DetectedCamera &get(int index)
    {
        init();

        PSMOVE_VERIFY(index < cameras.size(), "index=%d, cameras.size()=%d", index, int(cameras.size()));

        return cameras[index];
    }

    void each(std::function<void(int index, const DetectedCamera &camera)> callback)
    {
        init();

        for (size_t i=0; i<cameras.size(); ++i) {
            callback(i, cameras[i]);
        }
    }

    size_t count()
    {
        init();

        return cameras.size();
    }

private:
    bool inited { false };
    std::vector<DetectedCamera> cameras;
};

static DetectedCameras
detected_cameras;

struct CameraControlSystemSettings {
    // TODO
};

CameraControlSystemSettings *
CameraControlAVFoundation::backup_system_settings()
{
    // TODO

    return new CameraControlSystemSettings();
}

void
CameraControlAVFoundation::restore_system_settings(struct CameraControlSystemSettings *settings)
{
    if (!settings) {
        return;
    }

    // TODO

    delete settings;
}

void
CameraControlAVFoundation::set_parameters(float exposure, bool mirror)
{
    // TODO
}


CameraControlFrameLayout
CameraControlAVFoundation::get_frame_layout(int width, int height)
{
    const auto &camera = detected_cameras.get(cameraID);

    auto camera_type = camera.device_type();

    switch (camera_type) {
        case PS_CAMERA_PS3_EYE:
        case PS_CAMERA_PS4_CAMERA:
        case PS_CAMERA_PS5_CAMERA:
            return choose_camera_layout(camera_type, width, height);
        case PS_CAMERA_UNKNOWN:
            // TODO: Maybe query resolution from AVFoundation (see above)
            break;
    }

    return CameraControlFrameLayout { default_width, default_height, 0, 0, default_width, default_height };
}

PSMoveCameraInfo
CameraControlAVFoundation::get_camera_info()
{
    const char *camera_name = "Unknown camera";

    const auto &camera = detected_cameras.get(cameraID);

    switch (camera.device_type()) {
        case PS_CAMERA_PS3_EYE:
            camera_name = "PS3 Eye";
            break;
        case PS_CAMERA_PS4_CAMERA:
            camera_name = "PS4 Camera";
            break;
        case PS_CAMERA_PS5_CAMERA:
            camera_name = "PS5 Camera";
            break;
        case PS_CAMERA_UNKNOWN:
            camera_name = camera.localized_name.c_str();
            break;
        default:
            break;
    }

    return PSMoveCameraInfo {
        camera_name,
        "AVFoundation",
        layout.crop_width,
        layout.crop_height,
    };
}

CameraControl *
camera_control_driver_new(int camera_id, int width, int height, int framerate)
{
    detected_cameras.init();

    return new CameraControlAVFoundation(camera_id, width, height, framerate);
}

int
camera_control_driver_get_preferred_camera()
{
    int result = -1;

    detected_cameras.each([&] (int index, const DetectedCamera &camera) {
        switch (camera.device_type()) {
            case PS_CAMERA_PS3_EYE:
            case PS_CAMERA_PS4_CAMERA:
            case PS_CAMERA_PS5_CAMERA:
                result = index;
                break;
            case PS_CAMERA_UNKNOWN:
            default:
                break;
        }
    });

    return result;
}

int
camera_control_driver_count_connected()
{
    return detected_cameras.count();
}
