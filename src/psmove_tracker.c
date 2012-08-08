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
#include "tracker/camera_control.h"
#include "tracker/tracker_helpers.h"
#include "tracker/tracked_controller.h"
#include "tracker/tracked_color.h"
#include "tracker/tracker_trace.h"

#ifdef __linux
#  include "platform/psmove_linuxsupport.h"
#endif

#define DIMMING_FACTOR 1  // LED color dimming for use in high exposure settings
#define PRINT_DEBUG_STATS			// shall graphical statistics be printed to the image
//#define DEBUG_WINDOWS // shall additional windows be shown
#define GOOD_EXPOSURE 2051			// a very low exposure that was found to be good for tracking
#define ROIS 6                   	// the number of levels of regions of interest (roi)
#define BLINKS 4                 	// number of diff images to create during calibration
#define BLINK_DELAY 50             	// number of milliseconds to wait between a blink
#define CALIB_MIN_SIZE 50		 	// minimum size of the estimated glowing sphere during calibration process (in pixel)
#define CALIB_SIZE_STD 10	     	// maximum standard deviation (in %) of the glowing spheres found during calibration process
#define CALIB_MAX_DIST 30		 	// maximum displacement of the separate found blobs
#define COLOR_FILTER_RANGE_H 5		// +- H-Range of the hsv-colorfilter
#define COLOR_FILTER_RANGE_S 85		// +- s-Range of the hsv-colorfilter
#define COLOR_FILTER_RANGE_V 85		// +- v-Range of the hsv-colorfilter
#define CAMERA_FOCAL_LENGTH 28.3	// focal lenght constant of the ps-eye camera in (degrees)
#define CAMERA_PIXEL_HEIGHT 5		// pixel height constant of the ps-eye camera in (µm)
#define PS_MOVE_DIAMETER 47			// orb diameter constant of the ps-move controller in (mm)
/* Thresholds */
#define ROI_ADJUST_FPS_T 160		// the minimum fps to be reached, if a better roi-center adjusment is to be perfomred
#define CALIBRATION_DIFF_T 20		// during calibration, all grey values in the diff image below this value are set to black
// if tracker thresholds not met, sphere is deemed not to be found
#define TRACKER_QUALITY_T1 0.3		// minimum ratio of number of pixels in blob vs pixel of estimated circle.
#define TRACKER_QUALITY_T2 0.7		// maximum allowed change of the radius in percent, compared to the last estimated radius
#define TRACKER_QUALITY_T3 4		// minimum radius
#define TRACKER_ADAPTIVE_XY 1		// specifies to use a adaptive x/y smoothing
#define TRACKER_ADAPTIVE_Z 1		// specifies to use a adaptive z smoothing
#define COLOR_ADAPTION_QUALITY 35 	// maximal distance between the first estimated color and the newly estimated
#define COLOR_UPDATE_RATE 1	 	 	// every x seconds adapt to the color, 0 means no adaption
// if color thresholds not met, color is not adapted
#define COLOR_UPDATE_QUALITY_T1 0.8	// minimum ratio of number of pixels in blob vs pixel of estimated circle.
#define COLOR_UPDATE_QUALITY_T2 0.2	// maximum allowed change of the radius in percent, compared to the last estimated radius
#define COLOR_UPDATE_QUALITY_T3 6	// minimum radius
#ifdef WIN32
#define PSEYE_BACKUP_FILE "PSEye_backup_win.ini"
#else
#define PSEYE_BACKUP_FILE "PSEye_backup_v4l.ini"
#endif
struct _PSMoveTracker {
	CameraControl* cc;
	IplImage* frame; // the current frame of the camera
	int exposure; // the exposure to use
	IplImage* roiI[ROIS]; // array of images for each level of roi (colored)
	IplImage* roiM[ROIS]; // array of images for each level of roi (greyscale)
	IplConvKernel* kCalib; // kernel used for morphological operations during calibration
	CvScalar rHSV; // the range of the color filter
	TrackedController* controllers; // a pointer to a linked list of connected controllers
	PSMoveTrackingColor* available_colors; // a pointer to a linked list of available tracking colors
	CvMemStorage* storage; // use to store the result of cvFindContour and cvHughCircles
        long duration; // duration of tracking operation, in ms

	// internal variables
	float cam_focal_length; // in (mm)
	float cam_pixel_height; // in (µm)
	float ps_move_diameter; // in (mm)
	float user_factor_dist; // user defined factor used in distance calulation

	int tracker_adaptive_xy; // should adaptive x/y-smoothing be used
	int tracker_adaptive_z; // should adaptive z-smoothing be used

	int calibration_t;

	// if one is not met, the tracker is regarded as not found (although something has been found)
	float tracker_t1; // quality threshold1 for the tracker
	float tracker_t2; // quality threshold2 for the tracker
	float tracker_t3; // quality threshold3 for the tracker

	float adapt_t1; // quality threshold for the color adaption to discard its estimation again

	// if one is not met, the color will not use adaptive color estimation
	float color_t1; // quality threshold3 for the color adaption
	float color_t2; // quality threshold3 for the color adaption
	float color_t3; // quality threshold3 for the color adaption
	float color_update_rate; // how often shall the color be adapted (in seconds), 0 means never

	// internal variables (debug)
	float debug_fps; // the current FPS achieved by "psmove_tracker_update"
	time_t debug_last_live; // the timesamp when the last live-image was written to the file-system

};

// -------- START: internal functions only

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
int psmove_tracker_update_controller(PSMoveTracker* tracker, TrackedController* tc, float* q1, float* q2, float* q3);

/**
 * This draws tracking statistics into the current camera image. This is only used internally.
 *
 * tracker - the Tracker to use
 */
void psmove_tracker_draw_tracking_stats(PSMoveTracker* tracker);

/*
 *  This finds the biggest contour within the given image.
 *
 *  img  		- (in) 	the binary image to search for contours
 *  stor 		- (out) a storage that can be used to save the result of this function
 *  resContour 	- (out) points to the biggest contour found within the image
 *  resSize 	- (out)	the size of that contour in px²
 */
void psmove_tracker_biggest_contour(IplImage* img, CvMemStorage* stor, CvSeq** resContour, float* resSize);

/*
 * This calculates the distance of the orb of the controller.
 *
 * t 			 - (in) the PSMoveTracker to use (used to read variables)
 * blob_diameter - (in) the diameter size of the orb in pixels
 *
 * Returns: The distance between the orb and the camera in (mm).
 */
float psmove_tracker_get_distance(PSMoveTracker* t, float blob_diameter);

/*
 * This returns a subjective distance between the first estimated color and the currently estimated color.
 * Subjective, because it takes the different color components not equally into account.
 *    Result calculates lik: abs(c1.h-c2.h) + abs(c1.s-c2.s)*0.5 + abs(c1.v-c2.v)*0.5
 *
 * tc - The controller whose first/current color estimation distance should be calculated.
 *
 * Returns: a subjective distance
 */
float psmove_tracker_hsvcolor_diff(TrackedController* tc);

/*
 * This will estimate the position and the radius of the orb.
 * It will calcualte the radius by findin the two most distant points
 * in the contour. And its by choosing the mid point of those two.
 *
 * cont 	- (in) 	The contour representing the orb.
 * center 	- (out)	The X,Y-Coordinate of the contour that is calculatedhere
 * radius	- (out) The radius of the contour that is calculated here.
 */
void psmove_tracker_estimate_3d_pos(CvSeq* cont, CvPoint* center, float* radius);

/*
 * This function return a optimal ROI center point for a given Tracked controller.
 * On very fast movements, it may happen that the orb is visible in the ROI, but resides
 * at its border. This function will simply look for the biggest blob in the ROI and return a
 * point so that that blob would be in the center of the ROI.
 *
 * tc - (in) The controller whose ROI centerpoint should be adjusted.
 * t  - (in) The PSMoveTracker to use.
 *
 * Returns: a better center point for the current ROI.
 */
CvPoint psmove_tracker_better_roi_center(TrackedController* tc, PSMoveTracker* t);

int psmove_tracker_old_color_is_tracked(PSMoveTracker* t, PSMove* move, int r, int g, int b);

// -------- END: internal functions only

PSMoveTracker *psmove_tracker_new() {
    int camera = 0;

#ifdef __linux
    /**
     * On Linux, we might have multiple cameras (e.g. most laptops have
     * built-in cameras), so we try looking for the one that is handled
     * by the PSEye driver.
     **/
    camera = linux_find_pseye();
    if (camera == -1) {
        /* Could not find the PSEye - fallback to first camera */
        camera = 0;
    }
#endif

    char *camera_env = getenv(PSMOVE_TRACKER_CAMERA_ENV);
    if (camera_env) {
        char *end;
        long camera_env_id = strtol(camera_env, &end, 10);
        if (*end == '\0' && *camera_env != '\0') {
            camera = (int)camera_env_id;
#ifdef PSMOVE_DEBUG
            fprintf(stderr, "[PSMOVE] Using camera %d (%s is set)\n",
                    camera, PSMOVE_TRACKER_CAMERA_ENV);
#endif
        }
    }

    return psmove_tracker_new_with_camera(camera);
}

PSMoveTracker *
psmove_tracker_new_with_camera(int camera) {
	int i = 0;
	PSMoveTracker* t = (PSMoveTracker*) calloc(1, sizeof(PSMoveTracker));
	t->controllers = 0x0;
	t->rHSV = cvScalar(COLOR_FILTER_RANGE_H, COLOR_FILTER_RANGE_S, COLOR_FILTER_RANGE_V, 0);
        t->duration = 0;
	t->debug_fps = 0;
	t->debug_last_live = 0;
	t->storage = cvCreateMemStorage(0);

	t->cam_focal_length = CAMERA_FOCAL_LENGTH;
	t->cam_pixel_height = CAMERA_PIXEL_HEIGHT;
	t->ps_move_diameter = PS_MOVE_DIAMETER;
	t->user_factor_dist = 1.05;

	t->calibration_t = CALIBRATION_DIFF_T;
	t->tracker_t1 = TRACKER_QUALITY_T1;
	t->tracker_t2 = TRACKER_QUALITY_T2;
	t->tracker_t3 = TRACKER_QUALITY_T3;
	t->tracker_adaptive_xy = TRACKER_ADAPTIVE_XY;
	t->tracker_adaptive_z = TRACKER_ADAPTIVE_Z;
	t->adapt_t1 = COLOR_ADAPTION_QUALITY;
	t->color_t1 = COLOR_UPDATE_QUALITY_T1;
	t->color_t2 = COLOR_UPDATE_QUALITY_T2;
	t->color_t3 = COLOR_UPDATE_QUALITY_T3;
	t->color_update_rate = COLOR_UPDATE_RATE;
	// prepare available colors for tracking
	psmove_tracker_prepare_colors(t);

	// start the video capture device for tracking
	t->cc = camera_control_new(camera);
	camera_control_read_calibration(t->cc, "Intrinsics.xml", "Distortion.xml");

	// use static exposure
	t->exposure = GOOD_EXPOSURE;
	// use static adaptive exposure

	// backup the systems settings, if not already backuped
	char *filename = psmove_util_get_file_path(PSEYE_BACKUP_FILE);
	if (!th_file_exists(filename)) {
            camera_control_backup_system_settings(t->cc, filename);
            free(filename);
        }

	//t->exposure = psmove_tracker_adapt_to_light(t, 25, 2051, 4051);
	camera_control_set_parameters(t->cc, 0, 0, 0, t->exposure, 0, 0xffff, 0xffff, 0xffff, -1, -1);

	// just query a frame so that we know the camera works
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
	int b = (MIN(t->roiI[0]->height, t->roiI[0]->width) / ROIS);
	for (i = 1; i < ROIS; i++) {
		IplImage* z = t->roiI[i - 1];
		int h = b * (ROIS - i);
		t->roiI[i] = cvCreateImage(cvSize(h, h), z->depth, 3);
		t->roiM[i] = cvCreateImage(cvSize(h, h), z->depth, 1);
	}

	// prepare structure used for
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

	// looks like there is a free color -> try to calibrate/enable the controller with that color
	unsigned char r = color->r;
	unsigned char g = color->g;
	unsigned char b = color->b;

	return psmove_tracker_enable_with_color(tracker, move, r, g, b);
}

int psmove_tracker_old_color_is_tracked(PSMoveTracker* t, PSMove* move, int r, int g, int b) {
	int delay = 100;
	int result = 0;

	int nTimes = 3;

	TrackedController* tc = tracked_controller_create();
	tc->dColor = cvScalar(b, g, r, 0);
	float q1 = 0;
	float q3 = 0;
	int i = 0;
	int d = 0;
	if (tracked_controller_load_color(tc)) {

		result = 1;
		for (i = 0; i < nTimes; i++) {
			// sleep a little befor checking the next image
			for (d = 0; d < delay / 10; d++) {
				usleep(1000 * 10);
				psmove_set_leds(move,
                                        r * DIMMING_FACTOR,
                                        g * DIMMING_FACTOR,
                                        b * DIMMING_FACTOR);
				psmove_update_leds(move);
				psmove_tracker_update_image(t);
			}

			psmove_tracker_update_controller(t, tc, &q1, 0, &q3);

			// if the quality is higher than 83% and the blobs radius bigger than 8px
			result = result && q1 > 0.83 && q3 > 8;
		}
	}
	tracked_controller_release(&tc, 1);
	return result;

}

enum PSMoveTracker_Status psmove_tracker_enable_with_color(PSMoveTracker *tracker, PSMove *move, unsigned char r, unsigned char g, unsigned char b) {
	PSMoveTracker* t = tracker;
	int i;
	// check if the controller is already enabled!
	if (tracked_controller_find(tracker->controllers, move))
		return Tracker_CALIBRATED;

	// check if the color is already in use, if not, mark it as used, return with a error if it is already used
	PSMoveTrackingColor* tracked_color = tracked_color_find(tracker->available_colors, r, g, b);
	if (tracked_color == 0x0 || tracked_color->is_used)
		return Tracker_CALIBRATION_ERROR;

	// try to track the controller with the old color, if it works, immediately return1
	if (psmove_tracker_old_color_is_tracked(tracker, move, r, g, b)) {
		TrackedController* itm = tracked_controller_insert(&tracker->controllers, move);
		itm->dColor = cvScalar(b, g, r, 0);
		tracked_controller_load_color(itm);
		tracked_color->is_used = 1;
		return Tracker_CALIBRATED;
	}

	// clear the calibration html trace
	psmove_html_trace_clear();

	IplImage* frame = camera_control_query_frame(tracker->cc);
	IplImage* images[BLINKS]; // array of images saved during calibration for estimation of sphere color
	IplImage* diffs[BLINKS]; // array of masks saved during calibration for estimation of sphere color
	double sizes[BLINKS]; // array of blob sizes saved during calibration for estimation of sphere color
	for (i = 0; i < BLINKS; i++) {
		images[i] = cvCreateImage(cvGetSize(frame), frame->depth, 3);
		diffs[i] = cvCreateImage(cvGetSize(frame), frame->depth, 1);
	}
	// DEBUG log the assigned color
	CvScalar assignedColor = cvScalar(b, g, r, 0);
	psmove_html_trace_put_color_var("assignedColor", assignedColor);

	// for each blink
	for (i = 0; i < BLINKS; i++) {
		// create a diff image
		psmove_tracker_get_diff(tracker, move, r, g, b, images[i], diffs[i], BLINK_DELAY);

		// DEBUG log the diff image and the image with the lit sphere
		psmove_html_trace_image_at(images[i], i, "originals");
		psmove_html_trace_image_at(diffs[i], i, "rawdiffs");

		// threshold it to reduce image noise
		cvThreshold(diffs[i], diffs[i], t->calibration_t, 0xFF, CV_THRESH_BINARY);

		// DEBUG log the thresholded diff image
		psmove_html_trace_image_at(diffs[i], i, "threshdiffs");

		// use morphological operations to further remove noise
		cvErode(diffs[i], diffs[i], t->kCalib, 1);
		cvDilate(diffs[i], diffs[i], t->kCalib, 1);

		// DEBUG log the even more cleaned up diff-image
		psmove_html_trace_image_at(diffs[i], i, "erodediffs");
	}

	// put the diff images together to get hopefully only one intersection region
	// the region at which the controllers sphere resides.
	for (i = 1; i < BLINKS; i++) {
		cvAnd(diffs[0], diffs[i], diffs[0], 0x0);
	}

	// find the biggest contour
	float sizeBest = 0;
	CvSeq* contourBest = 0x0;
	psmove_tracker_biggest_contour(diffs[0], t->storage, &contourBest, &sizeBest);

	// blank out the image and repaint the blob where the sphere is deemed to be
	cvSet(diffs[0], th_black, 0x0);
	if (contourBest)
		cvDrawContours(diffs[0], contourBest, th_white, th_white, -1, CV_FILLED, 8, cvPoint(0, 0));

	cvClearMemStorage(t->storage);

	// DEBUG log the final diff-image used for color estimation
	psmove_html_trace_image_at(diffs[0], 0, "finaldiff");

	// CHECK if the blob contains a minimum number of pixels
	if (cvCountNonZero(diffs[0]) < CALIB_MIN_SIZE) {
		psmove_html_trace_put_log_entry("WARNING", "The final mask my not be representative for color estimation.");
	}

	// calculate the avg color
	CvScalar color = cvAvg(images[0], diffs[0]);
	CvScalar hsv_assigned = th_brg2hsv(assignedColor);
	CvScalar hsv_color = th_brg2hsv(color);

	psmove_html_trace_put_color_var("estimatedColor", color);
	psmove_html_trace_put_int_var("estimated_hue", hsv_color.val[0]);
	psmove_html_trace_put_int_var("assigned_hue", hsv_assigned.val[0]);
	psmove_html_trace_put_int_var("allowed_hue_difference", t->rHSV.val[0]);

	// CHECK if the hue of the estimated and the assigned colors differ more than allowed in the color-filter range.s
	if (abs(hsv_assigned.val[0] - hsv_color.val[0]) > t->rHSV.val[0]) {
		psmove_html_trace_put_log_entry("WARNING", "The estimated color seems not to be similar to the color it should be.");
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
		cvErode(mask, mask, t->kCalib, 1);
		cvDilate(mask, mask, t->kCalib, 1);

		// DEBUG log the color filter and
		psmove_html_trace_image_at(mask, i, "filtered");

		// find the biggest contour in the image and save its location and size
		psmove_tracker_biggest_contour(mask, t->storage, &contourBest, &sizeBest);
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

		// CHECK for errors (no contour, more than one contour, or contour too small)
		if (contourBest == 0x0) {
			psmove_html_trace_array_item_at(i, "contours", "no contour");
		} else if (sizes[i] <= CALIB_MIN_SIZE) {
			psmove_html_trace_array_item_at(i, "contours", "too small");
		} else if (dist >= CALIB_MAX_DIST) {
			psmove_html_trace_array_item_at(i, "contours", "too far apart");
		} else {
			psmove_html_trace_array_item_at(i, "contours", "OK");
			// all checks passed, increase the number of valid contours
			valid_countours++;
		}
		cvClearMemStorage(t->storage);

	}

	// clean up all temporary images
	for (i = 0; i < BLINKS; i++) {
		cvReleaseImage(&images[i]);
		cvReleaseImage(&diffs[i]);
	}

	int CHECK_HAS_ERRORS = 0;

	// CHECK if sphere was found in each BLINK image
	if (valid_countours < BLINKS) {
		psmove_html_trace_put_log_entry("ERROR", "The sphere could not be found in all images.");
		CHECK_HAS_ERRORS++;
	}

	// CHECK if the size of the found contours are similar
	double stdSizes = sqrt(th_var(sizes, BLINKS));
	if (stdSizes >= (th_avg(sizes, BLINKS) / 100.0 * CALIB_SIZE_STD)) {
		psmove_html_trace_put_log_entry("ERROR", "The spheres found differ too much in size.");
		CHECK_HAS_ERRORS++;
	}

	if (CHECK_HAS_ERRORS)
		return Tracker_CALIBRATION_ERROR;

	// insert to list of tracked controllers
	TrackedController* itm = tracked_controller_insert(&tracker->controllers, move);
	// set current color
	itm->dColor = cvScalar(b, g, r, 0);
	// set first estimated color
	itm->eFColor = color;
	itm->eFColorHSV = hsv_color;
	// set current estimated color
	itm->eColor = color;
	itm->eColorHSV = hsv_color;

	// set, that this color is in use
	tracked_color->is_used = 1;

	tracked_controller_save_colors(tracker->controllers);
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
		if (tc->is_tracked)
			return Tracker_CALIBRATED_AND_FOUND;
		else
			return Tracker_CALIBRATED_AND_NOT_FOUND;
	} else {
		return Tracker_UNCALIBRATED;
	}
}

void*
psmove_tracker_get_image(PSMoveTracker *tracker) {
	return tracker->frame;
}

void psmove_tracker_update_image(PSMoveTracker *tracker) {
	tracker->frame = camera_control_query_frame(tracker->cc);
}

int psmove_tracker_update_controller(PSMoveTracker *tracker, TrackedController* tc, float* q1, float* q2, float* q3) {
	PSMoveTracker* t = tracker;
	CvPoint c;
	int i = 0;
	int sphere_found = 0;

	// calculate upper & lower bounds for the color filter
	CvScalar min, max;
	th_minus(tc->eColorHSV.val, t->rHSV.val, min.val, 3);
	th_plus(tc->eColorHSV.val, t->rHSV.val, max.val, 3);

	// this is the tracking algorithm
	while (1) {
		// get pointers to data structures for the given ROI-Level
		IplImage *roi_i = t->roiI[tc->roi_level];
		IplImage *roi_m = t->roiM[tc->roi_level];

		// adjust the ROI, so that the blob is fully visible, but only if we have a reasonable FPS
		if (t->debug_fps > ROI_ADJUST_FPS_T) {

			CvPoint nRoiCenter = psmove_tracker_better_roi_center(tc, tracker);
			if (nRoiCenter.x != -1) {
				tc->roi_x = nRoiCenter.x;
				tc->roi_y = nRoiCenter.y;
				psmove_tracker_fix_roi(tc, roi_i->width, roi_i->height, t->roiI[0]->width, t->roiI[0]->height);
			}
		}

		// apply the ROI
		cvSetImageROI(t->frame, cvRect(tc->roi_x, tc->roi_y, roi_i->width, roi_i->height));
		cvCvtColor(t->frame, roi_i, CV_BGR2HSV);

		// apply color filter
		cvInRangeS(roi_i, min, max, roi_m);
		cvSmooth(roi_i, roi_i, CV_GAUSSIAN, 5, 5, 0, 0);

		// find the biggest contour in the image
		float sizeBest = 0;
		CvSeq* contourBest = 0x0;
		psmove_tracker_biggest_contour(roi_m, t->storage, &contourBest, &sizeBest);

		if (contourBest) {
			CvMoments mu;
			CvRect br = cvBoundingRect(contourBest, 0);

			// restore the biggest contour
			cvSet(roi_m, th_black, 0x0);
			cvDrawContours(roi_m, contourBest, th_white, th_white, -1, CV_FILLED, 8, cvPoint(0, 0));
#ifdef DEBUG_WINDOWS
			if (tc->next == 0x0)
				cvShowImage("0", roi_m);
			else
				cvShowImage("1", roi_m);
#endif
			// calucalte image-moments
			cvMoments(roi_m, &mu, 0);
			CvPoint p = cvPoint(mu.m10 / mu.m00, mu.m01 / mu.m00);
			CvPoint oldMCenter = cvPoint(tc->mx, tc->my);
			tc->mx = p.x + tc->roi_x;
			tc->my = p.y + tc->roi_y;
			CvPoint newMCenter = cvPoint(tc->mx, tc->my);

			// remember the old radius and calcutlate the new x/y position and radius of the found contour
			float oldRadius = tc->r;
			psmove_tracker_estimate_3d_pos(contourBest, &c, &tc->r);

			// apply radius-smoothing if enabled
			if (t->tracker_adaptive_z) {
				// calculate the difference between calculated radius and the smoothed radius of the past
				float rDiff = abs(tc->rs - tc->r);
				// calcualte a adaptive smoothing factor
				// a big distance leads to no smoothing, a small one to strong smoothing
				float rf = MIN(rDiff/4+0.15,1);

				// apply adaptive smoothing of the radius
				tc->rs = tc->rs * (1 - rf) + tc->r * rf;
				tc->r = tc->rs;
			}

			// apply x/y coordinate smoothing if enabled
			if (t->tracker_adaptive_z) {
				// a big distance between the old and new center of mass results in no smoothing
				// a little one to strong smoothing
				float diff = th_dist(oldMCenter, newMCenter);
				float f = MIN(diff / 7 + 0.15, 1);
				// apply adaptive smoothing
				tc->x = tc->x * (1 - f) + (c.x + tc->roi_x) * f;
				tc->y = tc->y * (1 - f) + (c.y + tc->roi_y) * f;
			} else {
				// do NOT apply adaptive smoothing
				tc->x = c.x + tc->roi_x;
				tc->y = c.y + tc->roi_y;
			}

			// calculate the quality of the tracking
			int pixelInBlob = cvCountNonZero(roi_m);
			float pixelInResult = tc->r * tc->r * th_PI;
			float tq1 = 0;
			float tq2 = FLT_MAX;
			float tq3 = tc->r;

			// The quality checks are all performed on the radius of the blob
			// its old radius and size.
			tq1 = pixelInBlob / pixelInResult;

			// always check pixelration and minimal size
			sphere_found = tq1 > t->tracker_t1 && tq3 > t->tracker_t3;

			if (tq1 > 0.85) {
				tc->x = tc->mx;
				tc->y = tc->my;
			}
			// only perform check if we already found the sphere once
			if (oldRadius > 0) {
				tq2 = abs(oldRadius - tc->r) / (oldRadius + FLT_EPSILON);

				// decrease TQ1 by half if below 20px (gives better results if controller is far away)
				if (pixelInBlob < 20)
					tq1 = tq1 * 0.5;

				// additionally check for to big changes
				sphere_found = sphere_found && tq2 < t->tracker_t2;
			}

			// save output parameters for the quality indicators
			if (q1 != 0x0)
				*q1 = tq1;
			if (q2 != 0x0)
				*q2 = tq2;
			if (q3 != 0x0)
				*q3 = tq3;

			// only if the quality is okay update the future ROI
			if (sphere_found) {

				// use adaptive color detection
				// only if 	1) the sphere has been found
				// AND		2) the UPDATE_RATE has passed
				// AND		3) the tracking-quality is high;
				int do_color_adaption = 0;
				time_t now = time(0);
				if (t->color_update_rate > 0 && difftime(now, tc->last_color_update) > t->color_update_rate)
					do_color_adaption = 1;

				if (do_color_adaption && tq1 > t->color_t1 && tq2 < t->color_t2 && tq3 > t->color_t3) {
					// calculate the new estimated color (adaptive color estimation)
					CvScalar newColor = cvAvg(t->frame, roi_m);
					th_plus(tc->eColor.val, newColor.val, tc->eColor.val, 3);
					th_mul(tc->eColor.val, 0.5, tc->eColor.val, 3);
					tc->eColorHSV = th_brg2hsv(tc->eColor);
					tc->last_color_update = now;
					// CHECK if the current estimate is too far away from its original estimation
					if (psmove_tracker_hsvcolor_diff(tc) > t->adapt_t1) {
						tc->eColor = tc->eFColor;
						tc->eColorHSV = tc->eFColorHSV;
						sphere_found = 0;
					}
				}

				// update the future roi box
				br.width = th_max(br.width, br.height) * 3;
				br.height = br.width;
				// find a suitable ROI level
				for (i = 0; i < ROIS; i++) {
					if (br.width > t->roiI[i]->width && br.height > t->roiI[i]->height)
						break;
					tc->roi_level = i;
					// update easy accessors
					roi_i = t->roiI[tc->roi_level];
					roi_m = t->roiM[tc->roi_level];
				}

				// adjust the roi variables accordingly
				float dx = tc->roi_x - (tc->x - roi_i->width / 2);
				float dy = tc->roi_y - (tc->y - roi_i->height / 2);
				if (dx < -1 || dx > 1)
					tc->roi_x = tc->x - roi_i->width / 2;
				if (dy < -1 || dy > 1)
					tc->roi_y = tc->y - roi_i->height / 2;

				// assure that the roi is within the target image
				psmove_tracker_fix_roi(tc, roi_i->width, roi_i->height, t->roiI[0]->width, t->roiI[0]->height);
			}
		}
		cvClearMemStorage(t->storage);
		cvResetImageROI(t->frame);

		if (sphere_found || roi_i->width == t->roiI[0]->width) {
			// the sphere was found, or the max ROI was reached
			break;
		} else {
			// the sphere was not found, increase the ROI and search again!
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

	// remember if the sphere was found
	tc->is_tracked = sphere_found;
	return sphere_found;
}

int psmove_tracker_update(PSMoveTracker *tracker, PSMove *move) {
	TrackedController* tc = 0x0;
	int spheres_found = 0;
	int UPDATE_ALL_CONTROLLERS = move == 0x0;

        // FPS calculation
        long started = psmove_util_get_ticks();
	if (UPDATE_ALL_CONTROLLERS) {
		// iterate trough all controllers and find their lit spheres
		tc = tracker->controllers;
		for (; tc != 0x0 && tracker->frame; tc = tc->next) {
			spheres_found += psmove_tracker_update_controller(tracker, tc, 0, 0, 0);
		}
	} else {
		// find just that specific controller
		tc = tracked_controller_find(tracker->controllers, move);
		if (tracker->frame && tc) {
			spheres_found = psmove_tracker_update_controller(tracker, tc, 0, 0, 0);
		}
	}
        tracker->duration = (psmove_util_get_ticks() - started);

	// draw all/one controller information to camera image
#ifdef PRINT_DEBUG_STATS
	psmove_tracker_draw_tracking_stats(tracker);
#endif
	// return the number of spheres found
	return spheres_found;

}

int psmove_tracker_get_position(PSMoveTracker *tracker, PSMove *move, float *x, float *y, float *radius) {
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
	tracked_controller_save_colors(tracker->controllers);

	char *filename = psmove_util_get_file_path(PSEYE_BACKUP_FILE);
	if (th_file_exists(filename)) {
            camera_control_restore_system_settings(tracker->cc, filename);
            free(filename);
        }
	cvReleaseMemStorage(&tracker->storage);
	int i = 0;
	for (; i < ROIS; i++) {
		cvReleaseImage(&tracker->roiM[0]);
		cvReleaseImage(&tracker->roiI[0]);
	}
	cvReleaseStructuringElement(&tracker->kCalib);
	tracked_controller_release(&tracker->controllers, 1);
	tracked_color_release(&tracker->available_colors, 1);

    camera_control_delete(tracker->cc);
    // FIXME: Free tracker itself!
}

// -------- Implementation: internal functions only
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
        r *= DIMMING_FACTOR;
        g *= DIMMING_FACTOR;
        b *= DIMMING_FACTOR;
	psmove_set_leds(move, r, g, b);
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
	tracked_color_insert(&tracker->available_colors, 0x00, 0xff, 0xff);
	// create YELLOW (fair tracking)
	tracked_color_insert(&tracker->available_colors, 0xff, 0xff, 0x00);

}

void psmove_tracker_draw_tracking_stats(PSMoveTracker* tracker) {
	CvPoint p;
	IplImage* frame = psmove_tracker_get_image(tracker);

	float textSmall = 0.8;
	float textNormal = 1;
	char text[256];
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
        if (tracker->duration) {
            tracker->debug_fps = (0.85 * tracker->debug_fps + 0.15 *
                (1000. / (double)tracker->duration));
        }
	sprintf(text, "avg(lum):%.0f", avgLum);
	th_put_text(frame, text, cvPoint(255, 20), th_white, textNormal);

	TrackedController* tc;
	// draw all/one controller information to camera image
	tc = tracker->controllers;
	for (; tc != 0x0 && tracker->frame; tc = tc->next) {
		if (tc->is_tracked) {
			// controller specific statistics
			p.x = tc->x;
			p.y = tc->y;
			roi_w = tracker->roiI[tc->roi_level]->width;
			roi_h = tracker->roiI[tc->roi_level]->height;
			c = tc->eColor;

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

			double distance = psmove_tracker_get_distance(tracker, tc->r * 2);

			sprintf(text, "radius: %.2f", tc->r);
			th_put_text(frame, text, cvPoint(tc->roi_x, tc->roi_y + vOff - 35), c, textSmall);
			sprintf(text, "dist: %.2fmm", distance);
			th_put_text(frame, text, cvPoint(tc->roi_x, tc->roi_y + vOff - 25), c, textSmall);

			cvCircle(frame, p, tc->r, th_white, 1, 8, 0);
		}
	}

	// every second save a debug-image to the filesystem
	time_t now = time(0);
	if (difftime(now, tracker->debug_last_live) > 1) {
		psmove_html_trace_image(frame, "livefeed", tracker->debug_last_live);
		tracker->debug_last_live = now;
	}
}

float psmove_tracker_hsvcolor_diff(TrackedController* tc) {
	float diff = 0;
	diff += abs(tc->eFColorHSV.val[0] - tc->eColorHSV.val[0]) * 1; // diff of HUE is very important
	diff += abs(tc->eFColorHSV.val[1] - tc->eColorHSV.val[1]) * 0.5; // saturation and value not so much
	diff += abs(tc->eFColorHSV.val[2] - tc->eColorHSV.val[2]) * 0.5;
	return diff;
}

float psmove_tracker_get_distance(PSMoveTracker* t, float blob_diameter) {

	// PS Eye uses OV7725 Chip --> http://image-sensors-world.blogspot.co.at/2010/10/omnivision-vga-sensor-inside-sony-eye.html
	// http://www.ovt.com/download_document.php?type=sensor&sensorid=80
	// http://photo.stackexchange.com/questions/12434/how-do-i-calculate-the-distance-of-an-object-in-a-photo
	/*
	 distance to object (mm) =   focal length (mm) * real height of the object (mm) * image height (pixels)
	 ---------------------------------------------------------------------------
	 object height (pixels) * sensor height (mm)
	 */

	return (t->cam_focal_length * t->ps_move_diameter * t->user_factor_dist) / (blob_diameter * t->cam_pixel_height / 100.0 + FLT_EPSILON);
}

void psmove_tracker_biggest_contour(IplImage* img, CvMemStorage* stor, CvSeq** resContour, float* resSize) {
	CvSeq* contour;
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

void psmove_tracker_estimate_3d_pos(CvSeq* cont, CvPoint* center, float* radius) {
	int i, j;
	float d = 0;
	float cd = 0;
	CvPoint m1;
	CvPoint m2;
	CvPoint2D32f m;
	CvPoint * p1;
	CvPoint * p2;

	int step = MAX(1,cont->total/20);

	// compare every two points of the contour (but not more than 20)
	// to find the most distant pair
	for (i = 0; i < cont->total; i += step) {
		p1 = (CvPoint*) cvGetSeqElem(cont, i);
		for (j = i + 1; j < cont->total; j += step) {
			p2 = (CvPoint*) cvGetSeqElem(cont, j);
			cd = th_dist_squared(*p1,*p2);
			if (cd > d) {
				d = cd;
				m1 = *p1;
				m2 = *p2;
			}
		}
	}
	// calculate center of that pair
	m.x = 0.5 * (m1.x + m2.x);
	m.y = 0.5 * (m1.y + m2.y);
	//cvLine(img, m1[s], m2[s], th_yellow, 1, 8, 0);
	center->x = (int) (m.x + 0.5);
	center->y = (int) (m.y + 0.5);
	// calcualte the radius
	*radius = sqrt(d) / 2;
}

CvPoint psmove_tracker_better_roi_center(TrackedController* tc, PSMoveTracker* t) {
	CvPoint erg = cvPoint(-1, -1);
	CvScalar min, max;
	th_minus(tc->eColorHSV.val, t->rHSV.val, min.val, 3);
	th_plus(tc->eColorHSV.val, t->rHSV.val, max.val, 3);

	IplImage *roi_i = t->roiI[tc->roi_level];
	IplImage *roi_m = t->roiM[tc->roi_level];

	// cut out the roi!
	cvSetImageROI(t->frame, cvRect(tc->roi_x, tc->roi_y, roi_i->width, roi_i->height));
	cvCvtColor(t->frame, roi_i, CV_BGR2HSV);

	// apply color filter
	cvInRangeS(roi_i, min, max, roi_m);
	cvSmooth(roi_i, roi_i, CV_GAUSSIAN, 5, 5, 0, 0);

	float sizeBest = 0;
	CvSeq* contourBest = 0x0;
	psmove_tracker_biggest_contour(roi_m, t->storage, &contourBest, &sizeBest);
	if (contourBest) {
		cvSet(roi_m, th_black, 0x0);
		cvDrawContours(roi_m, contourBest, th_white, th_white, -1, CV_FILLED, 8, cvPoint(0, 0));
		// calucalte image-moments to estimate the better ROI center
		CvMoments mu;
		cvMoments(roi_m, &mu, 0);

		erg = cvPoint(mu.m10 / mu.m00, mu.m01 / mu.m00);
		erg.x = erg.x + tc->roi_x - roi_m->width / 2;
		erg.y = erg.y + tc->roi_y - roi_m->height / 2;
	}
	cvClearMemStorage(t->storage);
	cvResetImageROI(t->frame);
	return erg;
}
