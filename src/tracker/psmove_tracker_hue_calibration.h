#pragma once

/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2022, 2023 Thomas Perl <m@thp.io>
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

#include "psmove.h"

#include "opencv2/core/core_c.h"

#include "tracker_helpers.h"

#include <vector>
#include <unordered_map>


namespace psmove {
namespace tracker {

struct HueCalibrationInfo {
    HueCalibrationInfo() = default;

    HueCalibrationInfo(float hue, float dimming, CvScalar cam_hsv, float background_match_fraction=0.f);
    CvScalar diff_hsv() const;
    float penalty_score() const;
    CvScalar rgb() const;

    float hue { 0.f }; // target hue value (S = 255, V = 255)
    float dimming { 1.f }; // dimming value (0.01 dark -> 1.0 bright)
    float cam_hsv[3] { 0.f, 0.f, 0.f }; // HSV value seen by camera
    float background_match_fraction { 0.f }; // fraction of pixels matching HSV range in background
};

struct ColorCalibrationCollection {
    ColorCalibrationCollection() = default;

    bool add(const HueCalibrationInfo &info, float max_hue_distance=0.f);
    std::vector<HueCalibrationInfo> build() const;

private:
    static bool sort_order(const HueCalibrationInfo &a, const HueCalibrationInfo &b);

    std::unordered_map<float, HueCalibrationInfo> map;
};

std::vector<HueCalibrationInfo>
hue_calibration_internal(PSMoveTracker *tracker, PSMove *move,
        CvSize size, std::function<IplImage *()> get_frame,
        CvScalar rHSV, IplConvKernel *kCalib);

} // namespace tracker
} // namespace psmove
