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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "opencv2/core/core_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#include "psmove_tracker.h"
#include "tracker_helpers.h"
#include "tracked_controller.h"
#include "tracked_color.h"
#include "high_precision_timer.h"
#include "tracker_trace.h"
#include "camera_control.h"

#define DIMMING_FACTOR 1 // Factor to dim the color of the RGB LEDs
#define BLINKS 4                 // number of diff images to create during calibration
#define ROIS 5                   // the number of levels of regions of interest (roi)
#define CALIB_MIN_SIZE 100		 // minimum size of the estimated glowing sphere during calibration process
#define CALIB_SIZE_STD 10	     // maximum standard deviation (in %) of the glowing spheres found during calibration process
#define CALIB_MAX_DIST 30		 // minimum size of the estimated glowing sphere during calibration process
struct _PSMoveTracker {
	int cam; // the camera to use
	//CvCapture* capture; // the camera device opened for capture
	CameraControl* cc;
	IplImage* frame; // the current frame of the camera
	int exposure; // the exposure to use
	IplImage* roiI[ROIS]; // array of images for each level of roi (colored)
	IplImage* roiM[ROIS]; // array of images for each level of roi (greyscale)
	IplConvKernel* kCalib; // kernel used for morphological operations during calibration
	CvScalar rHSV; // the range of the color filter
	TrackedController* controllers; // a pointer to a linked list of connected controllers
	PSMoveTrackingColor* available_colors; // a pointer to a linked list of available tracking colors
	HPTimer* timer; // pointer to a high-precision timer used for internal calculation and debugging purposes
	float debug_fps; // the current FPS achieved by "psmove_tracker_update"
	time_t debug_last_live; // the timesamp when the last live-image was written to the file-system
};

/* Macro: Print a critical message if an assertion fails */
#define tracker_CRITICAL(x) \
        {fprintf(stderr, "[TRACKER] Assertion fail in %s: %s\n", __func__, x);}

/**
 * Adapts the cameras exposure to the current lighting conditions
 * This function will adapt to the most suitable exposure, it will start
 * with "expMin" and increases step by step to "expMax" until it reaches "lumMin" or "expMax"
 *
 * tracker - A valid PSMoveTracker * instance
 * limMin  - Minimal luminance to reach
 * expMin  - Minimal exposure to test
 * expMax  - Maximal exposure to test
 *
 * Returns: the most suitable exposure within range
 **/
int psmove_tracker_adapt_to_light(PSMoveTracker *tracker, int lumMin, int expMin, int expMax);

/**
 * This function switches the sphere of the given PSMove on to the given color and takes
 * a picture via the given capture. Then it switches it of and takes a picture again. A difference image
 * is calculated from these two images. It stores the image of the lit sphere and
 * of the diff-image in the passed parameter "on" and "diff". Before taking
 * a picture it waits for the specified delay (in microseconds).
 *
 * tracker - the tracker that contains the camera control
 * move    - the PSMove controller to use
 * r,g,b   - the RGB color to use to lit the sphere
 * on	   - the pre-allocated image to store the captured image when the sphere is lit
 * diff    - the pre-allocated image to store the calculated diff-image
 * delay   - the time to wait before taking a picture (in microseconds)
 **/
void psmove_tracker_get_diff(PSMoveTracker* tracker, PSMove* move, int r, int g, int b, IplImage* on, IplImage* diff, int delay);

/**
 * This function assures thate the roi (region of interest) is always within the bounds
 * of the camera image.
 *
 * tc         - The TrackableController containing the roi to check & fix
 * roi_width  - the width of the roi
 * roi_height - the height of the roi
 * cam_width  - the width of the camera image
 * cam_height - the height of the camera image
 **/
void psmove_tracker_fix_roi(TrackedController* tc, int roi_width, int image_height, int cam_width, int cam_height);

/**
 * This function prepares the linked list of suitable colors, that can be used for tracking.
 */
void psmove_tracker_prepare_colors(PSMoveTracker* tracker);

/**
 * This function is just the internal implementation of "psmove_tracker_update"
 */
int psmove_tracker_update_controller(PSMoveTracker* tracker, TrackedController* tc);
int psmove_tracker_update_controller2(PSMoveTracker* tracker, TrackedController* tc);

/**
 * This draws tracking statistics into the current camera image. This is only used internally.
 *
 * tracker - the Tracker to use
 * tc      - the TrackedController to draw the statistics of. Or NULL, if all shall be drawn.
 */
void psmove_tracker_draw_tracking_stats(PSMoveTracker* tracker, TrackedController* tc);

void psmove_tracker_biggest_contour(IplImage* img, CvSeq** resContour, float* resSize);

PSMoveTracker *
psmove_tracker_new() {
	int i = 0;
	PSMoveTracker* t = (PSMoveTracker*) calloc(1, sizeof(PSMoveTracker));
	t->controllers = 0x0;
	t->rHSV = cvScalar(7, 85, 85, 0);
	t->cam = CV_CAP_ANY;
	t->timer = hp_timer_create();
	t->debug_fps = 0;
	t->debug_last_live = 0;

	// prepare available colors for tracking
	psmove_tracker_prepare_colors(t);

	// start the video capture device for tracking
	t->cc = camera_control_new();

	camera_control_read_calibration(t->cc, "IntrinsicsPSMove.xml", "DistortionPSMove.xml");

	// use static exposure
	t->exposure = 2051;
	// use static adaptive exposure
	//t->exposure = psmove_tracker_adapt_to_light(t, 25, 2051, 4051);
	camera_control_set_parameters(t->cc, 0, 0, 0, t->exposure, 0, 0xffff, 0xffff, 0xffff, -1, -1);

	// just query a frame
	IplImage* frame;
	while (1) {
		frame = camera_control_query_frame(t->cc);
		if (!frame)
			continue;
		else
			break;
	}

	// prepare ROI data structures
	t->roiI[0] = cvCreateImage(cvGetSize(frame), frame->depth, 3);
	t->roiM[0] = cvCreateImage(cvGetSize(frame), frame->depth, 1);
	for (i = 1; i < ROIS; i++) {
		IplImage* z = t->roiI[i - 1];
		int w = z->width;
		t->roiI[i] = cvCreateImage(cvSize(w * 0.6, w * 0.6), z->depth, 3);
		t->roiM[i] = cvCreateImage(cvSize(w * 0.6, w * 0.6), z->depth, 1);
	}

	t->kCalib = cvCreateStructuringElementEx(5, 5, 3, 3, CV_SHAPE_RECT, 0x0);
	return t;
}

enum PSMoveTracker_Status psmove_tracker_enable(PSMoveTracker *tracker, PSMove *move) {
	// check if there is a free color, return on error immediately
	PSMoveTrackingColor* color = tracker->available_colors;
	for (; color != 0x0 && color->is_used; color = color->next)
		;

	if (color == 0x0)
		return Tracker_CALIBRATION_ERROR;

	unsigned char r = color->r;
	unsigned char g = color->g;
	unsigned char b = color->b;

	return psmove_tracker_enable_with_color(tracker, move, r, g, b);
}

enum PSMoveTracker_Status psmove_tracker_enable_with_color(PSMoveTracker *tracker, PSMove *move, unsigned char r, unsigned char g, unsigned char b) {
	// check if the controller is already enabled!
	if (tracked_controller_find(tracker->controllers, move))
		return Tracker_CALIBRATED;

	// check if the color is already in use, if not, mark it as used, return if it is already used
	PSMoveTrackingColor* tracked_color = tracked_color_find(tracker->available_colors, r, g, b);
	if (tracked_color != 0x0 && tracked_color->is_used)
		return Tracker_CALIBRATION_ERROR;

	// clear the calibration html trace
	psmove_html_trace_clear();

	IplImage* frame = camera_control_query_frame(tracker->cc);
	IplImage* images[BLINKS]; // array of images saved during calibration for estimation of sphere color
	IplImage* diffs[BLINKS]; // array of masks saved during calibration for estimation of sphere color
	double sizes[BLINKS]; // array of blob sizes saved during calibration for estimation of sphere color
	int i;
	for (i = 0; i < BLINKS; i++) {
		images[i] = cvCreateImage(cvGetSize(frame), frame->depth, 3);
		diffs[i] = cvCreateImage(cvGetSize(frame), frame->depth, 1);
	}
	// controller is not enabled ... enable it!
	PSMoveTracker* t = tracker;
	CvScalar assignedColor = cvScalar(b, g, r, 0);
	psmove_html_trace_var_color("assignedColor", assignedColor);

	// for each blink
	for (i = 0; i < BLINKS; i++) {
		// create a diff image
		psmove_tracker_get_diff(tracker, move, r, g, b, images[i], diffs[i], 100);

		psmove_html_trace_image_at(images[i], i, "originals");
		psmove_html_trace_image_at(diffs[i], i, "rawdiffs");
		// threshold it to reduce image noise
		cvThreshold(diffs[i], diffs[i], 20, 0xFF, CV_THRESH_BINARY);

		psmove_html_trace_image_at(diffs[i], i, "threshdiffs");

		// use morphological operations to further remove noise
		cvErode(diffs[i], diffs[i], t->kCalib, 1);
		cvDilate(diffs[i], diffs[i], t->kCalib, 1);

		psmove_html_trace_image_at(diffs[i], i, "erodediffs");
	}

	// put the diff images together!
	for (i = 1; i < BLINKS; i++) {
		cvAnd(diffs[0], diffs[i], diffs[0], 0x0);
	}

	// find the biggest contour
	float sizeBest = 0;
	CvSeq* contourBest = 0x0;
	psmove_tracker_biggest_contour(diffs[0], &contourBest, &sizeBest);

	// use only the biggest contour for the following color estimation
	cvSet(diffs[0], th_black, 0x0);
	if (contourBest)
		cvDrawContours(diffs[0], contourBest, th_white, th_white, -1, CV_FILLED, 8, cvPoint(0, 0));

	psmove_html_trace_image_at(diffs[0], 0, "finaldiff");

	if (cvCountNonZero(diffs[0]) < CALIB_MIN_SIZE) {
		psmove_html_trace_log_entry("WARNING", "The final mask my not be representative for color estimation.");
	}

	// calculate the avg color within the biggest contour
	CvScalar color = cvAvg(images[0], diffs[0]);
	CvScalar hsv_assigned = th_brg2hsv(assignedColor);
	CvScalar hsv_color = th_brg2hsv(color);
	psmove_html_trace_var_color("estimatedColor", color);

	psmove_html_trace_var_int("estimated_hue", hsv_color.val[0]);
	psmove_html_trace_var_int("assigned_hue", hsv_assigned.val[0]);
	psmove_html_trace_var_int("allowed_hue_difference", t->rHSV.val[0]);

	if (abs(hsv_assigned.val[0] - hsv_color.val[0]) > t->rHSV.val[0]) {
		psmove_html_trace_log_entry("WARNING", "The estimated color seems not to be similar to the color it should be.");
	}

	// just reusing the data structure
	IplImage* mask = diffs[0];

	int valid_countours = 0;
	// calculate upper & lower bounds for the color filter
	CvScalar min, max;
	th_minus(hsv_color.val, t->rHSV.val, min.val, 3);
	th_plus(hsv_color.val, t->rHSV.val, max.val, 3);
	// for each image (where the sphere was lit)

	CvPoint firstPosition = cvPoint(-9999, 9999);
	for (i = 0; i < BLINKS; i++) {
		// convert to HSV
		cvCvtColor(images[i], images[i], CV_BGR2HSV);
		// apply color filter
		cvInRangeS(images[i], min, max, mask);

		// use morphological operations to further remove noise
		//cvErode(mask,mask, t->kCalib, 1);
		//cvDilate(mask,mask, t->kCalib, 1);

		psmove_html_trace_image_at(mask, i, "filtered");
		psmove_tracker_biggest_contour(mask, &contourBest, &sizeBest);

		// there may be only a single contour in this picture with at least 100px size
		sizes[i] = 0;
		float dist = 9999;
		CvRect bBox;
		if (contourBest) {
			bBox = cvBoundingRect(contourBest, 0);
			if (i == 0) {
				firstPosition = cvPoint(bBox.x, bBox.y);
			}
			dist = sqrt(pow(firstPosition.x - bBox.x, 2) + pow(firstPosition.y - bBox.y, 2));
			sizes[i] = sizeBest;
		}

		// check for errors (no contour, more than one contour, or contour too small)
		if (contourBest == 0x0) {
			psmove_html_trace_array_item_at(i, "contours", "no contour");
		} else if (sizes[i] <= CALIB_MIN_SIZE) {
			psmove_html_trace_array_item_at(i, "contours", "too small");
		} else if (dist >= CALIB_MAX_DIST) {
			psmove_html_trace_array_item_at(i, "contours", "too far away");
		} else {
			psmove_html_trace_array_item_at(i, "contours", "OK");
			valid_countours++;
		}

	}

	// clean up all temporary images
	for (i = 0; i < BLINKS; i++) {
		cvReleaseImage(&images[i]);
		cvReleaseImage(&diffs[i]);
	}

	int CHECK_HAS_ERRORS = 0;
	// check if sphere was found in each BLINK image
	if (valid_countours < BLINKS) {
		psmove_html_trace_log_entry("ERROR", "The sphere could not be found in all images.");
		CHECK_HAS_ERRORS++;
	}

	// check if the size of the found contours are similar
	double stdSizes = sqrt(th_var(sizes, BLINKS));
	printf("stdSizes: %f   vs   %f \n", stdSizes, th_avg(sizes, BLINKS) / 100.0);
	if (stdSizes >= (th_avg(sizes, BLINKS) / 100.0 * CALIB_SIZE_STD)) {
		psmove_html_trace_log_entry("ERROR", "The spheres found differ too much in size.");
		CHECK_HAS_ERRORS++;
	}


	if (CHECK_HAS_ERRORS)
		return Tracker_CALIBRATION_ERROR;

	// insert to list
	TrackedController* itm = tracked_controller_insert(&tracker->controllers, move);
	// set current color
	itm->dColor = cvScalar(b, g, r, 0);
	// set estimated color
	itm->eColor = color;
	itm->eColorHSV = hsv_color;
	itm->roi_x = 0;
	itm->roi_y = 0;
	itm->roi_level = 0;
	// set the state of the available colors
	if (tracked_color != 0x0)
		tracked_color->is_used = 1;
	return Tracker_CALIBRATED;
}

int psmove_tracker_get_color(PSMoveTracker *tracker, PSMove *move, unsigned char *r, unsigned char *g, unsigned char *b) {
	TrackedController* tc = tracked_controller_find(tracker->controllers, move);
	if (tc != 0x0) {
		*r = tc->dColor.val[2] * DIMMING_FACTOR;
		*g = tc->dColor.val[1] * DIMMING_FACTOR;
		*b = tc->dColor.val[0] * DIMMING_FACTOR;
		return 1;
	} else
		return 0;
}

void psmove_tracker_disable(PSMoveTracker *tracker, PSMove *move) {
	TrackedController* tc = tracked_controller_find(tracker->controllers, move);
	PSMoveTrackingColor* color = tracked_color_find(tracker->available_colors, tc->dColor.val[2], tc->dColor.val[1], tc->dColor.val[0]);
	if (tc != 0x0) {
		tracked_controller_remove(&tracker->controllers, move);
		tracked_controller_release(&tc, 0);
	}
	if (color != 0x0)
		color->is_used = 0;
}

enum PSMoveTracker_Status psmove_tracker_get_status(PSMoveTracker *tracker, PSMove *move) {
	TrackedController* tc = tracked_controller_find(tracker->controllers, move);
	if (tc != 0x0) {
		return Tracker_CALIBRATED;
	} else {
		return Tracker_UNCALIBRATED;
	}
}

void*
psmove_tracker_get_image(PSMoveTracker *tracker) {
	return (void*)(tracker->frame);
}

void psmove_tracker_update_image(PSMoveTracker *tracker) {
	tracker->frame = camera_control_query_frame(tracker->cc);
}

int psmove_tracker_update_controller(PSMoveTracker *tracker, TrackedController* tc) {
	PSMoveTracker* t = tracker;
	int i = 0;
	int sphere_found = 0;

	// calculate upper & lower bounds for the color filter
	CvScalar min, max;
	th_minus(tc->eColorHSV.val, t->rHSV.val, min.val, 3);
	th_plus(tc->eColorHSV.val, t->rHSV.val, max.val, 3);
	// this is the tracking algo
	while (1) {
		IplImage *roi_i = t->roiI[tc->roi_level];
		IplImage *roi_m = t->roiM[tc->roi_level];

		// cut out the roi!
		cvSetImageROI(t->frame, cvRect(tc->roi_x, tc->roi_y, roi_i->width, roi_i->height));
		cvCvtColor(t->frame, roi_i, CV_BGR2HSV);
		// apply color filter
		cvInRangeS(roi_i, min, max, roi_m);

		float sizeBest = 0;
		CvSeq* contourBest = 0x0;
		psmove_tracker_biggest_contour(roi_m, &contourBest, &sizeBest);

		if (contourBest) {
			sphere_found = 1;
			CvMoments mu;
			CvRect br = cvBoundingRect(contourBest, 0);

			// apply color filter
			cvInRangeS(roi_i, min, max, roi_m);
			// whenn calling "cvFindContours", the original image is partly consumed -> restoring the contour in the binary image
			//cvDrawContours(roi_m, best, th_white, th_white, -1, CV_FILLED, 8, cvPoint(0, 0));

			// use adaptive color detection
			CvScalar newColor = cvAvg(t->frame, roi_m);
			th_plus(tc->eColor.val, newColor.val, tc->eColor.val, 3);
			th_mul(tc->eColor.val, 0.5, tc->eColor.val, 3);
			tc->eColorHSV = th_brg2hsv(tc->eColor);

			// calucalte image-moments to estimate the center off mass (x/y position of the blob)
			cvSetImageROI(roi_m, br);
			cvMoments(roi_m, &mu, 0);
			cvResetImageROI(roi_m);
			CvPoint p = cvPoint(mu.m10 / mu.m00, mu.m01 / mu.m00);
			tc->x = p.x + tc->roi_x + br.x;
			tc->y = p.y + tc->roi_y + br.y;
			tc->r = sqrt(cvCountNonZero(roi_m) / th_PI);

			// update the future roi box
			br.width = th_max(br.width, br.height) * 2;
			br.height = br.width;
			for (i = 0; i < ROIS; i++) {
				if (br.width > t->roiI[i]->width && br.height > t->roiI[i]->height)
					break;

				tc->roi_level = i;
				// update easy accessors
				roi_i = t->roiI[tc->roi_level];
				roi_m = t->roiM[tc->roi_level];
			}

			tc->roi_x = tc->x - roi_i->width / 2;
			tc->roi_y = tc->y - roi_i->height / 2;

			// assure thate the roi is within the target image
			psmove_tracker_fix_roi(tc, roi_i->width, roi_i->height, t->roiI[0]->width, t->roiI[0]->height);
		}
		cvResetImageROI(t->frame);

		if (sphere_found || roi_i->width == t->roiI[0]->width) {
			// the sphere was found, or the max ROI was reached
			break;
		} else {
			// the sphere was not found, increase the ROI
			tc->roi_x += roi_i->width / 2;
			tc->roi_y += roi_i->height / 2;

			tc->roi_level = tc->roi_level - 1;
			// update easy accessors
			roi_i = t->roiI[tc->roi_level];
			roi_m = t->roiM[tc->roi_level];

			tc->roi_x -= roi_i->width / 2;
			tc->roi_y -= roi_i->height / 2;

			// assure that the roi is within the target image
			psmove_tracker_fix_roi(tc, roi_i->width, roi_i->height, t->roiI[0]->width, t->roiI[0]->height);
		}
	}
	return sphere_found;
}

CvMemStorage* fmc_Storage;
int psmove_tracker_update_controller2(PSMoveTracker *tracker, TrackedController* tc) {
	PSMoveTracker* t = tracker;
	int i = 0;
	int sphere_found = 0;

	// calculate upper & lower bounds for the color filter
	CvScalar min, max;
	th_minus(tc->eColorHSV.val, t->rHSV.val, min.val, 3);
	th_plus(tc->eColorHSV.val, t->rHSV.val, max.val, 3);
	// this is the tracking algo
	while (1) {
		IplImage *roi_i = t->roiI[tc->roi_level];
		IplImage *roi_m = t->roiM[tc->roi_level];

		// cut out the roi!
		cvSetImageROI(t->frame, cvRect(tc->roi_x, tc->roi_y, roi_i->width, roi_i->height));
		cvCvtColor(t->frame, roi_i, CV_BGR2HSV);

		// apply color filter
		cvInRangeS(roi_i, min, max, roi_m);

		float sizeBest = 0;
		CvSeq* contourBest = 0x0;
		IplImage* clone = cvCloneImage(roi_m);
		psmove_tracker_biggest_contour(roi_m, &contourBest, &sizeBest);

		if (contourBest) {
			sphere_found = 1;
			CvMoments mu;
			CvRect br = cvBoundingRect(contourBest, 0);

			// restore the biggest contour
			cvSet(roi_m, th_black, 0x0);
			cvDrawContours(roi_m, contourBest, th_white, th_white, -1, CV_FILLED, 8, cvPoint(0, 0));
			cvSmooth(roi_m, roi_m, CV_GAUSSIAN, 7, 7, 0, 0);

			// calucalte image-moments to estimate the correct ROI
			cvMoments(roi_m, &mu, 0);
			CvPoint p = cvPoint(mu.m10 / mu.m00, mu.m01 / mu.m00);
			tc->x = p.x + tc->roi_x;
			tc->y = p.y + tc->roi_y;
			tc->r = th_max(br.width,br.height) / 2;

			// DEBUG ONLY
			cvCircle(t->frame, p, tc->r, th_green, 1, 8, 0);

			cvSmooth(roi_m, roi_m, CV_GAUSSIAN, 5, 5, 0, 0);
			th_create_mem_storage(&fmc_Storage, 0);

			CvSeq* results = cvHoughCircles(roi_m, fmc_Storage, CV_HOUGH_GRADIENT, 2, roi_m->width / 2, 10, 20, tc->r * 1.0, tc->r * 2.5);
			for (i = 0; i < results->total; i++) {
				float* p = (float*) cvGetSeqElem(results, i);
				CvPoint pt = cvPoint(cvRound(p[0]), cvRound(p[1]));
				cvCircle(t->frame, pt, cvRound(p[2]), th_red, 1, 8, 0);
			}

			/*
			 IplImage* clone = cvCloneImage(roi_m);
			 cvSet(clone, th_black, 0);
			 cvCircle(clone, cvPoint(p.x + br.x, p.y + br.y), tc->r, th_white, CV_FILLED, 8, 0);
			 if (tc->r > 20) {
			 // use adaptive color detection
			 // remove outliers
			 // estimate new color
			 CvScalar newColor = cvAvg(t->frame, clone);
			 th_plus(tc->eColor.val, newColor.val, tc->eColor.val, 3);
			 th_mul(tc->eColor.val, 0.5, tc->eColor.val, 3);
			 tc->eColorHSV = th_brg2hsv(tc->eColor);
			 }
			 cvNot(clone, clone);

			 cvSet(roi_m, th_black, clone);
			 cvReleaseImage(&clone);
			 */

			// update the future roi box
			br.width = th_max(br.width, br.height) * 2;
			br.height = br.width;
			for (i = 0; i < ROIS; i++) {
				if (br.width > t->roiI[i]->width && br.height > t->roiI[i]->height)
					break;

				tc->roi_level = i;
				// update easy accessors
				roi_i = t->roiI[tc->roi_level];
				roi_m = t->roiM[tc->roi_level];
			}

			tc->roi_x = tc->x - roi_i->width / 2;
			tc->roi_y = tc->y - roi_i->height / 2;

			// assure that the roi is within the target image
			psmove_tracker_fix_roi(tc, roi_i->width, roi_i->height, t->roiI[0]->width, t->roiI[0]->height);
		}
		cvResetImageROI(t->frame);

		if (sphere_found || roi_i->width == t->roiI[0]->width) {
			// the sphere was found, or the max ROI was reached
			break;
		} else {
			// the sphere was not found, increase the ROI
			tc->roi_x += roi_i->width / 2;
			tc->roi_y += roi_i->height / 2;

			tc->roi_level = tc->roi_level - 1;
			// update easy accessors
			roi_i = t->roiI[tc->roi_level];
			roi_m = t->roiM[tc->roi_level];

			tc->roi_x -= roi_i->width / 2;
			tc->roi_y -= roi_i->height / 2;

			// assure that the roi is within the target image
			psmove_tracker_fix_roi(tc, roi_i->width, roi_i->height, t->roiI[0]->width, t->roiI[0]->height);
		}
	}
	return sphere_found;
}

int psmove_tracker_update(PSMoveTracker *tracker, PSMove *move) {
	TrackedController* tc = 0x0;
	int spheres_found = 0;
	int UPDATE_ALL_CONTROLLERS = move == 0x0;
	hp_timer_start(tracker->timer);

	if (UPDATE_ALL_CONTROLLERS) {
		// iterate trough all controllers and find their lit spheres
		tc = tracker->controllers;
		for (; tc != 0x0 && tracker->frame; tc = tc->next) {
			spheres_found += psmove_tracker_update_controller2(tracker, tc);
		}
	} else {
		// find just that specific controller
		tc = tracked_controller_find(tracker->controllers, move);
		if (tracker->frame && tc) {
			spheres_found = psmove_tracker_update_controller2(tracker, tc);
		}
	}
	hp_timer_stop(tracker->timer);

	// draw all/one controller information to camera image
	if (UPDATE_ALL_CONTROLLERS) {
		tc = tracker->controllers;
		for (; tc != 0x0 && tracker->frame; tc = tc->next) {
			psmove_tracker_draw_tracking_stats(tracker, tc);
		}
	} else {
		tc = tracked_controller_find(tracker->controllers, move);
		if (tracker->frame && tc) {
			psmove_tracker_draw_tracking_stats(tracker, tc);
		}
	}

	// return the number of spheres found
	return spheres_found;

}

int psmove_tracker_get_position(PSMoveTracker *tracker, PSMove *move, int *x, int *y, int *radius) {
	TrackedController* tc = tracked_controller_find(tracker->controllers, move);
	if (tc != 0x0) {
		if (x != 0x0)
			*x = tc->x;

		if (y != 0x0)
			*y = tc->y;

		if (radius != 0x0)
			*radius = tc->r;
		// TODO: return age of tracking values (if possible)
	}
	return 1;
}

void psmove_tracker_free(PSMoveTracker *tracker) {
	// TODO: free memory
}

// [PRIVATE][implementation] internal functions used for the tracker
int psmove_tracker_adapt_to_light(PSMoveTracker *tracker, int lumMin, int expMin, int expMax) {
	int exp = expMin;
	// set the camera parameters to minimal exposure
	camera_control_set_parameters(tracker->cc, 0, 0, 0, exp, 0, 0xffff, 0xffff, 0xffff, -1, -1);
	IplImage* frame;
	// calculate a stepsize to increase the exposure, so that not more than 5 steps are neccessary
	int step = (expMax - expMin) / 10;
	if (step == 0)
		step = 1;
	int lastExp = exp;
	while (1) {
		// wait a little for the new parameters to be applied
		usleep(1000000 / 10);
		frame = camera_control_query_frame(tracker->cc);
		//}
		if (!frame)
			continue;

		// calculate the average color
		CvScalar avgColor = cvAvg(frame, 0x0);
		// calculate the average luminance (energy)
		float avgLum = th_avg(avgColor.val, 3);

		printf("exp:%d: lum:%f\n", exp, avgLum);
		// if the minimal luminance "limMin" has not been reached, increase the current exposure "exp"
		if (avgLum < lumMin)
			exp = exp + step;

		// check exposure boundaries
		if (exp < expMin)
			exp = expMin;
		if (exp > expMax)
			exp = expMax;

		// if the current exposure has been modified, apply it!
		if (lastExp != exp) {
			// reconfigure the camera
			camera_control_set_parameters(tracker->cc, 0, 0, 0, exp, 0, 0xffff, 0xffff, 0xffff, -1, -1);
			lastExp = exp;
		} else
			break;
	}
	printf("exposure set to %d(0x%x)\n", exp, exp);
	return exp;
}

void psmove_tracker_get_diff(PSMoveTracker* tracker, PSMove* move, int r, int g, int b, IplImage* on, IplImage* diff, int delay) {
	int elapsedTime = 0;
	int step = 10;
	// the time to wait for the controller to set the color up
	IplImage* frame;
	// switch the LEDs ON and wait for the sphere to be fully lit
	psmove_set_leds(move, r*DIMMING_FACTOR, g*DIMMING_FACTOR, b*DIMMING_FACTOR);
	psmove_update_leds(move);

	// take the first frame (sphere lit)
	while (1) {
		usleep(1000 * step);
		frame = camera_control_query_frame(tracker->cc);
		// break if delay has been reached
		if (elapsedTime >= delay)
			break;
		elapsedTime += step;
	}
	cvCopy(frame, on, 0x0);

	// switch the LEDs OFF and wait for the sphere to be off
	psmove_set_leds(move, 0, 0, 0);
	psmove_update_leds(move);

	// take the second frame (sphere iff)
	while (1) {
		usleep(1000 * step);
		frame = camera_control_query_frame(tracker->cc);
		// break if delay has been reached
		if (elapsedTime >= delay * 2)
			break;
		elapsedTime += step;
	}
	// convert both to grayscale images
	IplImage* grey1 = cvCloneImage(diff);
	IplImage* grey2 = cvCloneImage(diff);
	cvCvtColor(frame, grey1, CV_BGR2GRAY);
	cvCvtColor(on, grey2, CV_BGR2GRAY);

	// calculate the diff of to images and save it in "diff"
	cvAbsDiff(grey1, grey2, diff);

	// clean up
	cvReleaseImage(&grey1);
	cvReleaseImage(&grey2);
}

void psmove_tracker_fix_roi(TrackedController* tc, int roi_width, int roi_height, int cam_width, int cam_height) {
	if (tc->roi_x < 0)
		tc->roi_x = 0;
	if (tc->roi_y < 0)
		tc->roi_y = 0;

	if (tc->roi_x + roi_width > cam_width)
		tc->roi_x = cam_width - roi_width;
	if (tc->roi_y + roi_height > cam_height)
		tc->roi_y = cam_height - roi_height;
}

void psmove_tracker_prepare_colors(PSMoveTracker* tracker) {
	// create MAGENTA (good tracking)
	tracked_color_insert(&tracker->available_colors, 0xff, 0x00, 0xff);
	// create CYAN (good tracking)
	tracked_color_insert(&tracker->available_colors, 0x0, 0xff, 0xff);
	// create YELLOW (fair tracking)
	tracked_color_insert(&tracker->available_colors, 0xff, 0xff, 0x00);

}

void psmove_tracker_draw_tracking_stats(PSMoveTracker* tracker, TrackedController* tc) {
	CvPoint p;
	IplImage* frame = psmove_tracker_get_image(tracker);

	float textSmall = 0.8;
	float textNormal = 1;
	char text[256];
	CvScalar ic;
	CvScalar c;
	CvScalar avgC;
	float avgLum = 0;
	int roi_w = 0;
	int roi_h = 0;

	// general statistics
	avgC = cvAvg(frame, 0x0);
	avgLum = th_avg(avgC.val, 3);
	cvRectangle(frame, cvPoint(0, 0), cvPoint(frame->width, 25), th_black, CV_FILLED, 8, 0);
	sprintf(text, "fps:%.0f", tracker->debug_fps);
	th_put_text(frame, text, cvPoint(10, 20), th_white, textNormal);
	tracker->debug_fps = 0.85 * tracker->debug_fps + 0.15 * (1.0 / hp_timer_get_seconds(tracker->timer));
	sprintf(text, "avg(lum):%.0f", avgLum);
	th_put_text(frame, text, cvPoint(255, 20), th_white, textNormal);

	// controller specific statistics
	p.x = tc->x;
	p.y = tc->y;
	roi_w = tracker->roiI[tc->roi_level]->width;
	roi_h = tracker->roiI[tc->roi_level]->height;
	c = tc->eColor;
	ic = cvScalar(0xff - c.val[0], 0xff - c.val[1], 0xff - c.val[2], 0x0);

	cvRectangle(frame, cvPoint(tc->roi_x, tc->roi_y), cvPoint(tc->roi_x + roi_w, tc->roi_y + roi_h), th_white, 3, 8, 0);
	cvRectangle(frame, cvPoint(tc->roi_x, tc->roi_y), cvPoint(tc->roi_x + roi_w, tc->roi_y + roi_h), th_red, 1, 8, 0);
	cvRectangle(frame, cvPoint(tc->roi_x, tc->roi_y - 45), cvPoint(tc->roi_x + roi_w, tc->roi_y - 5), th_black, CV_FILLED, 8, 0);

	int vOff = 0;
	if (roi_h == frame->height)
		vOff = roi_h;
	sprintf(text, "RGB:%x,%x,%x", (int) c.val[2], (int) c.val[1], (int) c.val[0]);
	th_put_text(frame, text, cvPoint(tc->roi_x, tc->roi_y + vOff - 5), c, textSmall);

	sprintf(text, "ROI:%dx%d", roi_w, roi_h);
	th_put_text(frame, text, cvPoint(tc->roi_x, tc->roi_y + vOff - 15), c, textSmall);

// PS Eye uses OV7725 Chip --> http://image-sensors-world.blogspot.co.at/2010/10/omnivision-vga-sensor-inside-sony-eye.html
// http://www.ovt.com/download_document.php?type=sensor&sensorid=80
// http://photo.stackexchange.com/questions/12434/how-do-i-calculate-the-distance-of-an-object-in-a-photo
	/*
	 distance to object (mm) =   focal length (mm) * real height of the object (mm) * image height (pixels)
	 ---------------------------------------------------------------------------
	 object height (pixels) * sensor height (mm)
	 */

#define CAMERA_FOCAL_LENGTH 28.3 	// (mm)
#define PS_MOVE_DIAMETER 44 		// (mm)
#define SENSOR_PIXEL_HEIGHT 5		// (µm)
	float orb_size = tc->r * 2;
	double distance = (CAMERA_FOCAL_LENGTH * PS_MOVE_DIAMETER) / (orb_size * SENSOR_PIXEL_HEIGHT / 100.0 + FLT_EPSILON);
	sprintf(text, "radius: %.2f", tc->r);
	th_put_text(frame, text, cvPoint(tc->roi_x, tc->roi_y + vOff - 35), c, textSmall);
	sprintf(text, "dist: %.2fmm", distance);
	th_put_text(frame, text, cvPoint(tc->roi_x, tc->roi_y + vOff - 25), c, textSmall);

	//cvCircle(frame, p, 6, ic, 2, 8, 0);
	//cvCircle(frame, p, tc->r, th_green, 1, 8, 0);

	// every second save a debug-image to the filesystem
	time_t now = time(0);
	if (difftime(now, tracker->debug_last_live) > 1) {
		psmove_trace_image(frame, "livefeed", tracker->debug_last_live);
		tracker->debug_last_live = now;
	}
}

void psmove_tracker_biggest_contour(IplImage* img, CvSeq** resContour, float* resSize) {
	CvSeq* contour;
	static CvMemStorage* stor = NULL;
	if (!stor) stor = cvCreateMemStorage(0);
	*resSize = 0;
	*resContour = 0;
	cvFindContours(img, stor, &contour, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	for (; contour != 0x0; contour = contour->h_next) {
		float f = cvContourArea(contour, CV_WHOLE_SEQ, 0);
		if (f > *resSize) {
			*resSize = f;
			*resContour = contour;
		}
	}
}
