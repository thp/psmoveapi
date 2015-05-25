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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#include "opencv2/core/core_c.h"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/highgui/highgui_c.h"

#include "psmove_tracker.h"
#include "../psmove_private.h"
#include "psmove_tracker_private.h"

#include "camera_control.h"
#include "tracker_helpers.h"
#include "tracker_trace.h"

#ifdef __linux
#  include "platform/psmove_linuxsupport.h"
#endif

//#define DEBUG_WINDOWS 			// shall additional windows be shown
#define ROIS 4                   	// the number of levels of regions of interest (roi)
#define BLINKS 2                 	// number of diff images to create during calibration
#define BLINK_DELAY 200             	// number of milliseconds to wait between a blink
#define CALIB_MIN_SIZE 50		 	// minimum size of the estimated glowing sphere during calibration process (in pixel)
#define CALIB_SIZE_STD 10	     	// maximum standard deviation (in %) of the glowing spheres found during calibration process
#define CALIB_MAX_DIST 30		 	// maximum displacement of the separate found blobs
#define COLOR_FILTER_RANGE_H 20		// +- H-Range of the hsv-colorfilter
#define COLOR_FILTER_RANGE_S 85		// +- s-Range of the hsv-colorfilter
#define COLOR_FILTER_RANGE_V 85		// +- v-Range of the hsv-colorfilter

/* Thresholds */
#define ROI_ADJUST_FPS_T 160		// the minimum fps to be reached, if a better roi-center adjusment is to be perfomred
#define CALIBRATION_DIFF_T 20		// during calibration, all grey values in the diff image below this value are set to black
// if tracker thresholds not met, sphere is deemed not to be found
#define TRACKER_QUALITY_T1 0.3		// minimum ratio of number of pixels in blob vs pixel of estimated circle.
#define TRACKER_QUALITY_T2 0.7		// maximum allowed change of the radius in percent, compared to the last estimated radius
#define TRACKER_QUALITY_T3 4		// minimum radius
#define TRACKER_ADAPTIVE_XY 1		// specifies to use a adaptive x/y smoothing
#define TRACKER_ADAPTIVE_Z 1		// specifies to use a adaptive z smoothing
#define COLOR_ADAPTION_QUALITY 35 	// maximal distance (calculated by 'psmove_tracker_hsvcolor_diff') between the first estimated color and the newly estimated
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

#define INTRINSICS_XML "intrinsics.xml"
#define DISTORTION_XML "distortion.xml"

#define COLOR_MAPPING_DAT "colormapping.dat"

/* Only re-use color mappings "younger" than 2 hours */
#define COLOR_MAPPING_MAX_AGE (2*60*60)

/**
 * Syntactic sugar - iterate over all valid controllers of a tracker
 *
 * Usage example:
 *
 *    TrackedController *tc;
 *    for_each_controller (tracker, tc) {
 *        // do something with "tc" here
 *    }
 *
 **/
#define for_each_controller(tracker, var) \
    for (var=tracker->controllers; var<tracker->controllers+PSMOVE_TRACKER_MAX_CONTROLLERS; var++) \
        if (var->move)

struct _TrackedController {
    /* Move controller, or NULL if free slot */
    PSMove* move;

    /* Assigned RGB color of the controller */
    struct PSMove_RGBValue color;

    CvScalar eFColor;			// first estimated color (BGR)
    CvScalar eFColorHSV;		// first estimated color (HSV)

    CvScalar eColor;			// estimated color (BGR)
    CvScalar eColorHSV; 		// estimated color (HSV)

    int roi_x, roi_y;			// x/y - Coordinates of the ROI
    int roi_level; 	 			// the current index for the level of ROI
    enum PSMove_Bool roi_level_fixed;    // true if the ROI level should be fixed
    float mx, my;				// x/y - Coordinates of center of mass of the blob
    float x, y, r;				// x/y - Coordinates of the controllers sphere and its radius
    int search_tile; 			// current search quadrant when controller is not found (reset to 0 if found)
    float rs;					// a smoothed variant of the radius

    float q1, q2, q3; // Calculated quality criteria from the tracker

    int is_tracked;				// 1 if tracked 0 otherwise
    long last_color_update;	// the timestamp when the last color adaption has been performed
    enum PSMove_Bool auto_update_leds;
};

typedef struct _TrackedController TrackedController;


/* Has to be 256, so that next_slot automatically wraps */
#define COLOR_MAPPING_RING_BUFFER_SIZE 256

/**
 * A ring buffer used for saving color mappings of previous sessions. There
 * is a pointer "next_slot" that will point to the next free slot. From there,
 * new values can be saved (forward) and old values can be searched (backward).
 **/
struct ColorMappingRingBuffer {
    struct {
        /* The RGB LED value */
        struct PSMove_RGBValue from;

        /* The dimming factor for which this mapping is valid */
        unsigned char dimming;

        /* The value of the controller in the camera */
        struct PSMove_RGBValue to;
    } map[COLOR_MAPPING_RING_BUFFER_SIZE];
    unsigned char next_slot;
};

/**
 * Parameters of the Pearson type VII distribution
 * Source: http://fityk.nieto.pl/model.html
 * Used for calculating the distance from the radius
 **/
struct PSMoveTracker_DistanceParameters {
    float height;
    float center;
    float hwhm;
    float shape;
};

/**
 * Experimentally-determined parameters for a PS Eye camera
 * in wide angle mode with a PS Move, color = (255, 0, 255)
 **/
static struct PSMoveTracker_DistanceParameters
pseye_distance_parameters = {
    /* height = */ 517.281,
    /* center = */ 1.297338,
    /* hwhm = */ 3.752844,
    /* shape = */ 0.4762335,
};

struct _PSMoveTracker {
	CameraControl* cc;

        /* Timestamps for performance measurements */
        PSMove_timestamp ts_camera_begin; // when the capture was started
        PSMove_timestamp ts_camera_grab; // when the image was grabbed
        PSMove_timestamp ts_camera_retrieve; // when the image was retrieved
        PSMove_timestamp ts_camera_converted; // when the image was converted

	IplImage* frame; // the current frame of the camera
        IplImage *frame_rgb; // the frame as tightly packed RGB data
	int exposure; // the exposure to use
	IplImage* roiI[ROIS]; // array of images for each level of roi (colored)
	IplImage* roiM[ROIS]; // array of images for each level of roi (greyscale)
	IplConvKernel* kCalib; // kernel used for morphological operations during calibration
	CvScalar rHSV; // the range of the color filter

        // Parameters for psmove_tracker_distance_from_radius()
        struct PSMoveTracker_DistanceParameters distance_parameters;

        enum PSMoveTracker_Exposure exposure_mode; // exposure mode
        float dimming_factor; // dimming factor used on LED RGB values

	TrackedController controllers[PSMOVE_TRACKER_MAX_CONTROLLERS]; // controller data

        struct ColorMappingRingBuffer color_mapping; // remembered color mappings

	CvMemStorage* storage; // use to store the result of cvFindContour and cvHughCircles
        long duration; // duration of tracking operation, in ms

        // size of "search" tiles when tracking is lost
        int search_tile_width; // width of a single tile
        int search_tile_height; // height of a single tile
        int search_tiles_horizontal; // number of search tiles per row
        int search_tiles_count; // number of search tiles

	// internal variables
	int tracker_adaptive_xy; // should adaptive x/y-smoothing be used
	int tracker_adaptive_z; // should adaptive z-smoothing be used

	int calibration_t; // the threshold used during calibration to create the diff image

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

        enum PSMove_Bool mirror; // mirror camera image horizontally
};

// -------- START: internal functions only

/**
 * Adapts the cameras exposure to the current lighting conditions
 *
 * This function will find the most suitable exposure.
 *
 * tracker - A valid PSMoveTracker * instance
 * target_luminance - The target luminance value (higher = brighter)
 *
 * Returns: the most suitable exposure
 **/
int psmove_tracker_adapt_to_light(PSMoveTracker *tracker, float target_luminance);

/**
 * Find the TrackedController * for a given PSMove * instance
 *
 * if move == NULL, the next free slot will be returned
 *
 * Returns the TrackedController * instance, or NULL if not found
 **/
TrackedController *
psmove_tracker_find_controller(PSMoveTracker *tracker, PSMove *move);


/**
 * Wait for a given time for a frame from the tracker
 *
 * tracker - A valid PSMoveTracker * instance
 * frame - A pointer to an IplImage * to store the frame
 * delay - The delay to wait for the frame
 **/
void
psmove_tracker_wait_for_frame(PSMoveTracker *tracker, IplImage **frame, int delay);

/**
 * This function switches the sphere of the given PSMove on to the given color and takes
 * a picture via the given capture. Then it switches it of and takes a picture again. A difference image
 * is calculated from these two images. It stores the image of the lit sphere and
 * of the diff-image in the passed parameter "on" and "diff". Before taking
 * a picture it waits for the specified delay (in microseconds).
 *
 * tracker - the tracker that contains the camera control
 * move    - the PSMove controller to use
 * rgb     - the RGB color to use to lit the sphere
 * on	   - the pre-allocated image to store the captured image when the sphere is lit
 * diff    - the pre-allocated image to store the calculated diff-image
 * delay   - the time to wait before taking a picture (in microseconds)
 **/
void
psmove_tracker_get_diff(PSMoveTracker* tracker, PSMove* move,
        struct PSMove_RGBValue rgb, IplImage* on, IplImage* diff, int delay,
        float dimming_factor);

/**
 * This function seths the rectangle of the ROI and assures that the itis always within the bounds
 * of the camera image.
 *
 * tracker          - A valid PSMoveTracker * instance
 * tc         - The TrackableController containing the roi to check & fix
 * roi_x	  - the x-part of the coordinate of the roi
 * roi_y	  - the y-part of the coordinate of the roi
 * roi_width  - the width of the roi
 * roi_height - the height of the roi
 * cam_width  - the width of the camera image
 * cam_height - the height of the camera image
 **/
void psmove_tracker_set_roi(PSMoveTracker* tracker, TrackedController* tc, int roi_x, int roi_y, int roi_width, int roi_height);

/**
 * This function is just the internal implementation of "psmove_tracker_update"
 */
int psmove_tracker_update_controller(PSMoveTracker* tracker, TrackedController *tc);

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
 * This returns a subjective distance between the first estimated (during calibration process) color and the currently estimated color.
 * Subjective, because it takes the different color components not equally into account.
 *    Result calculates like: abs(c1.h-c2.h) + abs(c1.s-c2.s)*0.5 + abs(c1.v-c2.v)*0.5
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
 * x            - (out) The X coordinate of the center.
 * y            - (out) The Y coordinate of the center.
 * radius	- (out) The radius of the contour that is calculated here.
 */
void
psmove_tracker_estimate_circle_from_contour(CvSeq* cont, float *x, float *y, float* radius);

/*
 * This function return a optimal ROI center point for a given Tracked controller.
 * On very fast movements, it may happen that the orb is visible in the ROI, but resides
 * at its border. This function will simply look for the biggest blob in the ROI and return a
 * point so that that blob would be in the center of the ROI.
 *
 * tc - (in) The controller whose ROI centerpoint should be adjusted.
 * tracker  - (in) The PSMoveTracker to use.
 * center - (out) The better center point for the current ROI
 *
 * Returns: nonzero if a new point was found, zero otherwise
 */
int
psmove_tracker_center_roi_on_controller(TrackedController* tc, PSMoveTracker* tracker, CvPoint *center);

int
psmove_tracker_color_is_used(PSMoveTracker *tracker, struct PSMove_RGBValue color);

enum PSMoveTracker_Status
psmove_tracker_enable_with_color_internal(PSMoveTracker *tracker, PSMove *move, struct PSMove_RGBValue color);

/*
 * This function reads old calibration color values and tries to track the controller with that color.
 * if it works, the function returns 1, 0 otherwise.
 * Can help to speed up calibration process on application startup.
 *
 * tracker     - (in) A valid PSMoveTracker
 * move  - (in) A valid PSMove controller
 * rgb - (in) The color the PSMove controller's sphere will be lit.
 */

int
psmove_tracker_old_color_is_tracked(PSMoveTracker* tracker, PSMove* move, struct PSMove_RGBValue rgb);

/**
 * Lookup a camera-visible color value
 **/
int
psmove_tracker_lookup_color(PSMoveTracker *tracker, struct PSMove_RGBValue rgb, CvScalar *color);

/**
 * Remember a color value after calibration
 **/
void
psmove_tracker_remember_color(PSMoveTracker *tracker, struct PSMove_RGBValue rgb, CvScalar color);

// -------- END: internal functions only

PSMoveTracker *psmove_tracker_new() {
    int camera = 0;

#if defined(__linux) && defined(PSMOVE_USE_PSEYE)
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

    int camera_env = psmove_util_get_env_int(PSMOVE_TRACKER_CAMERA_ENV);
    if (camera_env != -1) {
        camera = camera_env;
        psmove_DEBUG("Using camera %d (%s is set)\n", camera,
                PSMOVE_TRACKER_CAMERA_ENV);
    }

    return psmove_tracker_new_with_camera(camera);
}

void
psmove_tracker_set_auto_update_leds(PSMoveTracker *tracker, PSMove *move,
        enum PSMove_Bool auto_update_leds)
{
    psmove_return_if_fail(tracker != NULL);
    psmove_return_if_fail(move != NULL);
    TrackedController *tc = psmove_tracker_find_controller(tracker, move);
    psmove_return_if_fail(tc != NULL);
    tc->auto_update_leds = auto_update_leds;
}


enum PSMove_Bool
psmove_tracker_get_auto_update_leds(PSMoveTracker *tracker, PSMove *move)
{
    psmove_return_val_if_fail(tracker != NULL, PSMove_False);
    psmove_return_val_if_fail(move != NULL, PSMove_False);

    TrackedController *tc = psmove_tracker_find_controller(tracker, move);
    psmove_return_val_if_fail(tc != NULL, PSMove_False);
    return tc->auto_update_leds;
}


void
psmove_tracker_set_dimming(PSMoveTracker *tracker, float dimming)
{
    psmove_return_if_fail(tracker != NULL);
    tracker->dimming_factor = dimming;
}

float
psmove_tracker_get_dimming(PSMoveTracker *tracker)
{
    psmove_return_val_if_fail(tracker != NULL, 0);
    return tracker->dimming_factor;
}

void
psmove_tracker_set_exposure(PSMoveTracker *tracker,
        enum PSMoveTracker_Exposure exposure)
{
    psmove_return_if_fail(tracker != NULL);
    tracker->exposure_mode = exposure;

    float target_luminance = 0;
    switch (tracker->exposure_mode) {
        case Exposure_LOW:
            target_luminance = 0;
            break;
        case Exposure_MEDIUM:
            target_luminance = 25;
            break;
        case Exposure_HIGH:
            target_luminance = 50;
            break;
        default:
            psmove_DEBUG("Invalid exposure mode: %d\n", exposure);
            break;
    }

    #if defined(__APPLE__) && !defined(CAMERA_CONTROL_USE_PS3EYE_DRIVER)
        camera_control_initialize();
    #endif

    tracker->exposure = psmove_tracker_adapt_to_light(tracker, target_luminance);
    camera_control_set_parameters(tracker->cc, 0, 0, 0, tracker->exposure,
            0, 0xffff, 0xffff, 0xffff, -1, -1);
}

enum PSMoveTracker_Exposure
psmove_tracker_get_exposure(PSMoveTracker *tracker)
{
    psmove_return_val_if_fail(tracker != NULL, Exposure_INVALID);
    return tracker->exposure_mode;
}

void
psmove_tracker_enable_deinterlace(PSMoveTracker *tracker,
        enum PSMove_Bool enabled)
{
    psmove_return_if_fail(tracker != NULL);
    psmove_return_if_fail(tracker->cc != NULL);

    camera_control_set_deinterlace(tracker->cc, enabled);
}

void
psmove_tracker_set_mirror(PSMoveTracker *tracker,
        enum PSMove_Bool enabled)
{
    psmove_return_if_fail(tracker != NULL);

    tracker->mirror = enabled;
}

enum PSMove_Bool
psmove_tracker_get_mirror(PSMoveTracker *tracker)
{
    psmove_return_val_if_fail(tracker != NULL, PSMove_False);

    return tracker->mirror;
}

PSMoveTracker *
psmove_tracker_new_with_camera(int camera) {
	PSMoveTracker* tracker = (PSMoveTracker*) calloc(1, sizeof(PSMoveTracker));
	tracker->rHSV = cvScalar(COLOR_FILTER_RANGE_H, COLOR_FILTER_RANGE_S, COLOR_FILTER_RANGE_V, 0);
	tracker->storage = cvCreateMemStorage(0);

        tracker->dimming_factor = 0.;

	tracker->calibration_t = CALIBRATION_DIFF_T;
	tracker->tracker_t1 = TRACKER_QUALITY_T1;
	tracker->tracker_t2 = TRACKER_QUALITY_T2;
	tracker->tracker_t3 = TRACKER_QUALITY_T3;
	tracker->tracker_adaptive_xy = TRACKER_ADAPTIVE_XY;
	tracker->tracker_adaptive_z = TRACKER_ADAPTIVE_Z;
	tracker->adapt_t1 = COLOR_ADAPTION_QUALITY;
	tracker->color_t1 = COLOR_UPDATE_QUALITY_T1;
	tracker->color_t2 = COLOR_UPDATE_QUALITY_T2;
	tracker->color_t3 = COLOR_UPDATE_QUALITY_T3;
	tracker->color_update_rate = COLOR_UPDATE_RATE;

#if defined(__APPLE__) && !defined(CAMERA_CONTROL_USE_PS3EYE_DRIVER)
    PSMove *move = psmove_connect();
    psmove_set_leds(move, 255, 255, 255);
    psmove_update_leds(move);

    printf("Cover the iSight camera with the sphere and press the Move button\n");
    _psmove_wait_for_button(move, Btn_MOVE);
    psmove_set_leds(move, 0, 0, 0);
    psmove_update_leds(move);
    psmove_set_leds(move, 255, 255, 255);
    psmove_update_leds(move);
#endif

	// start the video capture device for tracking
    tracker->cc = camera_control_new(camera); // Returns NULL if no control found.
                                              // e.g. PS3EYE set during compile but not plugged in.
    if (!tracker->cc)
    {
        free(tracker);
        return NULL;
    }

        char *intrinsics_xml = psmove_util_get_file_path(INTRINSICS_XML);
        char *distortion_xml = psmove_util_get_file_path(DISTORTION_XML);
        camera_control_read_calibration(tracker->cc, intrinsics_xml, distortion_xml);
        free(intrinsics_xml);
        free(distortion_xml);

	// backup the systems settings, if not already backuped
	char *filename = psmove_util_get_file_path(PSEYE_BACKUP_FILE);
        camera_control_backup_system_settings(tracker->cc, filename);
	free(filename);

#if !defined(__APPLE__) || defined(CAMERA_CONTROL_USE_PS3EYE_DRIVER)
        // try to load color mapping data (not on Mac OS X for now, because the
        // automatic white balance means we get different colors every time)
        filename = psmove_util_get_file_path(COLOR_MAPPING_DAT);
        FILE *fp = NULL;
        time_t now = time(NULL);
        struct stat st;
        memset(&st, 0, sizeof(st));

        if (stat(filename, &st) == 0 && now != (time_t)-1) {
            if (st.st_mtime >= (now - COLOR_MAPPING_MAX_AGE)) {
                fp = fopen(filename, "rb");
            } else {
                printf("%s is too old - not restoring colors.\n", filename);
            }
        }

        if (fp) {
            if (!fread(&(tracker->color_mapping),
                        sizeof(struct ColorMappingRingBuffer),
                        1, fp)) {
                psmove_WARNING("Cannot read data from: %s\n", filename);
            } else {
                printf("color mappings restored.\n");
            }

            fclose(fp);
        }
        free(filename);
#endif

        // Default to the distance parameters for the PS Eye camera
        tracker->distance_parameters = pseye_distance_parameters;

	// use static exposure
        psmove_tracker_set_exposure(tracker, Exposure_LOW);

	// just query a frame so that we know the camera works
	IplImage* frame = NULL;
	while (!frame) {
		frame = camera_control_query_frame(tracker->cc, NULL, NULL);
	}

	// prepare ROI data structures

        /* Define the size of the biggest ROI */
        int size = psmove_util_get_env_int(PSMOVE_TRACKER_ROI_SIZE_ENV);

        if (size == -1) {
            size = MIN(frame->width, frame->height) / 2;
        } else {
            psmove_DEBUG("Using ROI size: %d\n", size);
        }

        int w = size, h = size;

    // We need to grab an image from the camera to determine the frame size
    psmove_tracker_update_image(tracker);

    tracker->search_tile_width = w;
    tracker->search_tile_height = h;

    tracker->search_tiles_horizontal = (tracker->frame->width +
            tracker->search_tile_width - 1) / tracker->search_tile_width;
    int search_tiles_vertical = (tracker->frame->height +
            tracker->search_tile_height - 1) / tracker->search_tile_height;

    tracker->search_tiles_count = tracker->search_tiles_horizontal *
        search_tiles_vertical;

    if (tracker->search_tiles_count % 2 == 0) {
        /**
         * search_tiles_count must be uneven, so that when picking every second
         * tile, we still "visit" every tile after two scans when we wrap:
         *
         *  ABA
         *  BAB
         *  ABA -> OK, first run = A, second run = B
         *
         *  ABAB
         *  ABAB -> NOT OK, first run = A, second run = A
         *
         * Incrementing the count will make the algorithm visit the lower right
         * item twice, but will then cause the second run to visit 'B's.
         *
         * We pick every second tile, so that we only need half the time to
         * sweep through the whole image (which usually means faster recovery).
         **/
        tracker->search_tiles_count++;
    }


	int i;
	for (i = 0; i < ROIS; i++) {
		tracker->roiI[i] = cvCreateImage(cvSize(w,h), frame->depth, 3);
		tracker->roiM[i] = cvCreateImage(cvSize(w,h), frame->depth, 1);

		/* Smaller rois are always square, and 70% of the previous level */
		h = w = MIN(w,h) * 0.7f;
	}

	// prepare structure used for erode and dilate in calibration process
	int ks = 5; // Kernel Size
	int kc = (ks + 1) / 2; // Kernel Center
	tracker->kCalib = cvCreateStructuringElementEx(ks, ks, kc, kc, CV_SHAPE_RECT, NULL);

#if defined(__APPLE__) && !defined(CAMERA_CONTROL_USE_PS3EYE_DRIVER)
    printf("Move the controller away and press the Move button\n");
    _psmove_wait_for_button(move, Btn_MOVE);
    psmove_set_leds(move, 0, 0, 0);
    psmove_update_leds(move);
    psmove_disconnect(move);
#endif

	return tracker;
}

enum PSMoveTracker_Status
psmove_tracker_enable(PSMoveTracker *tracker, PSMove *move)
{
    psmove_return_val_if_fail(tracker != NULL, Tracker_CALIBRATION_ERROR);
    psmove_return_val_if_fail(move != NULL, Tracker_CALIBRATION_ERROR);

    // Switch off the controller and all others while enabling another one
    TrackedController *tc;
    for_each_controller(tracker, tc) {
        psmove_set_leds(tc->move, 0, 0, 0);
        psmove_update_leds(tc->move);
    }
    psmove_set_leds(move, 0, 0, 0);
    psmove_update_leds(move);

    int i;

    /* Preset colors - use them in ascending order if not used yet */
    struct PSMove_RGBValue preset_colors[] = {
        {0xFF, 0x00, 0xFF}, /* magenta */
        {0x00, 0xFF, 0xFF}, /* cyan */
        {0xFF, 0xFF, 0x00}, /* yellow */
        {0xFF, 0x00, 0x00}, /* red */
#ifdef __APPLE__
        {0x00, 0xFF, 0x00}, /* green */
#else
        {0x00, 0x00, 0xFF}, /* blue */
#endif
    };

    for (i=0; i<ARRAY_LENGTH(preset_colors); i++) {
        if (!psmove_tracker_color_is_used(tracker, preset_colors[i])) {
            return psmove_tracker_enable_with_color_internal(tracker,
                    move, preset_colors[i]);
        }
    }

    /* No colors are available anymore */
    return Tracker_CALIBRATION_ERROR;
}

int
psmove_tracker_old_color_is_tracked(PSMoveTracker* tracker, PSMove* move, struct PSMove_RGBValue rgb)
{
    CvScalar color;

    if (!psmove_tracker_lookup_color(tracker, rgb, &color)) {
        return 0;
    }

    TrackedController *tc = psmove_tracker_find_controller(tracker, NULL);

    if (!tc) {
        return 0;
    }

    tc->move = move;
    tc->color = rgb;
    tc->auto_update_leds = PSMove_True;

    tc->eColor = tc->eFColor = color;
    tc->eColorHSV = tc->eFColorHSV = th_brg2hsv(tc->eFColor);

    /* Try to track the controller, give up after 100 iterations */
    int i;
    for (i=0; i<100; i++) {
        psmove_set_leds(move,
                rgb.r * tracker->dimming_factor,
                rgb.g * tracker->dimming_factor,
                rgb.b * tracker->dimming_factor);
        psmove_update_leds(move);
        usleep(1000 * 10); // wait 10ms - ok, since we're not blinking
        psmove_tracker_update_image(tracker);
        psmove_tracker_update(tracker, move);

        if (tc->is_tracked) {
            // TODO: Verify quality criteria to avoid bogus tracking
            return 1;
        }
    }

    psmove_tracker_disable(tracker, move);
    return 0;
}

int
psmove_tracker_lookup_color(PSMoveTracker *tracker, struct PSMove_RGBValue rgb, CvScalar *color)
{
    unsigned char current = tracker->color_mapping.next_slot - 1;
    unsigned char dimming = 255 * tracker->dimming_factor;

    while (current != tracker->color_mapping.next_slot) {
        if (memcmp(&rgb, &(tracker->color_mapping.map[current].from),
                    sizeof(struct PSMove_RGBValue)) == 0 &&
                tracker->color_mapping.map[current].dimming == dimming) {
            struct PSMove_RGBValue to = tracker->color_mapping.map[current].to;
            color->val[0] = to.r;
            color->val[1] = to.g;
            color->val[2] = to.b;
            return 1;
        }

        current--;
    }

    return 0;
}

void
psmove_tracker_remember_color(PSMoveTracker *tracker, struct PSMove_RGBValue rgb, CvScalar color)
{
    unsigned char dimming = 255 * tracker->dimming_factor;

    struct PSMove_RGBValue to;
    to.r = color.val[0];
    to.g = color.val[1];
    to.b = color.val[2];

    unsigned char slot = tracker->color_mapping.next_slot++;
    tracker->color_mapping.map[slot].from = rgb;
    tracker->color_mapping.map[slot].dimming = dimming;
    tracker->color_mapping.map[slot].to = to;

    char *filename = psmove_util_get_file_path(COLOR_MAPPING_DAT);
    FILE *fp = fopen(filename, "wb");
    if (fp) {
        if (!fwrite(&(tracker->color_mapping),
                    sizeof(struct ColorMappingRingBuffer),
                    1, fp)) {
            psmove_WARNING("Cannot write data to: %s\n", filename);
        } else {
            printf("color mappings saved.\n");
        }

        fclose(fp);
    }
    free(filename);
}

enum PSMoveTracker_Status
psmove_tracker_enable_with_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char r, unsigned char g, unsigned char b)
{
    psmove_return_val_if_fail(tracker != NULL, Tracker_CALIBRATION_ERROR);
    psmove_return_val_if_fail(move != NULL, Tracker_CALIBRATION_ERROR);

    struct PSMove_RGBValue rgb = { r, g, b };
    return psmove_tracker_enable_with_color_internal(tracker, move, rgb);
}

enum PSMove_Bool
psmove_tracker_blinking_calibration(PSMoveTracker *tracker, PSMove *move,
        struct PSMove_RGBValue rgb, CvScalar *color, CvScalar *hsv_color)
{
    char *color_str = psmove_util_get_env_string(PSMOVE_TRACKER_COLOR_ENV);
    if (color_str != NULL) {
        int r, g, b;
        if (sscanf(color_str, "%02x%02x%02x", &r, &g, &b) == 3) {
            printf("r: %d, g: %d, b: %d\n", r, g, b);
            *color = cvScalar(r, g, b, 0);
            *hsv_color = th_brg2hsv(*color);
            free(color_str);
            return PSMove_True;
        } else {
            psmove_WARNING("Cannot parse color: '%s'\n", color_str);
        }
        free(color_str);
    }

    psmove_tracker_update_image(tracker);
    IplImage* frame = tracker->frame;
    assert(frame != NULL);

    // Switch off all other controllers for better measurements
    TrackedController *tc;
    for_each_controller(tracker, tc) {
        psmove_set_leds(tc->move, 0, 0, 0);
        psmove_update_leds(tc->move);
    }

    // clear the calibration html trace
    psmove_html_trace_clear();

    IplImage *mask = NULL;
    IplImage *images[BLINKS]; // array of images saved during calibration for estimation of sphere color
    IplImage *diffs[BLINKS]; // array of masks saved during calibration for estimation of sphere color
    int i;
    for (i = 0; i < BLINKS; i++) {
        // allocate the images
        images[i] = cvCreateImage(cvGetSize(frame), frame->depth, 3);
        diffs[i] = cvCreateImage(cvGetSize(frame), frame->depth, 1);
    }
    double sizes[BLINKS]; // array of blob sizes saved during calibration for estimation of sphere color
    float sizeBest = 0;
    CvSeq *contourBest = NULL;

    // DEBUG log the assigned color
    psmove_html_trace_put_color_var("assignedColor", cvScalar(rgb.b, rgb.g, rgb.r, 0));

    float dimming = 1.0;
    if (tracker->dimming_factor > 0) {
        dimming = tracker->dimming_factor;
    }
    while (1) {
        for (i = 0; i < BLINKS; i++) {
            // create a diff image
            psmove_tracker_get_diff(tracker, move, rgb, images[i], diffs[i], BLINK_DELAY, dimming);

            // DEBUG log the diff image and the image with the lit sphere
            psmove_html_trace_image_at(images[i], i, "originals");
            psmove_html_trace_image_at(diffs[i], i, "rawdiffs");

            // threshold it to reduce image noise
            cvThreshold(diffs[i], diffs[i], tracker->calibration_t, 0xFF /* white */, CV_THRESH_BINARY);

            // DEBUG log the thresholded diff image
            psmove_html_trace_image_at(diffs[i], i, "threshdiffs");

            // use morphological operations to further remove noise
            cvErode(diffs[i], diffs[i], tracker->kCalib, 1);
            cvDilate(diffs[i], diffs[i], tracker->kCalib, 1);

            // DEBUG log the even more cleaned up diff-image
            psmove_html_trace_image_at(diffs[i], i, "erodediffs");
        }

        // put the diff images together to get hopefully only one intersection region
        // the region at which the controllers sphere resides.
        mask = diffs[0];
        for (i=1; i<BLINKS; i++) {
            cvAnd(mask, diffs[i], mask, NULL);
        }

        // find the biggest contour and repaint the blob where the sphere is expected
        psmove_tracker_biggest_contour(diffs[0], tracker->storage, &contourBest, &sizeBest);
        cvSet(mask, TH_COLOR_BLACK, NULL);
        if (contourBest) {
            cvDrawContours(mask, contourBest, TH_COLOR_WHITE, TH_COLOR_WHITE, -1, CV_FILLED, 8, cvPoint(0, 0));
        }
        cvClearMemStorage(tracker->storage);

        // DEBUG log the final mask used for color estimation
        psmove_html_trace_image_at(mask, 0, "finaldiff");

        // CHECK if the blob contains a minimum number of pixels
        if (cvCountNonZero(mask) < CALIB_MIN_SIZE) {
            psmove_html_trace_put_log_entry("WARNING", "The final mask my not be representative for color estimation.");
        }

        // calculate the average color from the first image
        *color = cvAvg(images[0], mask);
        *hsv_color = th_brg2hsv(*color);
        psmove_DEBUG("Dimming: %.2f, H: %.2f, S: %.2f, V: %.2f\n", dimming,
                hsv_color->val[0], hsv_color->val[1], hsv_color->val[2]);

        if (tracker->dimming_factor == 0.) {
            if (hsv_color->val[1] > 128) {
                tracker->dimming_factor = dimming;
                break;
            } else if (dimming < 0.01) {
                break;
            }
        } else {
            break;
        }

        dimming *= 0.3;
    }

    psmove_html_trace_put_color_var("estimatedColor", *color);
    psmove_html_trace_put_int_var("estimated_hue", (*hsv_color).val[0]);

    int valid_countours = 0;

    // calculate upper & lower bounds for the color filter
    CvScalar min = th_scalar_sub(*hsv_color, tracker->rHSV);
    CvScalar max = th_scalar_add(*hsv_color, tracker->rHSV);

    CvPoint firstPosition;
    for (i=0; i<BLINKS; i++) {
        // Convert to HSV, then apply the color range filter to the mask
        cvCvtColor(images[i], images[i], CV_BGR2HSV);
        cvInRangeS(images[i], min, max, mask);

        // use morphological operations to further remove noise
        cvErode(mask, mask, tracker->kCalib, 1);
        cvDilate(mask, mask, tracker->kCalib, 1);

        // DEBUG log the color filter and
        psmove_html_trace_image_at(mask, i, "filtered");

        // find the biggest contour in the image and save its location and size
        psmove_tracker_biggest_contour(mask, tracker->storage, &contourBest, &sizeBest);
        sizes[i] = 0;
        float dist = FLT_MAX;
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
        if (!contourBest) {
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
        cvClearMemStorage(tracker->storage);

    }

    // clean up all temporary images
    for (i=0; i<BLINKS; i++) {
        cvReleaseImage(&images[i]);
        cvReleaseImage(&diffs[i]);
    }

    // CHECK if sphere was found in each BLINK image
    if (valid_countours < BLINKS) {
        psmove_html_trace_put_log_entry("ERROR", "The sphere could not be found in all images.");
        return PSMove_False;
    }

    // CHECK if the size of the found contours are similar
    double sizeVariance, sizeAverage;
    th_stats(sizes, BLINKS, &sizeVariance, &sizeAverage);
    if (sqrt(sizeVariance) >= (sizeAverage / 100.0 * CALIB_SIZE_STD)) {
        psmove_html_trace_put_log_entry("ERROR", "The spheres found differ too much in size.");
        return PSMove_False;
    }

    return PSMove_True;
}


enum PSMoveTracker_Status
psmove_tracker_enable_with_color_internal(PSMoveTracker *tracker, PSMove *move,
        struct PSMove_RGBValue rgb)
{
    // check if the controller is already enabled!
    if (psmove_tracker_find_controller(tracker, move)) {
        return Tracker_CALIBRATED;
    }

    // cannot use the same color for two different controllers
    if (psmove_tracker_color_is_used(tracker, rgb)) {
        return Tracker_CALIBRATION_ERROR;
    }

    // try to track the controller with the old color, if it works we are done
    if (psmove_tracker_old_color_is_tracked(tracker, move, rgb)) {
        return Tracker_CALIBRATED;
    }

    CvScalar color;
    CvScalar hsv_color;
    if (psmove_tracker_blinking_calibration(tracker, move, rgb, &color, &hsv_color)) {
        // Find the next free slot to use as TrackedController
        TrackedController *tc = psmove_tracker_find_controller(tracker, NULL);

        if (tc != NULL) {
            tc->move = move;
            tc->color = rgb;
            tc->auto_update_leds = PSMove_True;

            psmove_tracker_remember_color(tracker, rgb, color);
            tc->eColor = tc->eFColor = color;
            tc->eColorHSV = tc->eFColorHSV = hsv_color;

            return Tracker_CALIBRATED;
        }
    }

    return Tracker_CALIBRATION_ERROR;
}

int
psmove_tracker_get_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char *r, unsigned char *g, unsigned char *b)
{
    psmove_return_val_if_fail(tracker != NULL, 0);
    psmove_return_val_if_fail(move != NULL, 0);

    TrackedController *tc = psmove_tracker_find_controller(tracker, move);

    if (tc) {
        *r = tc->color.r * tracker->dimming_factor;
        *g = tc->color.g * tracker->dimming_factor;
        *b = tc->color.b * tracker->dimming_factor;

        return 1;
    }

    return 0;
}

int
psmove_tracker_get_camera_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char *r, unsigned char *g, unsigned char *b)
{
    psmove_return_val_if_fail(tracker != NULL, 0);
    psmove_return_val_if_fail(move != NULL, 0);

    TrackedController *tc = psmove_tracker_find_controller(tracker, move);

    if (tc) {
        *r = tc->eColor.val[0];
        *g = tc->eColor.val[1];
        *b = tc->eColor.val[2];

        return 1;
    }

    return 0;
}

int
psmove_tracker_set_camera_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char r, unsigned char g, unsigned char b)
{
    psmove_return_val_if_fail(tracker != NULL, 0);
    psmove_return_val_if_fail(move != NULL, 0);

    TrackedController *tc = psmove_tracker_find_controller(tracker, move);

    if (tc) {
        /* Update the current color */
        tc->eColor.val[0] = r;
        tc->eColor.val[1] = g;
        tc->eColor.val[2] = b;
        tc->eColorHSV = th_brg2hsv(tc->eColor);

        /* Update the "first" color (to avoid re-adaption to old color) */
        tc->eFColor = tc->eColor;
        tc->eFColorHSV = tc->eColorHSV;

        return 1;
    }

    return 0;
}


void
psmove_tracker_disable(PSMoveTracker *tracker, PSMove *move)
{
    psmove_return_if_fail(tracker != NULL);
    psmove_return_if_fail(move != NULL);

    TrackedController *tc = psmove_tracker_find_controller(tracker, move);

    if (tc) {
        // Clear the tracked controller state - also sets move = NULL
        memset(tc, 0, sizeof(TrackedController));

        // XXX: If we "defrag" tracker->controllers to avoid holes with NULL
        // controllers, we can simplify psmove_tracker_find_controller() and
        // abort search at the first encounter of a NULL controller
    }
}

enum PSMoveTracker_Status
psmove_tracker_get_status(PSMoveTracker *tracker, PSMove *move)
{
    psmove_return_val_if_fail(tracker != NULL, Tracker_CALIBRATION_ERROR);
    psmove_return_val_if_fail(move != NULL, Tracker_CALIBRATION_ERROR);

    TrackedController *tc = psmove_tracker_find_controller(tracker, move);

    if (tc) {
        if (tc->is_tracked) {
            return Tracker_TRACKING;
        } else {
            return Tracker_CALIBRATED;
        }
    }

    return Tracker_NOT_CALIBRATED;
}

void*
psmove_tracker_get_frame(PSMoveTracker *tracker) {
	return tracker->frame;
}

PSMoveTrackerRGBImage
psmove_tracker_get_image(PSMoveTracker *tracker)
{
    PSMoveTrackerRGBImage result = { NULL, 0, 0 };

    if (tracker != NULL) {
        result.width = tracker->frame->width;
        result.height = tracker->frame->height;

        if (tracker->frame_rgb == NULL) {
            tracker->frame_rgb = cvCreateImage(cvSize(result.width, result.height),
                    IPL_DEPTH_8U, 3);
        }

        cvCvtColor(tracker->frame, tracker->frame_rgb, CV_BGR2RGB);
        result.data = tracker->frame_rgb->imageData;
    }

    return result;
}

void psmove_tracker_update_image(PSMoveTracker *tracker) {
    psmove_return_if_fail(tracker != NULL);

    tracker->ts_camera_begin = _psmove_timestamp();
    tracker->frame = camera_control_query_frame(tracker->cc,
            &(tracker->ts_camera_grab), &(tracker->ts_camera_retrieve));
    if (tracker->mirror) {
        /**
         * Mirror image on the X axis (works for me with the PS Eye on Linux,
         * although the OpenCV docs say the third parameter should be 0 for X
         * axis mirroring)
         *
         * See also:
         * http://cv-kolaric.blogspot.com/2007/07/effects-of-cvflip.html
         **/
        cvFlip(tracker->frame, NULL, 1);
    }
    tracker->ts_camera_converted = _psmove_timestamp();
}

int
psmove_tracker_update_controller(PSMoveTracker *tracker, TrackedController *tc)
{
        float x, y;
	int i = 0;
	int sphere_found = 0;

        if (tc->auto_update_leds) {
            unsigned char r, g, b;
            psmove_tracker_get_color(tracker, tc->move, &r, &g, &b);
            psmove_set_leds(tc->move, r, g, b);
            psmove_update_leds(tc->move);
        }

	// calculate upper & lower bounds for the color filter
	CvScalar min = th_scalar_sub(tc->eColorHSV, tracker->rHSV);
        CvScalar max = th_scalar_add(tc->eColorHSV, tracker->rHSV);

	// this is the tracking algorithm
	while (1) {
		// get pointers to data structures for the given ROI-Level
		IplImage *roi_i = tracker->roiI[tc->roi_level];
		IplImage *roi_m = tracker->roiM[tc->roi_level];

		// adjust the ROI, so that the blob is fully visible, but only if we have a reasonable FPS
		if (tracker->debug_fps > ROI_ADJUST_FPS_T) {
			// TODO: check for validity differently
			CvPoint nRoiCenter;
                        if (psmove_tracker_center_roi_on_controller(tc, tracker, &nRoiCenter)) {
				psmove_tracker_set_roi(tracker, tc, nRoiCenter.x, nRoiCenter.y, roi_i->width, roi_i->height);
			}
		}

		// apply the ROI
		cvSetImageROI(tracker->frame, cvRect(tc->roi_x, tc->roi_y, roi_i->width, roi_i->height));
		cvCvtColor(tracker->frame, roi_i, CV_BGR2HSV);

		// apply color filter
		cvInRangeS(roi_i, min, max, roi_m);

		#ifdef DEBUG_WINDOWS
                        cvShowImage("binary:0", roi_m);
                        cvShowImage("hsv:0", roi_i);
		#endif

		// find the biggest contour in the image
		float sizeBest = 0;
		CvSeq* contourBest = NULL;
		psmove_tracker_biggest_contour(roi_m, tracker->storage, &contourBest, &sizeBest);

		if (contourBest) {
			CvMoments mu; // ImageMoments are use to calculate the center of mass of the blob
			CvRect br = cvBoundingRect(contourBest, 0);

			// restore the biggest contour
			cvSet(roi_m, TH_COLOR_BLACK, NULL);
			cvDrawContours(roi_m, contourBest, TH_COLOR_WHITE, TH_COLOR_WHITE, -1, CV_FILLED, 8, cvPoint(0, 0));
			// calucalte image-moments
			cvMoments(roi_m, &mu, 0);
			// calucalte the mass center
			CvPoint p = cvPoint(mu.m10 / mu.m00, mu.m01 / mu.m00);
			CvPoint oldMCenter = cvPoint(tc->mx, tc->my);
			tc->mx = p.x + tc->roi_x;
			tc->my = p.y + tc->roi_y;
			CvPoint newMCenter = cvPoint(tc->mx, tc->my);

			// remember the old radius and calcutlate the new x/y position and radius of the found contour
			float oldRadius = tc->r;
			// estimate x/y position and radius of the sphere
			psmove_tracker_estimate_circle_from_contour(contourBest, &x, &y, &tc->r);

			// apply radius-smoothing if enabled
			if (tracker->tracker_adaptive_z) {
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
			if (tracker->tracker_adaptive_z) {
				// a big distance between the old and new center of mass results in no smoothing
				// a little one to strong smoothing
				float diff = sqrt(th_dist_squared(oldMCenter, newMCenter));
				float f = MIN(diff / 7 + 0.15, 1);
				// apply adaptive smoothing
				tc->x = tc->x * (1 - f) + (x + tc->roi_x) * f;
				tc->y = tc->y * (1 - f) + (y + tc->roi_y) * f;
			} else {
				// do NOT apply adaptive smoothing
				tc->x = x + tc->roi_x;
				tc->y = y + tc->roi_y;
			}

			// calculate the quality of the tracking
			int pixelInBlob = cvCountNonZero(roi_m);
			float pixelInResult = tc->r * tc->r * M_PI;
                        tc->q1 = 0;
                        tc->q2 = FLT_MAX;
                        tc->q3 = tc->r;

			// decrease TQ1 by half if below 20px (gives better results if controller is far away)
			if (pixelInBlob < 20) {
				tc->q1 /= 2;
                        }

			// The quality checks are all performed on the radius of the blob
			// its old radius and size.
			tc->q1 = pixelInBlob / pixelInResult;

			// always check pixel-ratio and minimal size
			sphere_found = tc->q1 > tracker->tracker_t1 && tc->q3 > tracker->tracker_t3;

			// use the mass center if the quality is very good
			// TODO: make 0.85 as a CONST
			if (tc->q1 > 0.85) {
				tc->x = tc->mx;
				tc->y = tc->my;
			}
			// only perform check if we already found the sphere once
			if (oldRadius > 0 && tc->search_tile==0) {
				tc->q2 = abs(oldRadius - tc->r) / (oldRadius + FLT_EPSILON);

				// additionally check for to big changes
				sphere_found = sphere_found && tc->q2 < tracker->tracker_t2;
			}

			// only if the quality is okay update the future ROI
			if (sphere_found) {
				// use adaptive color detection
				// only if 	1) the sphere has been found
				// AND		2) the UPDATE_RATE has passed
				// AND		3) the tracking-quality is high;
				int do_color_adaption = 0;
				long now = psmove_util_get_ticks();
				if (tracker->color_update_rate > 0 && (now - tc->last_color_update) > tracker->color_update_rate*1000)
					do_color_adaption = 1;

				if (do_color_adaption && tc->q1 > tracker->color_t1 && tc->q2 < tracker->color_t2 && tc->q3 > tracker->color_t3) {
					// calculate the new estimated color (adaptive color estimation)
					CvScalar newColor = cvAvg(tracker->frame, roi_m);

                                        tc->eColor = th_scalar_mul(th_scalar_add(tc->eColor, newColor), 0.5);

					tc->eColorHSV = th_brg2hsv(tc->eColor);
					tc->last_color_update = now;
					// CHECK if the current estimate is too far away from its original estimation
					if (psmove_tracker_hsvcolor_diff(tc) > tracker->adapt_t1) {
						tc->eColor = tc->eFColor;
						tc->eColorHSV = tc->eFColorHSV;
						sphere_found = 0;
					}
				}

				// update the future roi box
				br.width = MAX(br.width, br.height) * 3;
				br.height = br.width;
				// find a suitable ROI level
				for (i = 0; i < ROIS; i++) {
					if (br.width > tracker->roiI[i]->width && br.height > tracker->roiI[i]->height)
						break;

                                        if (tc->roi_level_fixed) {
                                            tc->roi_level = 0;
                                        } else {
                                            tc->roi_level = i;
                                        }
					// update easy accessors
					roi_i = tracker->roiI[tc->roi_level];
					roi_m = tracker->roiM[tc->roi_level];
				}

				// assure that the roi is within the target image
				psmove_tracker_set_roi(tracker, tc, tc->x - roi_i->width / 2, tc->y - roi_i->height / 2,  roi_i->width, roi_i->height);
			}
		}
		cvClearMemStorage(tracker->storage);
		cvResetImageROI(tracker->frame);

		if (sphere_found) {
			//tc->search_tile = 0;
			// the sphere was found
			break;
		}else if(tc->roi_level>0){
			// the sphere was not found, increase the ROI and search again!
			tc->roi_x += roi_i->width / 2;
			tc->roi_y += roi_i->height / 2;

                        if (tc->roi_level_fixed) {
                            tc->roi_level = 0;
                        } else {
                            tc->roi_level = tc->roi_level - 1;
                        }
			// update easy accessors
			roi_i = tracker->roiI[tc->roi_level];
			roi_m = tracker->roiM[tc->roi_level];

			// assure that the roi is within the target image
			psmove_tracker_set_roi(tracker, tc, tc->roi_x -roi_i->width / 2, tc->roi_y - roi_i->height / 2, roi_i->width, roi_i->height);
		}else {
			int rx;
			int ry;
			// the sphere could not be found til a reasonable roi-level

                        rx = tracker->search_tile_width * (tc->search_tile %
                                tracker->search_tiles_horizontal);
                        ry = tracker->search_tile_height * (int)(tc->search_tile /
                                tracker->search_tiles_horizontal);
                        tc->search_tile = ((tc->search_tile + 2) %
                                tracker->search_tiles_count);

			tc->roi_level=0;
			psmove_tracker_set_roi(tracker, tc, rx, ry, tracker->roiI[tc->roi_level]->width, tracker->roiI[tc->roi_level]->height);
			break;
		}
	}

	// remember if the sphere was found
	tc->is_tracked = sphere_found;
	return sphere_found;
}

int
psmove_tracker_update(PSMoveTracker *tracker, PSMove *move)
{
    psmove_return_val_if_fail(tracker->frame != NULL, 0);

    int spheres_found = 0;

    long started = psmove_util_get_ticks();

    TrackedController *tc;
    for_each_controller(tracker, tc) {
        if (move == NULL || tc->move == move) {
            spheres_found += psmove_tracker_update_controller(tracker, tc);
        }
    }

    tracker->duration = psmove_util_get_ticks() - started;

    return spheres_found;
}

int
psmove_tracker_get_position(PSMoveTracker *tracker, PSMove *move,
        float *x, float *y, float *radius)
{
    psmove_return_val_if_fail(tracker != NULL, 0);
    psmove_return_val_if_fail(move != NULL, 0);

    TrackedController *tc = psmove_tracker_find_controller(tracker, move);

    if (tc) {
        if (x) {
            *x = tc->x;
        }
        if (y) {
            *y = tc->y;
        }
        if (radius) {
            *radius = tc->r;
        }

        // TODO: return age of tracking values (if possible)
        return 1;
    }

    return 0;
}

void
psmove_tracker_get_size(PSMoveTracker *tracker,
        int *width, int *height)
{
    psmove_return_if_fail(tracker != NULL);
    psmove_return_if_fail(tracker->frame != NULL);

    *width = tracker->frame->width;
    *height = tracker->frame->height;
}

void
psmove_tracker_free(PSMoveTracker *tracker)
{
    psmove_return_if_fail(tracker != NULL);

    if (tracker->frame_rgb != NULL) {
        cvReleaseImage(&tracker->frame_rgb);
    }

    char *filename = psmove_util_get_file_path(PSEYE_BACKUP_FILE);
    camera_control_restore_system_settings(tracker->cc, filename);
    free(filename);

    cvReleaseMemStorage(&tracker->storage);

    int i;
    for (i=0; i < ROIS; i++) {
        cvReleaseImage(&tracker->roiM[i]);
        cvReleaseImage(&tracker->roiI[i]);
    }
    cvReleaseStructuringElement(&tracker->kCalib);

    camera_control_delete(tracker->cc);
    free(tracker);
}

// -------- Implementation: internal functions only
int
psmove_tracker_adapt_to_light(PSMoveTracker *tracker, float target_luminance)
{
    float minimum_exposure = 2051;
    float maximum_exposure = 65535;
    float current_exposure = (maximum_exposure + minimum_exposure) / 2.;

    if (target_luminance == 0) {
        return minimum_exposure;
    }

    float step_size = (maximum_exposure - minimum_exposure) / 4.;

    // Switch off the controllers' LEDs for proper environment measurements
    TrackedController *tc;
    for_each_controller(tracker, tc) {
        psmove_set_leds(tc->move, 0, 0, 0);
        psmove_update_leds(tc->move);
    }

    int i;
    for (i=0; i<7; i++) {
        camera_control_set_parameters(tracker->cc, 0, 0, 0,
                (int)current_exposure, 0, 0xffff, 0xffff, 0xffff, -1, -1);

        IplImage* frame;
        psmove_tracker_wait_for_frame(tracker, &frame, 50);
        assert(frame != NULL);

        // calculate the average color and luminance (energy)
        float luminance = th_color_avg(cvAvg(frame, NULL));

        psmove_DEBUG("Exposure: %.2f, Luminance: %.2f\n", current_exposure, luminance);
        if (fabsf(luminance - target_luminance) < 1) {
            break;
        }

        // Binary search for the best exposure setting
        if (luminance > target_luminance) {
            current_exposure -= step_size;
        } else {
            current_exposure += step_size;
        }

        step_size /= 2.;
    }

    return current_exposure;
}


TrackedController *
psmove_tracker_find_controller(PSMoveTracker *tracker, PSMove *move)
{
    psmove_return_val_if_fail(tracker != NULL, NULL);

    int i;
    for (i=0; i<PSMOVE_TRACKER_MAX_CONTROLLERS; i++) {
        if (tracker->controllers[i].move == move) {
            return &(tracker->controllers[i]);
        }

        // XXX: Assuming a "defragmented" list of controllers, we could stop our
        // search here if we arrive at a controller where move == NULL and admit
        // failure immediately. See the comment in psmove_tracker_disable() for
        // what we would have to do to always keep the list defragmented.
    }

    return NULL;
}

void
psmove_tracker_wait_for_frame(PSMoveTracker *tracker, IplImage **frame, int delay)
{
    int elapsed_time = 0;
    int step = 10;

    while (elapsed_time < delay) {
        usleep(1000 * step);
        *frame = camera_control_query_frame(tracker->cc, NULL, NULL);
        elapsed_time += step;
    }
}

void psmove_tracker_get_diff(PSMoveTracker* tracker, PSMove* move,
        struct PSMove_RGBValue rgb, IplImage* on, IplImage* diff, int delay,
        float dimming_factor)
{
	// the time to wait for the controller to set the color up
	IplImage* frame;
	// switch the LEDs ON and wait for the sphere to be fully lit
	rgb.r *= dimming_factor;
	rgb.g *= dimming_factor;
	rgb.b *= dimming_factor;
	psmove_set_leds(move, rgb.r, rgb.g, rgb.b);
	psmove_update_leds(move);

	// take the first frame (sphere lit)
        psmove_tracker_wait_for_frame(tracker, &frame, delay);
	cvCopy(frame, on, NULL);

	// switch the LEDs OFF and wait for the sphere to be off
	psmove_set_leds(move, 0, 0, 0);
	psmove_update_leds(move);

	// take the second frame (sphere iff)
        psmove_tracker_wait_for_frame(tracker, &frame, delay);

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

void psmove_tracker_set_roi(PSMoveTracker* tracker, TrackedController* tc, int roi_x, int roi_y, int roi_width, int roi_height) {
	tc->roi_x = roi_x;
	tc->roi_y = roi_y;
	
	if (tc->roi_x < 0)
		tc->roi_x = 0;
	if (tc->roi_y < 0)
		tc->roi_y = 0;

	if (tc->roi_x + roi_width > tracker->frame->width)
		tc->roi_x = tracker->frame->width - roi_width;
	if (tc->roi_y + roi_height > tracker->frame->height)
		tc->roi_y = tracker->frame->height - roi_height;
}

void psmove_tracker_annotate(PSMoveTracker* tracker) {
	CvPoint p;
	IplImage* frame = tracker->frame;

        CvFont fontSmall = cvFont(0.8, 1);
        CvFont fontNormal = cvFont(1, 1);

	char text[256];
	CvScalar c;
	CvScalar avgC;
	float avgLum = 0;
	int roi_w = 0;
	int roi_h = 0;

	// general statistics
	avgC = cvAvg(frame, 0x0);
	avgLum = th_color_avg(avgC);
	cvRectangle(frame, cvPoint(0, 0), cvPoint(frame->width, 25), TH_COLOR_BLACK, CV_FILLED, 8, 0);
	sprintf(text, "fps:%.0f", tracker->debug_fps);
	cvPutText(frame, text, cvPoint(10, 20), &fontNormal, TH_COLOR_WHITE);
        if (tracker->duration) {
            tracker->debug_fps = (0.85 * tracker->debug_fps + 0.15 *
                (1000. / (double)tracker->duration));
        }
	sprintf(text, "avg(lum):%.0f", avgLum);
	cvPutText(frame, text, cvPoint(255, 20), &fontNormal, TH_COLOR_WHITE);


	// draw all/one controller information to camera image
        TrackedController *tc;
        for_each_controller(tracker, tc) {
		if (tc->is_tracked) {
			// controller specific statistics
			p.x = tc->x;
			p.y = tc->y;
			roi_w = tracker->roiI[tc->roi_level]->width;
			roi_h = tracker->roiI[tc->roi_level]->height;
			c = tc->eColor;

			cvRectangle(frame, cvPoint(tc->roi_x, tc->roi_y), cvPoint(tc->roi_x + roi_w, tc->roi_y + roi_h), TH_COLOR_WHITE, 3, 8, 0);
			cvRectangle(frame, cvPoint(tc->roi_x, tc->roi_y), cvPoint(tc->roi_x + roi_w, tc->roi_y + roi_h), TH_COLOR_RED, 1, 8, 0);
			cvRectangle(frame, cvPoint(tc->roi_x, tc->roi_y - 45), cvPoint(tc->roi_x + roi_w, tc->roi_y - 5), TH_COLOR_BLACK, CV_FILLED, 8, 0);

			int vOff = 0;
			if (roi_h == frame->height)
				vOff = roi_h;
			sprintf(text, "RGB:%x,%x,%x", (int) c.val[2], (int) c.val[1], (int) c.val[0]);
			cvPutText(frame, text, cvPoint(tc->roi_x, tc->roi_y + vOff - 5), &fontSmall, c);

			sprintf(text, "ROI:%dx%d", roi_w, roi_h);
			cvPutText(frame, text, cvPoint(tc->roi_x, tc->roi_y + vOff - 15), &fontSmall, c);

			double distance = psmove_tracker_distance_from_radius(tracker, tc->r);

			sprintf(text, "radius: %.2f", tc->r);
			cvPutText(frame, text, cvPoint(tc->roi_x, tc->roi_y + vOff - 35), &fontSmall, c);
			sprintf(text, "dist: %.2f cm", distance);
			cvPutText(frame, text, cvPoint(tc->roi_x, tc->roi_y + vOff - 25), &fontSmall, c);

			cvCircle(frame, p, tc->r, TH_COLOR_WHITE, 1, 8, 0);
		} else {
			roi_w = tracker->roiI[tc->roi_level]->width;
			roi_h = tracker->roiI[tc->roi_level]->height;
			cvRectangle(frame, cvPoint(tc->roi_x, tc->roi_y), cvPoint(tc->roi_x + roi_w, tc->roi_y + roi_h), tc->eColor, 3, 8, 0);
                }
	}
}

float psmove_tracker_hsvcolor_diff(TrackedController* tc) {
	float diff = 0;
	diff += abs(tc->eFColorHSV.val[0] - tc->eColorHSV.val[0]) * 1; // diff of HUE is very important
	diff += abs(tc->eFColorHSV.val[1] - tc->eColorHSV.val[1]) * 0.5; // saturation and value not so much
	diff += abs(tc->eFColorHSV.val[2] - tc->eColorHSV.val[2]) * 0.5;
	return diff;
}

void psmove_tracker_biggest_contour(IplImage* img, CvMemStorage* stor, CvSeq** resContour, float* resSize) {
	CvSeq* contour;
	*resSize = 0;
	*resContour = 0;
	cvFindContours(img, stor, &contour, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	for (; contour; contour = contour->h_next) {
		float f = cvContourArea(contour, CV_WHOLE_SEQ, 0);
		if (f > *resSize) {
			*resSize = f;
			*resContour = contour;
		}
	}
}

void
psmove_tracker_estimate_circle_from_contour(CvSeq* cont, float *x, float *y, float* radius)
{
    psmove_return_if_fail(cont != NULL);
    psmove_return_if_fail(x != NULL && y != NULL && radius != NULL);

	int i, j;
	float d = 0;
	float cd = 0;
	CvPoint m1 = cvPoint( 0, 0 );
	CvPoint m2 = cvPoint( 0, 0 );
	CvPoint * p1;
	CvPoint * p2;
        int found = 0;

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
                                found = 1;
			}
		}
	}
	// calculate center of that pair
        if (found) {
            *x = 0.5 * (m1.x + m2.x);
            *y = 0.5 * (m1.y + m2.y);
        }
	// calcualte the radius
	*radius = sqrt(d) / 2;
}

int
psmove_tracker_center_roi_on_controller(TrackedController* tc, PSMoveTracker* tracker, CvPoint *center)
{
    psmove_return_val_if_fail(tc != NULL, 0);
    psmove_return_val_if_fail(tracker != NULL, 0);
    psmove_return_val_if_fail(center != NULL, 0);

	CvScalar min = th_scalar_sub(tc->eColorHSV, tracker->rHSV);
        CvScalar max = th_scalar_add(tc->eColorHSV, tracker->rHSV);

	IplImage *roi_i = tracker->roiI[tc->roi_level];
	IplImage *roi_m = tracker->roiM[tc->roi_level];

	// cut out the roi!
	cvSetImageROI(tracker->frame, cvRect(tc->roi_x, tc->roi_y, roi_i->width, roi_i->height));
	cvCvtColor(tracker->frame, roi_i, CV_BGR2HSV);

	// apply color filter
	cvInRangeS(roi_i, min, max, roi_m);
	
	float sizeBest = 0;
	CvSeq* contourBest = NULL;
	psmove_tracker_biggest_contour(roi_m, tracker->storage, &contourBest, &sizeBest);
	if (contourBest) {
		cvSet(roi_m, TH_COLOR_BLACK, NULL);
		cvDrawContours(roi_m, contourBest, TH_COLOR_WHITE, TH_COLOR_WHITE, -1, CV_FILLED, 8, cvPoint(0, 0));
		// calucalte image-moments to estimate the better ROI center
		CvMoments mu;
		cvMoments(roi_m, &mu, 0);

		*center = cvPoint(mu.m10 / mu.m00, mu.m01 / mu.m00);
		center->x += tc->roi_x - roi_m->width / 2;
		center->y += tc->roi_y - roi_m->height / 2;
	}
	cvClearMemStorage(tracker->storage);
	cvResetImageROI(tracker->frame);

        return (contourBest != NULL);
}

float
psmove_tracker_distance_from_radius(PSMoveTracker *tracker, float radius)
{
    psmove_return_val_if_fail(tracker != NULL, 0.);

    double height = (double)tracker->distance_parameters.height;
    double center = (double)tracker->distance_parameters.center;
    double hwhm = (double)tracker->distance_parameters.hwhm;
    double shape = (double)tracker->distance_parameters.shape;
    double x = (double)radius;

    /**
     * Pearson type VII distribution
     * http://fityk.nieto.pl/model.html
     **/
    double a = pow((x - center) / hwhm, 2.);
    double b = pow(2., 1. / shape) - 1.;
    double c = 1. + a * b;
    double distance = height / pow(c, shape);

    return (float)distance;
}

void
psmove_tracker_set_distance_parameters(PSMoveTracker *tracker,
        float height, float center, float hwhm, float shape)
{
    psmove_return_if_fail(tracker != NULL);

    tracker->distance_parameters.height = height;
    tracker->distance_parameters.center = center;
    tracker->distance_parameters.hwhm = hwhm;
    tracker->distance_parameters.shape = shape;
}


int
psmove_tracker_color_is_used(PSMoveTracker *tracker, struct PSMove_RGBValue color)
{
    psmove_return_val_if_fail(tracker != NULL, 1);

    TrackedController *tc;
    for_each_controller(tracker, tc) {
        if (memcmp(&tc->color, &color, sizeof(struct PSMove_RGBValue)) == 0) {
            return 1;
        }
    }

    return 0;
}

void
_psmove_tracker_retrieve_stats(PSMoveTracker *tracker,
        PSMove_timestamp *ts_begin, PSMove_timestamp *ts_grab,
        PSMove_timestamp *ts_retrieve, PSMove_timestamp *ts_converted)
{
    psmove_return_if_fail(tracker != NULL);

    *ts_begin = tracker->ts_camera_begin;
    *ts_grab = tracker->ts_camera_grab;
    *ts_retrieve = tracker->ts_camera_retrieve;
    *ts_converted = tracker->ts_camera_converted;
}

void
_psmove_tracker_fix_roi_size(PSMoveTracker *tracker)
{
    psmove_return_if_fail(tracker != NULL);

    TrackedController *tc;
    for_each_controller (tracker, tc) {
        tc->roi_level_fixed = PSMove_True;
    }
}

