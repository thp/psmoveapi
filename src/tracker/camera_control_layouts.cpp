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


#include "camera_control_layouts.h"

#include <unordered_map>

namespace {

static const std::unordered_map<PSCameraDevice, std::vector<CameraControlFrameLayout>>
LAYOUTS = {
    {
        PS_CAMERA_PS3_EYE,
        {
            /* capture_width, capture_height, crop_x, crop_y, crop_width, crop_height */
            {  640,  480,    0,    0,  640,  480 },
            {  320,  240,    0,    0,  320,  240 },
        },
    },
    {
        PS_CAMERA_PS4_CAMERA,
        {
            /* capture_width, capture_height, crop_x, crop_y, crop_width, crop_height */
            { 3448,  808,   48,    0, 1280,  800 },
            { 1748,  408,   48,    0,  640,  400 },
            {  898,  200,   48,    0,  320,  192 },
        },
    },
    {
        PS_CAMERA_PS5_CAMERA,
        {
            /* capture_width, capture_height, crop_x, crop_y, crop_width, crop_height */
            { 2560,  800,    0,    0, 1280,  800 },
            { 1920, 1080,    0,    0, 1920, 1080 },
            {  960,  520,    0,    0,  960,  520 },
            {  640,  376,    0,    0,  640,  376 },
            {  640,  184,    0,    0,  320,  184 },
        },
    },
};

} // end anonymous namespace


CameraControlFrameLayout
choose_camera_layout(enum PSCameraDevice camera_type, int width, int height)
{
    auto it = LAYOUTS.find(camera_type);

    PSMOVE_VERIFY(it != LAYOUTS.end(), "Could not find layouts for device type %d", int(camera_type));

    auto &layouts = it->second;

    if (width == -1 && height == -1) {
        return layouts[0];
    }

    for (const auto &layout: layouts) {
        if (layout.crop_width == width && layout.crop_height == height) {
            return layout;
        }
    }

    PSMOVE_FATAL("Unsupported resolution: %dx%d", width, height);
    return layouts[0];
}
