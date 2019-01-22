
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
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

#ifdef WIN32
#    include <windows.h>
#endif

#include "opencv2/core/core_c.h"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#define CAM_TO_USE 0
#define SPACE_KEY 32
#define ESC_KEY 27
#define TEXT_COLOR cvScalar(0xFF, 0xFF, 0xFF, 0)
#define TEXT_POS cvPoint(20,30)

#define INTRINSICS_XML "intrinsics.xml"
#define DISTORTION_XML "distortion.xml"

void put_text(IplImage* img, const char* text) {
	CvFont font = cvFont(1.5, 1);
	cvPutText(img, text, TEXT_POS, &font, TEXT_COLOR);
}

IplImage* capture_frame(PSMoveTracker *tracker)
{
	psmove_tracker_update_image(tracker);
	return (IplImage *)psmove_tracker_get_frame(tracker);
}

int main(int arg, char** args) {
	int board_w = 4; // Board width in squares
	int board_h = 7; // Board height
	int n_boards = 10; // Number of boards
	int board_n = board_w * board_h;
	int user_canceled = 0;
	CvSize board_sz = cvSize(board_w, board_h);

	PSMoveTracker* tracker = psmove_tracker_new();
	if (tracker == NULL) {
		printf("Could not create tracker.\n");
		return 2;
	}
	psmove_tracker_set_exposure(tracker, Exposure_HIGH);

    char *intrinsics_xml = psmove_util_get_file_path(INTRINSICS_XML);
    char *distortion_xml = psmove_util_get_file_path(DISTORTION_XML);

	// Allocate Memory
	CvMat* image_points = cvCreateMat(n_boards * board_n, 2, CV_32FC1);
	CvMat* object_points = cvCreateMat(n_boards * board_n, 3, CV_32FC1);
	CvMat* point_counts = cvCreateMat(n_boards, 1, CV_32SC1);
	CvMat* intrinsic_matrix = cvCreateMat(3, 3, CV_32FC1);
	CvMat* distortion_coeffs = cvCreateMat(5, 1, CV_32FC1);
	IplImage *image = capture_frame(tracker);

	CvPoint2D32f* corners = (CvPoint2D32f*)calloc(board_n, sizeof(CvPoint2D32f));
	int i = 0;
	int j = 0;
	for (i = 0; i < board_n; i++)
		corners[i] = cvPoint2D32f(0, 0);

	int corner_count;
	int successes = 0;
	int step = 0;

	IplImage *gray_image1 = cvCreateImage(cvGetSize(image), image->depth, 1);
	IplImage *gray_image2 = cvCreateImage(cvGetSize(image), image->depth, 1);
	// Capture Corner views loop until we've got n_boards
	// succesful captures (all corners on the board are found)
	while (successes < n_boards) {
		int key = cvWaitKey(1);
		user_canceled = key == ESC_KEY;
		if (user_canceled)
			break;

		image = capture_frame(tracker); // Get next image
		cvCvtColor(image, gray_image1, CV_BGR2GRAY);
		corner_count = 0;
		int has_checkBoard = cvCheckChessboard(gray_image1, board_sz);

		if (has_checkBoard) {
			// Find chessboard corners:
			int found = cvFindChessboardCorners(gray_image1, board_sz, corners, &corner_count, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);

			// Get subpixel accuracy on those corners
			cvFindCornerSubPix(gray_image1, corners, corner_count, cvSize(11, 11), cvSize(-1, -1), cvTermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
			// Draw it
			cvDrawChessboardCorners(image, board_sz, corners, corner_count, found);

			char text[222];
			sprintf(text, "captured %d/%d (press 'SPACE' to capture!)", successes, n_boards);

			if (corner_count == board_n)
				put_text(image, text);
			else
				put_text(image, "press 'ESC' to exit");

			// If we got a good board, add it to our data
			if (corner_count == board_n && key == SPACE_KEY) {
				step = successes * board_n;
				for (i = step, j = 0; j < board_n; ++i, ++j) {
					CV_MAT_ELEM( *image_points, float, i, 0 ) = corners[j].x;
					CV_MAT_ELEM( *image_points, float, i, 1 ) = corners[j].y;
					CV_MAT_ELEM( *object_points, float, i, 0 ) = (float)(j / board_w);
					CV_MAT_ELEM( *object_points, float, i, 1 ) = (float)(j % board_w);
					CV_MAT_ELEM( *object_points, float, i, 2 ) = 0.0f;
				}
				CV_MAT_ELEM( *point_counts, int, successes, 0 ) = board_n;
				successes++;
			}
		} else {
			put_text(image, "press 'ESC' to exit");
		}
		cvShowImage("Calibration", image);
	}

	if (!user_canceled) {
		// Allocate matrices according to how many chessboards found
		CvMat* object_points2 = cvCreateMat(successes * board_n, 3, CV_32FC1);
		CvMat* image_points2 = cvCreateMat(successes * board_n, 2, CV_32FC1);
		CvMat* point_counts2 = cvCreateMat(successes, 1, CV_32SC1);

		// Transfer the points into the correct size matrices
		for (i = 0; i < successes * board_n; ++i) {
			CV_MAT_ELEM( *image_points2, float, i, 0) = CV_MAT_ELEM( *image_points, float, i, 0 );
			CV_MAT_ELEM( *image_points2, float, i, 1) = CV_MAT_ELEM( *image_points, float, i, 1 );
			CV_MAT_ELEM( *object_points2, float, i, 0) = CV_MAT_ELEM( *object_points, float, i, 0 );
			CV_MAT_ELEM( *object_points2, float, i, 1) = CV_MAT_ELEM( *object_points, float, i, 1 );
			CV_MAT_ELEM( *object_points2, float, i, 2) = CV_MAT_ELEM( *object_points, float, i, 2 );
		}

		for (i = 0; i < successes; ++i) {
			CV_MAT_ELEM( *point_counts2, int, i, 0 ) = CV_MAT_ELEM( *point_counts, int, i, 0 );
		}
		cvReleaseMat(&object_points);
		cvReleaseMat(&image_points);
		cvReleaseMat(&point_counts);

		// At this point we have all the chessboard corners we need
		// Initiliazie the intrinsic matrix such that the two focal lengths
		// have a ratio of 1.0

		CV_MAT_ELEM( *intrinsic_matrix, float, 0, 0 ) = 1.0;
		CV_MAT_ELEM( *intrinsic_matrix, float, 1, 1 ) = 1.0;

		// Calibrate the camera
		CvTermCriteria default_termination = cvTermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 30, DBL_EPSILON);
		cvCalibrateCamera2(object_points2, image_points2, point_counts2,
                        cvGetSize(image), intrinsic_matrix, distortion_coeffs,
                        NULL, NULL, CV_CALIB_FIX_ASPECT_RATIO, default_termination);

		// Save the intrinsics and distortions
		CvAttrList empty_attribues = cvAttrList(0, 0);
		cvSave(intrinsics_xml, intrinsic_matrix, 0, 0, empty_attribues);
		cvSave(distortion_xml, distortion_coeffs, 0, 0, empty_attribues);

		// Example of loading these matrices back in
		CvMat *intrinsic = (CvMat*) cvLoad(intrinsics_xml, 0, 0, 0);
		CvMat *distortion = (CvMat*) cvLoad(distortion_xml, 0, 0, 0);

		image = capture_frame(tracker);

		// Build the undistort map that we will use for all subsequent frames
		IplImage* mapx = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
		IplImage* mapy = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
		cvInitUndistortMap(intrinsic, distortion, mapx, mapy);

		// Run the camera to the screen, now showing the raw and undistorted image
		while (image) {
			IplImage *t = cvCloneImage(image);
			put_text(image, "press 'P' to pause or 'ESC' to exit");
			cvShowImage("Calibration", image); // Show raw image
			cvRemap(t, image, mapx, mapy, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cvScalarAll(0)); // undistort image
			cvShowImage("Undistorted", image); // Show corrected image

			cvCvtColor(image, gray_image1, CV_BGR2GRAY);
			cvCvtColor(t, gray_image2, CV_BGR2GRAY);
			cvAbsDiff(gray_image1, gray_image2, gray_image1);
			cvShowImage("Difference", gray_image1); // Show corrected image

			cvReleaseImage(&t);
			// Handle pause/unpause and esc
			int c = cvWaitKey(15);
			if (c == 'p') {
				c = 0;
				while (c != 'p' && c != ESC_KEY) {
					c = cvWaitKey(250);
				}
			}
			if (c == ESC_KEY)
				break;
			image = capture_frame(tracker);
		}

		cvReleaseMat(&intrinsic_matrix);
		cvReleaseMat(&distortion_coeffs);
		cvReleaseMat(&object_points2);
		cvReleaseMat(&image_points2);
		cvReleaseMat(&point_counts2);

		cvReleaseImage(&mapx);
		cvReleaseImage(&mapy);
	}

	cvReleaseImage(&gray_image1);
	cvReleaseImage(&gray_image2);

    free(intrinsics_xml);
    free(distortion_xml);
	free(corners);

    psmove_tracker_free(tracker);

	return 0;
}

