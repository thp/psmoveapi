/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Benjamin Venditti <benjamin.venditti@gmail.com>
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

#ifndef PSMOVE_TRACKER_H
#define PSMOVE_TRACKER_H

#include "psmove.h"
#include "psmove_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Defines the range of x/y values for the position getting, etc.. */
#define PSMOVE_TRACKER_DEFAULT_WIDTH 640
#define PSMOVE_TRACKER_DEFAULT_HEIGHT 480

/* Maximum number of controllers that can be tracked at once */
#define PSMOVE_TRACKER_MAX_CONTROLLERS 5

/* Name of the environment variable used to pick a camera */
#define PSMOVE_TRACKER_CAMERA_ENV "PSMOVE_TRACKER_CAMERA"

/* Name of the environment variable used to choose a pre-recorded video */
#define PSMOVE_TRACKER_FILENAME_ENV "PSMOVE_TRACKER_FILENAME"

/* Name of the environment variable used to set the biggest ROI size */
#define PSMOVE_TRACKER_ROI_SIZE_ENV "PSMOVE_TRACKER_ROI_SIZE"

/* Name of the environment variables for the camera image size */
#define PSMOVE_TRACKER_WIDTH_ENV "PSMOVE_TRACKER_WIDTH"
#define PSMOVE_TRACKER_HEIGHT_ENV "PSMOVE_TRACKER_HEIGHT"

typedef struct {
    void *data;
    int width;
    int height;
} PSMoveTrackerRGBImage;

/* Opaque data structure, defined only in psmove_tracker.c */
#ifndef SWIG
struct _PSMoveTracker;
typedef struct _PSMoveTracker PSMoveTracker;
#endif

/*! Status of the tracker */
enum PSMoveTracker_Status {
    Tracker_NOT_CALIBRATED, /*!< Controller not registered with tracker */
    Tracker_CALIBRATION_ERROR, /*!< Calibration failed (check lighting, visibility) */
    Tracker_CALIBRATED, /*!< Color calibration successful, not currently tracking */
    Tracker_TRACKING, /*!< Calibrated and successfully tracked in the camera */
};

/**
 * Create a new PS Move tracker and set up tracking
 *
 * This will select the best camera for tracking (this means that if
 * a PSEye is found, it will be used, otherwise the first available
 * camera will be used as fallback).
 *
 * Returns a new PSMoveTracker * instance or NULL (indicates error)
 **/
ADDAPI PSMoveTracker *
ADDCALL psmove_tracker_new();


/**
 * Create a new PS Move tracker and set up tracking
 *
 * This function can be used when multiple cameras are available to
 * force the use of a specific camera.
 *
 * camera ... zero-based index of the camera to use
 *
 * Usually it's better to use psmove_tracker_new() and let the library
 * choose the best camera, unless you have a good reason to use this.
 *
 * Returns a new PSMoveTracker * instance or NULL (indicates error)
 **/
ADDAPI PSMoveTracker *
ADDCALL psmove_tracker_new_with_camera(int camera);

ADDAPI void
ADDCALL psmove_tracker_set_auto_update_leds(PSMoveTracker *tracker, PSMove *move,
        enum PSMove_Bool auto_update_leds);

ADDAPI enum PSMove_Bool
ADDCALL psmove_tracker_get_auto_update_leds(PSMoveTracker *tracker, PSMove *move);

ADDAPI void
ADDCALL psmove_tracker_set_dimming(PSMoveTracker *tracker, float dimming);

ADDAPI float
ADDCALL psmove_tracker_get_dimming(PSMoveTracker *tracker);

ADDAPI void
ADDCALL psmove_tracker_enable_deinterlace(PSMoveTracker *tracker,
        enum PSMove_Bool enabled);

ADDAPI void
ADDCALL psmove_tracker_set_mirror(PSMoveTracker *tracker,
        enum PSMove_Bool enabled);

ADDAPI enum PSMove_Bool
ADDCALL psmove_tracker_get_mirror(PSMoveTracker *tracker);

/**
 * Enable tracking for a given PSMove * instance
 *
 * After this function has been called, the user program
 * should not set the LEDs of the controller directly.
 *
 * This function is non-blocking. Use the function
 * psmove_tracker_get_status() to query the progress of
 * calibration.
 *
 * Returns:
 *   Tracker_CALIBRATING when calibration has started,
 *   Tracker_CALIBRATED if it is already calibrated or
 *   Tracker_CALIBRATION_ERROR when there is any error.
 **/
ADDAPI enum PSMoveTracker_Status
ADDCALL psmove_tracker_enable(PSMoveTracker *tracker, PSMove *move);


/**
 * Enable tracking with a pre-defined sphere color
 *
 * This function does the same thing as the psmove_tracker_enable()
 * function, but forces the sphere color to a pre-determined value.
 *
 * Using this function might give worse tracking results, because
 * the color might not be optimal for a given lighting condition.
 *
 * The meanings of the parameters and return value are the same as
 * for psmove_tracker_enable(), and the r, g, b parameters are the
 * same as psmove_set_leds() of the PS Move API.
 **/
ADDAPI enum PSMoveTracker_Status
ADDCALL psmove_tracker_enable_with_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char r, unsigned char g, unsigned char b);


/**
 * Disable tracking for a given PSMove * instance
 *
 * If the PSMove * instance was never enabled, this function
 * does nothing. Otherwise it removes the instance from the
 * tracker and stops tracking the controller.
 *
 * At this point, the user program can set the LEDs again.
 * The library will set (and update) the LEDs to black in
 * this function if the PSMove * instance is valid.
 **/
ADDAPI void
ADDCALL psmove_tracker_disable(PSMoveTracker *tracker, PSMove *move);



/**
 * Get the current sphere color of a given controller
 *
 * r, g, b - Pointers to output destinations or NULL to ignore
 *
 * Returns nonzero if the color was successfully returned, zero if
 * the controller is not enabled or calibration has not completed yet.
 **/
ADDAPI int
ADDCALL psmove_tracker_get_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char *r, unsigned char *g, unsigned char *b);

ADDAPI int
ADDCALL psmove_tracker_get_camera_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char *r, unsigned char *g, unsigned char *b);

ADDAPI int
ADDCALL psmove_tracker_set_camera_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char r, unsigned char g, unsigned char b);

/**
 * Query the calibration status of a PSMove controller.
 *
 * This function is non-blocking and will return the
 * current status.
 *
 * Returns: Same as psmove_tracker_enable() - see there
 **/
ADDAPI enum PSMoveTracker_Status
ADDCALL psmove_tracker_get_status(PSMoveTracker *tracker, PSMove *move);

/**
 * Grabs internally a new image from the camera.
 * Should always be called before "psmove_tracker_update".
 *
 * tracker - A valid PSMoveTracker * instance
 *
 **/
ADDAPI void
ADDCALL psmove_tracker_update_image(PSMoveTracker *tracker);


/**
 * Process incoming data and update tracking information
 *
 * tracker - A valid PSMoveTracker * instance
 * move - A valid PSMove * instance (if only one controller should
 *        be updated), or NULL to update all enabled controllers
 *
 * Returns: nonzero if tracking was successful (sphere found), zero otherwise
 **/
ADDAPI int
ADDCALL psmove_tracker_update(PSMoveTracker *tracker, PSMove *move);

/**
 * Retrieves the most recently processed image by psmove_tracker_update
 *
 * tracker - A valid PSMoveTracker * instance
 *
 * Returns: returns the most recently processed image, zero otherwise
 *          XXX: Define the return value type (IplImage* internally)
 **/
ADDAPI void*
ADDCALL psmove_tracker_get_frame(PSMoveTracker *tracker);

ADDAPI PSMoveTrackerRGBImage
ADDCALL psmove_tracker_get_image(PSMoveTracker *tracker);

/**
 * Get the currently-tracked low-level position of the controllers
 *
 * tracker - A valid PSMoveTracker * instance
 * move - A valid (and enabled, with status Tracker_CALIBRATED) controller
 * x - A pointer to a float for storing the X coordinate, or NULL
 * y - A pointer to a float for storing the Y coordinate, or NULL
 * radius - A pointer to a float for storing the radius, or NULL
 *
 * Returns: the age of the sensor reading in milliseconds, or -1 on error
 **/
ADDAPI int
ADDCALL psmove_tracker_get_position(PSMoveTracker *tracker,
        PSMove *move, float *x, float *y, float *radius);


ADDAPI void
ADDCALL psmove_tracker_get_size(PSMoveTracker *tracker,
        int *width, int *height);

/**
 * Destroy an existing tracker instance and free allocated resources
 *
 * tracker - A valid PSMoveTracker * instance
 **/
ADDAPI void
ADDCALL psmove_tracker_free(PSMoveTracker *tracker);

#ifdef __cplusplus
}
#endif

#endif
