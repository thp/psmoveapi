
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012, 2023 Thomas Perl <m@thp.io>
 * Copyright (c) 2012 Benjamin Venditt <benjamin.venditti@gmail.com>
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


#include <stdio.h>

#include "psmove.h"
#include "psmove_tracker.h"
#include "psmove_tracker_opencv.h"
#include "../psmove_format.h"

#include "opencv2/core/core_c.h"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#define SPACE_KEY 32
#define ESC_KEY 27

void
put_text(IplImage *img, const std::string &text)
{
    CvFont font = cvFont(1, 1);

    cvPutText(img, text.c_str(), cvPoint(19, 29), &font, cvScalar(0, 0, 0, 255));
    cvPutText(img, text.c_str(), cvPoint(20, 30), &font, cvScalar(255, 255, 255, 255));
}

IplImage *
capture_frame(PSMoveTracker *tracker)
{
    IplImage *frame = nullptr;

    while (frame == nullptr) {
        psmove_tracker_update_image(tracker);
        frame = psmove_tracker_opencv_get_frame(tracker);
    }

    return frame;
}

int
camera_calibration_main(int argc, char *argv[])
{
    if (argc == 1) {
        PSMOVE_FATAL("Usage: %s filename.xml", argv[0]);
        return 1;
    }

    const char *output_filename = argv[1];

    int board_w = 9; // Board width in squares
    int board_h = 6; // Board height

    size_t n_boards = 10; // Number of boards
    size_t board_n = board_w * board_h;
    bool user_canceled = false;

    CvSize board_sz = cvSize(board_w, board_h);

    PSMoveTracker *tracker = psmove_tracker_new();
    PSMOVE_VERIFY(tracker != nullptr, "Could not create tracker");
    psmove_tracker_set_exposure(tracker, Exposure_HIGH);

    std::vector<std::vector<cv::Vec2f>> image_points;
    std::vector<std::vector<cv::Vec3f>> object_points;

    for (size_t y=0; y<n_boards; ++y) {
        std::vector<cv::Vec2f> image_row;
        std::vector<cv::Vec3f> object_row;
        for (size_t x=0; x<board_n; ++x) {
            image_row.emplace_back(cv::Vec2f(0.f, 0.f));
            object_row.emplace_back(cv::Vec3f(0.f, 0.f, 0.f));
        }
        image_points.emplace_back(image_row);
        object_points.emplace_back(object_row);
    }

    IplImage *image = capture_frame(tracker);

    size_t successes = 0;

    IplImage *gray_image1 = cvCreateImage(cvGetSize(image), image->depth, 1);

    // Capture Corner views loop until we've got n_boards
    // succesful captures (all corners on the board are found)
    while (successes < n_boards) {
        int key = cvWaitKey(1) & 0xFF;
        if (key == ESC_KEY) {
            user_canceled = true;
            break;
        }

        image = capture_frame(tracker);
        cvCvtColor(image, gray_image1, CV_BGR2GRAY);

        int has_checkBoard = cv::checkChessboard(cv::cvarrToMat(gray_image1), board_sz);

        if (has_checkBoard) {
            std::vector<cv::Point2f> corners(board_n);

            // Find chessboard corners:
            bool found = cv::findChessboardCorners(cv::cvarrToMat(gray_image1), board_sz, corners, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);

            // Get subpixel accuracy on those corners
            if (found) {
                int dx = 3;
                int dy = 3;

                for (auto &p: corners) {
                    dx = int(std::min(float(dx), std::min(float(image->width) - 1.f - p.x, p.x)));
                    dy = int(std::min(float(dy), std::min(float(image->height) - 1.f - p.y, p.y)));
                }

                cv::find4QuadCornerSubpix(cv::cvarrToMat(gray_image1), corners, cvSize(dx, dy));
            }

            // Draw it
            cv::drawChessboardCorners(cv::cvarrToMat(image), board_sz, corners, found);

            if (found && corners.size() == board_n) {
                put_text(image, format("captured %d/%d (press 'SPACE' to capture)", int(successes), int(n_boards)));

                // If we got a good board, add it to our data
                if (key == SPACE_KEY) {
                    for (size_t j = 0; j < board_n; ++j) {
                        image_points[successes][j] = cv::Vec2f(corners[j].x, corners[j].y);
                        object_points[successes][j] = cv::Vec3f(float(j / board_w), float(j % board_w), 0.f);
                    }

                    successes++;
                }
            } else {
                put_text(image, "press 'ESC' to exit");
            }
        } else {
            put_text(image, "press 'ESC' to exit");
        }
        cvShowImage("Calibration", image);
    }

    cvReleaseImage(&gray_image1);

    psmove_tracker_free(tracker);

    if (!user_canceled) {
        float intrinsic_init[] = {
            1.f, 0.f, 0.f,
            0.f, 1.f, 0.f,
            0.f, 0.f, 1.f,
        };
        cv::Mat intrinsic_matrix(3, 3, CV_32FC1, intrinsic_init);
        cv::Mat distortion_coeffs(5, 1, CV_32FC1);

        // Calibrate the camera
        std::vector<cv::Mat> rvecs;
        std::vector<cv::Mat> tvecs;
        std::vector<double> stdDeviationsIntrinsics;
        std::vector<double> stdDeviationsExtrinsics;
        std::vector<double> perViewErrors;

        double totalError = cv::calibrateCamera(object_points, image_points,
                cvGetSize(image), intrinsic_matrix, distortion_coeffs,
                rvecs, tvecs, stdDeviationsIntrinsics, stdDeviationsExtrinsics, perViewErrors,
                cv::CALIB_FIX_ASPECT_RATIO, cvTermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 30, DBL_EPSILON));

        PSMOVE_INFO("Total error from cv::calibrateCamera(): %lf", totalError);

        PSMOVE_INFO("Writing to %s", output_filename);
        cv::FileStorage out(output_filename, cv::FileStorage::WRITE);
        out << "intrinsic_matrix" << intrinsic_matrix;
        out << "distortion_coeffs" << distortion_coeffs;
        out.release();
    }

    return 0;
}

int
verify_camera_calibration_main(int argc, char *argv[])
{
    if (argc == 1) {
        PSMOVE_FATAL("Usage: %s filename.xml", argv[0]);
        return 1;
    }

    const char *input_filename = argv[1];

    cv::Mat intrinsic_matrix(3, 3, CV_32FC1);
    cv::Mat distortion_coeffs(5, 1, CV_32FC1);

    PSMOVE_INFO("Reading from %s", input_filename);
    cv::FileStorage in(input_filename, cv::FileStorage::READ);
    in["intrinsic_matrix"] >> intrinsic_matrix;
    in["distortion_coeffs"] >> distortion_coeffs;
    in.release();

    PSMoveTracker *tracker = psmove_tracker_new();
    PSMOVE_VERIFY(tracker != nullptr, "Could not create tracker");
    psmove_tracker_set_exposure(tracker, Exposure_HIGH);

    IplImage *image = capture_frame(tracker);

    // Build the undistort map that we will use for all subsequent frames
    cv::Mat mapx(cvGetSize(image), IPL_DEPTH_32F);
    cv::Mat mapy(cvGetSize(image), IPL_DEPTH_32F);

    cv::Mat R;
    cv::initUndistortRectifyMap(intrinsic_matrix, distortion_coeffs, R, intrinsic_matrix, cvGetSize(image), CV_32FC1, mapx, mapy);

    IplImage *gray_image1 = cvCreateImage(cvGetSize(image), image->depth, 1);
    IplImage *gray_image2 = cvCreateImage(cvGetSize(image), image->depth, 1);

    // Run the camera to the screen, now showing the raw and undistorted image
    while (image) {
        IplImage *t = cvCloneImage(image);
        put_text(image, "press 'P' to pause or 'ESC' to exit");
        cvShowImage("Camera Feed", image); // Show raw image
        cv::remap(cv::cvarrToMat(t), cv::cvarrToMat(image), mapx, mapy, cv::INTER_LINEAR); // undistort image
        cvShowImage("Undistorted", image); // Show corrected image

        cvCvtColor(image, gray_image1, CV_BGR2GRAY);
        cvCvtColor(t, gray_image2, CV_BGR2GRAY);
        cvAbsDiff(gray_image1, gray_image2, gray_image1);
        cvShowImage("Difference", gray_image1); // Show corrected image

        cvReleaseImage(&t);
        // Handle pause/unpause and esc
        int c = cvWaitKey(15) & 0xFF;
        if (c == 'p') {
            c = 0;
            while (c != 'p' && c != ESC_KEY) {
                c = cvWaitKey(250) & 0xFF;
            }
        }
        if (c == ESC_KEY)
            break;
        image = capture_frame(tracker);
    }

    cvReleaseImage(&gray_image1);
    cvReleaseImage(&gray_image2);

    psmove_tracker_free(tracker);

    return 0;
}
