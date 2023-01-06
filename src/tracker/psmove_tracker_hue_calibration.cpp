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

#include <algorithm>

#include "opencv2/core/core_c.h"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/highgui/highgui_c.h"

#include "psmove_tracker.h"
#include "psmove_tracker_opencv.h"

#include "psmove_tracker_hue_calibration.h"

#include "../psmove_private.h"
#include "../psmove_port.h"
#include "../psmove_format.h"

#include "tracker_helpers.h"

namespace {

struct HueCalibrationImage {
    HueCalibrationImage(CvScalar hsv)
        : hsv(hsv)
        , rgb(th_hsv2rgb(hsv))
    {
        PSMOVE_INFO("rgb @ hue=%f: %f %f %f", hsv.val[0], rgb.val[0], rgb.val[1], rgb.val[2]);
    }

    CvScalar hsv;
    CvScalar rgb;

    IplImage *on_image { nullptr };
};

} // end anonymous namespace


namespace psmove {
namespace tracker {

HueCalibrationInfo::HueCalibrationInfo(float hue, float dimming, CvScalar cam_hsv, float background_match_fraction)
    : hue(hue)
    , dimming(dimming)
    , cam_hsv{float(cam_hsv.val[0]), float(cam_hsv.val[1]), float(cam_hsv.val[2])}
    , background_match_fraction(background_match_fraction)
{
}

CvScalar
HueCalibrationInfo::diff_hsv() const
{
    return cvScalar(
        cam_hsv[0] - hue,
        cam_hsv[1] - 255.f,
        cam_hsv[2] - 255.f,
        0.f
    );
}

float
HueCalibrationInfo::penalty_score() const
{
    auto diff = diff_hsv();

    return -2.f * std::abs(diff.val[0]) + 0.1f * diff.val[1] + 0.1f * diff.val[2] - 5000.f * background_match_fraction;
}

CvScalar
HueCalibrationInfo::rgb() const
{
    return th_hsv2rgb(cvScalar(hue, 255.f, 255.f, 255.f));
}


bool
ColorCalibrationCollection::add(const HueCalibrationInfo &info, float max_hue_distance)
{
    float h = info.cam_hsv[0];
    float s = info.cam_hsv[1];
    float v = info.cam_hsv[2];

    float hue_distance = std::abs(info.diff_hsv().val[0]);

    if (h < 1.f && s < 1.f && v < 1.f) {
        PSMOVE_WARNING("Ignoring - could not find/mask color");
    } else if (max_hue_distance > 0.f && hue_distance > max_hue_distance) {
        PSMOVE_WARNING("Ignoring - hue in camera too far apart");
    } else if (v <= 90 && s <= 90) {
        PSMOVE_WARNING("Ignoring - value AND saturation are too low");
    } else if (v <= 120 || s <= 120) {
        PSMOVE_WARNING("Ignoring - value OR saturation are too low");
    } else {
        auto it = map.find(info.hue);

        // If we don't have one yet or we found a better one, save it
        if (it == map.end() || (it->second.penalty_score() < info.penalty_score())) {
            map[info.hue] = info;
        }

        return true;
    }

    return false;
}

std::vector<HueCalibrationInfo>
ColorCalibrationCollection::build() const
{
    std::vector<HueCalibrationInfo> result;

    for (const auto it: map) {
        const auto &info = it.second;

        PSMOVE_INFO("hue: %f", info.hue);

        auto rgb = info.rgb();

        PSMOVE_INFO("dimming: %f", info.dimming);
        PSMOVE_INFO("rgb = (%f, %f, %f)", rgb.val[0], rgb.val[1], rgb.val[2]);
        PSMOVE_INFO("cam_hsv = (%f, %f, %f)", info.cam_hsv[0], info.cam_hsv[1], info.cam_hsv[2]);
        PSMOVE_INFO("background_match_fraction = %d %%", int(100 * info.background_match_fraction));
        PSMOVE_INFO("score = %.2f", info.penalty_score());
        PSMOVE_INFO("-");

        result.emplace_back(info);
    }

    std::sort(result.begin(), result.end(), sort_order);

    return result;
}

bool
ColorCalibrationCollection::sort_order(const HueCalibrationInfo &a, const HueCalibrationInfo &b)
{
    // Lower penalty equals higher rank
    return a.penalty_score() >= b.penalty_score();
}


std::vector<HueCalibrationInfo>
hue_calibration_internal(PSMoveTracker *tracker, PSMove *move, CvSize size,
        std::function<IplImage *()> get_frame, CvScalar rHSV, IplConvKernel *kCalib)
{
    // This might need to be adjusted depending on background noise
    int hue_diff_threshold = 40;

    ColorCalibrationCollection color_calibration_collection;

    int e = int(100 * psmove_tracker_get_exposure(tracker));

    int dimming_steps = 3;
    for (int d=0; d<dimming_steps; ++d) {
        float dim = powf(0.3f, d);
        PSMOVE_INFO("Dimming: %f (d=%d)", dim, d);

        std::vector<HueCalibrationImage> hue_images;

        auto hsv = cvScalar(0.0, 255.0, 255.0, 1.0);

        int steps = std::max(5, psmove_count_connected() + 1);

        double offset = 0.5;
        double cycle = 180.0;

        double hue_distance = cycle / steps;
        PSMOVE_INFO("Hue distance: %.2f", hue_distance);

        hsv.val[0] = offset * cycle / steps;
        while (hsv.val[0] < cycle) {
            hue_images.emplace_back(hsv);
            hsv.val[0] += cycle / steps;
        }

        IplImage *off_image = cvCreateImage(size, 8, 3);
        IplImage *hsv_off_image = cvCreateImage(size, 8, 3);
        IplImage *gray_on_image = cvCreateImage(size, 8, 1);
        IplImage *gray_off_image = cvCreateImage(size, 8, 1);
        IplImage *diff_image = cvCreateImage(size, 8, 1);
        IplImage *mask_image = nullptr;
        IplImage *inv_mask_image = cvCreateImage(size, 8, 1);

        {
            // turn off the controller
            psmove_set_leds(move, 0, 0, 0);
            psmove_update_leds(move);

            // capture the scene with the controller unlit
            IplImage *off_out = get_frame();
            cvCopy(off_out, off_image, NULL);
            //th_dump_image(format("hue-off-dim%d-exp%d.png", d, e), off_image);

            cvCvtColor(off_image, gray_off_image, CV_BGR2GRAY);
            cvCvtColor(off_image, hsv_off_image, CV_BGR2HSV);
        }

        // go through all the hues and capture the images (+ build a mask)
        for (auto &hue_image: hue_images) {
            int hue = hue_image.hsv.val[0];

            // turn on the controller
            psmove_set_leds(move, hue_image.rgb.val[0] * dim, hue_image.rgb.val[1] * dim, hue_image.rgb.val[2] * dim);
            psmove_update_leds(move);

            IplImage *on_out = get_frame();
            //th_dump_image(format("hue-on-h%d-dim%d-exp%d.png", hue, d, e), on_out);

            hue_image.on_image = cvCloneImage(on_out);

            cvCvtColor(on_out, gray_on_image, CV_RGB2GRAY);
            cvAbsDiff(gray_on_image, gray_off_image, diff_image);
            //th_dump_image(format("hue-diff-h%d-dim%d-exp%d.png", hue, d, e), diff_image);

            cvThreshold(diff_image, diff_image, hue_diff_threshold, 0xFF /* white */, CV_THRESH_BINARY);
            //th_dump_image(format("hue-diff-thr-h%d-dim%d-exp%d.png", hue, d, e), diff_image);

            cvErode(diff_image, diff_image, kCalib, 1);
            cvDilate(diff_image, diff_image, kCalib, 1);
            //th_dump_image(format("hue-diff-denoise-h%d-dim%d-exp%d.png", hue, d, e), diff_image);

            if (!mask_image) {
                mask_image = cvCreateImage(size, 8, 1);
                cvCopy(diff_image, mask_image, NULL);
            } else {
                cvAnd(diff_image, mask_image, mask_image, NULL);
            }
        }

        //th_dump_image(format("hue-mask-dim%d-exp%d.png", d, e), mask_image);

        // inverse mask for hue detection and environment hue detection
        cvNot(mask_image, inv_mask_image);

        // turn off the controller again
        psmove_set_leds(move, 0, 0, 0);
        psmove_update_leds(move);

        for (auto &hue_image: hue_images) {
            int hue = hue_image.hsv.val[0];

            // zero out all pixels not in the mask
            cvSet(hue_image.on_image, cvScalar(0.0, 0.0, 0.0, 0.0), inv_mask_image);
            //th_dump_image(format("hue-color-masked-h%d-dim%d-exp%d.png", hue, d, e), hue_image.on_image);

            IplImage *hsv_image = cvCloneImage(hue_image.on_image);
            cvCvtColor(hue_image.on_image, hsv_image, CV_BGR2HSV);
            auto average_hsv = cvAvg(hsv_image, mask_image);
            cvReleaseImage(&hsv_image);

            // calculate upper & lower bounds for the color filter
            CvScalar min = th_scalar_sub(average_hsv, rHSV);
            CvScalar max = th_scalar_add(average_hsv, rHSV);

            IplImage *environment_hsv_image = cvCloneImage(hsv_off_image);
            IplImage *environment_mask_image = cvCreateImage(size, 8, 1);
            cvInRangeS(environment_hsv_image, min, max, environment_mask_image);
            cvReleaseImage(&environment_hsv_image);

            auto info = psmove::tracker::HueCalibrationInfo(hue_image.hsv.val[0], dim, average_hsv,
                    float(cvCountNonZero(environment_mask_image)) / float(size.width * size.height));

            auto diff_hsv = info.diff_hsv();

            std::vector<std::string> info_text = {
                format("Dimming stage:        dim=%.5f", dim),

                format("Original RGB:         R=%3d, G=%3d, B=%3d", int(hue_image.rgb.val[0]), int(hue_image.rgb.val[1]), int(hue_image.rgb.val[2])),
                format("Dimmed RGB:           R=%3d, G=%3d, B=%3d", int(hue_image.rgb.val[0]*dim), int(hue_image.rgb.val[1]*dim), int(hue_image.rgb.val[2]*dim)),

                format("Original HSV:         H=%3d, S=%3d, V=%3d", int(hue_image.hsv.val[0]), int(hue_image.hsv.val[1]), int(hue_image.hsv.val[2])),
                format("Average HSV (masked): H=%3d, S=%3d, V=%3d", int(average_hsv.val[0]), int(average_hsv.val[1]), int(average_hsv.val[2])),
                format("Difference in HSV:    H=%3d, S=%3d, V=%3d", int(diff_hsv.val[0]), int(diff_hsv.val[1]), int(diff_hsv.val[2])),
                format("Environment match:    %d %%", int(100 * info.background_match_fraction)),

                format("Penalty score:        %.2f", info.penalty_score()),
            };

            CvFont fontSmall = cvFont(0.8, 1);
            int x = 10;
            int y = 300;

            for (auto &line: info_text) {
                cvPutText(hue_image.on_image, line.c_str(), cvPoint(x, y), &fontSmall, cvScalar(255, 255, 255, 255));
                y += 10;

                PSMOVE_INFO("%s", line.c_str());
            }
            PSMOVE_INFO("---");

            // for debugging, show the original RGB value in the image
            cvRectangle(hue_image.on_image, cvPoint(x, y), cvPoint(x+50, y+50), th_rgb2bgr(hue_image.rgb), CV_FILLED, 8, 0);
            y += 50;

            if (color_calibration_collection.add(info, hue_distance * 0.8f)) {
                //th_dump_image(format("hue-color-masked-annotated-exp%d-h%03d-dim%d-envmask.png", e, hue, d), environment_mask_image);
                //th_dump_image(format("hue-color-masked-annotated-exp%d-h%03d-dim%d.png", e, hue, d), hue_image.on_image);
            } else {
                //th_dump_image(format("excluded-hue-color-masked-annotated-exp%d-h%03d-dim%d-envmask.png", e, hue, d), environment_mask_image);
                //th_dump_image(format("excluded-hue-color-masked-annotated-exp%d-h%03d-dim%d.png", e, hue, d), hue_image.on_image);
            }

            cvReleaseImage(&environment_mask_image);
            cvReleaseImage(&hue_image.on_image);
        }

        cvReleaseImage(&off_image);
        cvReleaseImage(&hsv_off_image);
        cvReleaseImage(&gray_on_image);
        cvReleaseImage(&gray_off_image);
        cvReleaseImage(&diff_image);
        cvReleaseImage(&mask_image);
        cvReleaseImage(&inv_mask_image);
    }

    return color_calibration_collection.build();
}

} // end namespace tracker
} // end namespace psmove
