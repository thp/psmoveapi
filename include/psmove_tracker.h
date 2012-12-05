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

/* Name of the environment variable used to set a fixed tracker color */
#define PSMOVE_TRACKER_COLOR_ENV "PSMOVE_TRACKER_COLOR"

/* Name of the environment variables for the camera image size */
#define PSMOVE_TRACKER_WIDTH_ENV "PSMOVE_TRACKER_WIDTH"
#define PSMOVE_TRACKER_HEIGHT_ENV "PSMOVE_TRACKER_HEIGHT"

typedef struct {
    void *data;
    int width;
    int height;
} PSMoveTrackerRGBImage; /*!< Structure for storing RGB image data */

/* Opaque data structure, defined only in psmove_tracker.c */
#ifndef SWIG
struct _PSMoveTracker;
typedef struct _PSMoveTracker PSMoveTracker; /*!< Handle to a Tracker object.
                                                  Obtained via psmove_tracker_new() */
#endif

/*! Status of the tracker */
enum PSMoveTracker_Status {
    Tracker_NOT_CALIBRATED, /*!< Controller not registered with tracker */
    Tracker_CALIBRATION_ERROR, /*!< Calibration failed (check lighting, visibility) */
    Tracker_CALIBRATED, /*!< Color calibration successful, not currently tracking */
    Tracker_TRACKING, /*!< Calibrated and successfully tracked in the camera */
};

/*! Exposure modes */
enum PSMoveTracker_Exposure {
    Exposure_LOW, /*!< Very low exposure: Good tracking, no environment visible */
    Exposure_MEDIUM, /*!< Middle ground: Good tracking, environment visibile */
    Exposure_HIGH, /*!< High exposure: Fair tracking, but good environment */
    Exposure_INVALID, /*!< Invalid exposure value (for returning failures) */
};

/**
 * \brief Create a new PS Move Tracker instance and open the camera
 *
 * This will select the best camera for tracking (this means that if
 * a PSEye is found, it will be used, otherwise the first available
 * camera will be used as fallback).
 *
 * \return A new \ref PSMoveTracker instance or \c NULL on error
 **/
ADDAPI PSMoveTracker *
ADDCALL psmove_tracker_new();


/**
 * \brief Create a new PS Move Tracker instance with a specific camera
 *
 * This function can be used when multiple cameras are available to
 * force the use of a specific camera.
 *
 * Usually it's better to use psmove_tracker_new() and let the library
 * choose the best camera, unless you have a good reason not to.
 *
 * \param camera Zero-based index of the camera to use
 *
 * \return A new \ref PSMoveTracker instance or \c NULL on error
 **/
ADDAPI PSMoveTracker *
ADDCALL psmove_tracker_new_with_camera(int camera);

/**
 * \brief Configure if the LEDs of a controller should be auto-updated
 *
 * If auto-update is enabled (the default), the tracker will set and
 * update the LEDs of the controller automatically. If not, the user
 * must set the LEDs of the controller and update them regularly. In
 * that case, the user can use psmove_tracker_get_color() to determine
 * the color that the controller's LEDs have to be set to.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A valid \ref PSMove handle
 * \param auto_update_leds \ref PSMove_True to auto-update LEDs from
 *                         the tracker, \ref PSMove_False if the user
 *                         will take care of updating the LEDs
 **/
ADDAPI void
ADDCALL psmove_tracker_set_auto_update_leds(PSMoveTracker *tracker, PSMove *move,
        enum PSMove_Bool auto_update_leds);

/**
 * \brief Check if the LEDs of a controller are updated automatically
 *
 * This is the getter function for psmove_tracker_set_auto_update_leds().
 * See there for details on what auto-updating LEDs means.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A valid \ref PSMove handle
 *
 * \return \ref PSMove_True if the controller's LEDs are set to be
 *         updated automatically, \ref PSMove_False otherwise
 **/
ADDAPI enum PSMove_Bool
ADDCALL psmove_tracker_get_auto_update_leds(PSMoveTracker *tracker, PSMove *move);

/**
 * \brief Set the LED dimming value for all controller
 *
 * Usually it's not necessary to call this function, as the dimming
 * is automatically determined when the first controller is enabled.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param dimming A value in the range from 0 (LEDs switched off) to
 *                1 (full LED intensity)
 **/
ADDAPI void
ADDCALL psmove_tracker_set_dimming(PSMoveTracker *tracker, float dimming);

/**
 * \brief Get the LED dimming value for all controllers
 *
 * See psmove_tracker_set_dimming() for details.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 *
 * \return The dimming value for the LEDs
 **/
ADDAPI float
ADDCALL psmove_tracker_get_dimming(PSMoveTracker *tracker);

/**
 * \brief Set the desired camera exposure mode
 *
 * This function sets the desired exposure mode. This should be
 * called before controllers are added to the tracker, so that the
 * dimming for the controllers can be determined for the specific
 * exposure setting.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param exposure One of the \ref PSMoveTracker_Exposure values
 **/
ADDAPI void
ADDCALL psmove_tracker_set_exposure(PSMoveTracker *tracker, enum PSMoveTracker_Exposure exposure);

/**
 * \brief Get the desired camera exposure mode
 *
 * See psmove_tracker_set_exposure() for details.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 *
 * \return One of the \ref PSMoveTracker_Exposure values
 **/
ADDAPI enum PSMoveTracker_Exposure
ADDCALL psmove_tracker_get_exposure(PSMoveTracker *tracker);

/**
 * \brief Enable or disable camera image deinterlacing (line doubling)
 *
 * Enables or disables camera image deinterlacing for this tracker.
 * You usually only want to enable deinterlacing if your camera source
 * provides interlaced images (e.g. 1080i). The interlacing will be
 * removed by doubling every other line. By default, deinterlacing is
 * disabled.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param enabled \ref PSMove_True to enable deinterlacing,
 *                \ref PSMove_False to disable deinterlacing (default)
 **/
ADDAPI void
ADDCALL psmove_tracker_enable_deinterlace(PSMoveTracker *tracker,
        enum PSMove_Bool enabled);

/**
 * \brief Enable or disable horizontal camera image mirroring
 *
 * Enables or disables horizontal mirroring of the camera image. The
 * mirroring setting will affect the X coordinates of the controller
 * positions tracked, as well as the output image. In addition, the
 * sensor fusion module will mirror the orientation information if
 * mirroring is set here. By default, mirroring is disabled.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param enabled \ref PSMove_True to mirror the image horizontally,
 *                \ref PSMove_False to leave the image as-is (default)
 **/
ADDAPI void
ADDCALL psmove_tracker_set_mirror(PSMoveTracker *tracker,
        enum PSMove_Bool enabled);

/**
 * \brief Query the current camera image mirroring state
 *
 * See psmove_tracker_set_mirror() for details.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 *
 * \return \ref PSMove_True if mirroring is enabled,
 *         \ref PSMove_False if mirroring is disabled
 **/
ADDAPI enum PSMove_Bool
ADDCALL psmove_tracker_get_mirror(PSMoveTracker *tracker);

/**
 * \brief Enable tracking of a motion controller
 *
 * Calling this function will register the controller with the
 * tracker, and start blinking calibration. The user should hold
 * the motion controller in front of the camera and wait for the
 * calibration to finish.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A valid \ref PSMove handle
 *
 * \return \ref Tracker_CALIBRATED if calibration succeeded
 * \return \ref Tracker_CALIBRATION_ERROR if calibration failed
 **/
ADDAPI enum PSMoveTracker_Status
ADDCALL psmove_tracker_enable(PSMoveTracker *tracker, PSMove *move);

/**
 * \brief Enable tracking with a custom sphere color
 *
 * This function does basically the same as psmove_tracker_enable(),
 * but forces the sphere color to a pre-determined value.
 *
 * Using this function might give worse tracking results, because
 * the color might not be optimal for a given lighting condition.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A valid \ref PSMove handle
 * \param r The red intensity of the desired color (0..255)
 * \param g The green intensity of the desired color (0..255)
 * \param b The blue intensity of the desired color (0..255)
 *
 * \return \ref Tracker_CALIBRATED if calibration succeeded
 * \return \ref Tracker_CALIBRATION_ERROR if calibration failed
 **/
ADDAPI enum PSMoveTracker_Status
ADDCALL psmove_tracker_enable_with_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char r, unsigned char g, unsigned char b);

/**
 * \brief Disable tracking of a motion controller
 *
 * If the \ref PSMove instance was never enabled, this function
 * does nothing. Otherwise it removes the instance from the
 * tracker and stops tracking the controller.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A valid \ref PSMove handle
 **/
ADDAPI void
ADDCALL psmove_tracker_disable(PSMoveTracker *tracker, PSMove *move);

/**
 * \brief Get the desired sphere color of a motion controller
 *
 * Get the sphere color of the controller as it is set using
 * psmove_update_leds(). This is not the color as the sphere
 * appears in the camera - for that, see
 * psmove_tracker_get_camera_color().
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A Valid \ref PSmove handle
 * \param r Pointer to store the red component of the color
 * \param g Pointer to store the green component of the color
 * \param g Pointer to store the blue component of the color
 *
 * \return Nonzero if the color was successfully returned, zero if
 *         if the controller is not enabled of calibration has not
 *         completed yet.
 **/
ADDAPI int
ADDCALL psmove_tracker_get_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char *r, unsigned char *g, unsigned char *b);

/**
 * \brief Get the sphere color of a controller in the camera image
 *
 * Get the sphere color of the controller as it currently
 * appears in the camera image. This is not the color that is
 * set using psmove_update_leds() - for that, see
 * psmove_tracker_get_color().
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A Valid \ref PSmove handle
 * \param r Pointer to store the red component of the color
 * \param g Pointer to store the green component of the color
 * \param g Pointer to store the blue component of the color
 *
 * \return Nonzero if the color was successfully returned, zero if
 *         if the controller is not enabled of calibration has not
 *         completed yet.
 **/
ADDAPI int
ADDCALL psmove_tracker_get_camera_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char *r, unsigned char *g, unsigned char *b);

/**
 * \brief Set the sphere color of a controller in the camera image
 *
 * This function should only be used in special situations - it is
 * usually not required to manually set the sphere color as it appears
 * in the camera image, as this color is determined at runtime during
 * blinking calibration. For some use cases, it might be useful to
 * set the color manually (e.g. when the user should be able to select
 * the color in the camera image after lighting changes).
 * 
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A valid \ref PSMove handle
 * \param r The red component of the color (0..255)
 * \param g The green component of the color (0..255)
 * \param b The blue component of the color (0..255)
 *
 * \return Nonzero if the color was successfully set, zero if
 *         if the controller is not enabled of calibration has not
 *         completed yet.
 **/
ADDAPI int
ADDCALL psmove_tracker_set_camera_color(PSMoveTracker *tracker, PSMove *move,
        unsigned char r, unsigned char g, unsigned char b);

/**
 * \brief Query the tracking status of a motion controller
 *
 * This function returns the current tracking status (or calibration
 * status if the controller is not calibrated yet) of a controller.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A valid \ref PSMove handle
 *
 * \return One of the \ref PSMoveTracker_Status values
 **/
ADDAPI enum PSMoveTracker_Status
ADDCALL psmove_tracker_get_status(PSMoveTracker *tracker, PSMove *move);

/**
 * \brief Retrieve the next image from the camera
 *
 * This function should be called once per main loop iteration (even
 * if multiple controllers are tracked), and will grab the next camera
 * image from the camera input device.
 *
 * This function must be called before psmove_tracker_update().
 *
 * \param tracker A valid \ref PSMoveTracker handle
 **/
ADDAPI void
ADDCALL psmove_tracker_update_image(PSMoveTracker *tracker);

/**
 * \brief Process incoming data and update tracking information
 *
 * This function tracks one or all motion controllers in the camera
 * image, and updates tracking information such as position, radius
 * and camera color.
 *
 * This function must be called after psmove_tracker_update_image().
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A valid \ref PSMove handle (to update a single controller)
 *             or \c NULL to update all enabled controllers at once
 *
 * \return Nonzero if tracking was successful, zero otherwise
 **/
ADDAPI int
ADDCALL psmove_tracker_update(PSMoveTracker *tracker, PSMove *move);

/**
 * \brief Draw debugging information onto the current camera image
 *
 * This function has to be called after psmove_tracker_update(), and
 * will annotate the camera image with sphere positions and other
 * information. The camera image will be modified in place, so no
 * call to psmove_tracker_update() should be carried out before the
 * next call to psmove_tracker_update_image().
 *
 * This function is used for demonstration and debugging purposes, in
 * production environments you usually do not want to use it.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 */
ADDAPI void
ADDCALL psmove_tracker_annotate(PSMoveTracker* tracker);

/**
 * \brief Get the current camera image as backend-specific pointer
 *
 * This function returns a pointer to the backend-specific camera
 * image. Right now, the only backend supported is OpenCV, so the
 * return value will always be a pointer to an IplImage structure.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 *
 * \return A pointer to the camera image (currently always an IplImage)
 *         - the caller MUST NOT modify or free the returned object.
 **/
ADDAPI void*
ADDCALL psmove_tracker_get_frame(PSMoveTracker *tracker);

/**
 * \brief Get the current camera image as 24-bit RGB data blob
 *
 * This function converts the internal camera image to a tightly-packed
 * 24-bit RGB image. The \ref PSMoveTrackerRGBImage structure is used
 * to return the image data pointer as well as the width and height of
 * the camera imaged. The size of the image data is 3 * width * height.
 *
 * The returned pixel data pointer points to tracker-internal data, and must
 * not be freed. The returned RGB data will only be valid as long as the
 * tracker exists.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * 
 * \return A \ref PSMoveTrackerRGBImage describing the RGB data and size.
 *         The RGB data is owned by the tracker, and must not be freed by
 *         the caller. The return value is valid only for the lifetime of
 *         the tracker object.
 **/
ADDAPI PSMoveTrackerRGBImage
ADDCALL psmove_tracker_get_image(PSMoveTracker *tracker);

/**
 * \brief Get the current position and radius of a tracked controller
 *
 * This function obtains the position and radius of a controller in the
 * camera image.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param move A valid \ref PSMove handle
 * \param x A pointer to store the X part of the position, or \c NULL
 * \param y A pointer to store the Y part of the position, or \c NULL
 * \param radius A pointer to store the controller radius, or \C NULL
 *
 * \return The age of the sensor reading in milliseconds, or -1 on error
 **/
ADDAPI int
ADDCALL psmove_tracker_get_position(PSMoveTracker *tracker,
        PSMove *move, float *x, float *y, float *radius);

/**
 * \brief Get the camera image size for the tracker
 *
 * This function can be used to obtain the real camera image size used
 * by the tracker. This is useful to convert the absolute position and
 * radius values to values relative to the image size if a camera is
 * used for which the size is now known. By default, the PS Eye camera
 * is used with an image size of 640x480.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param width A pointer to store the width of the camera image
 * \param height A pointer to store the height of the camera image
 **/
ADDAPI void
ADDCALL psmove_tracker_get_size(PSMoveTracker *tracker,
        int *width, int *height);

/**
 * \brief Calculate the physical distance (in cm) of the controller
 *
 * Given the radius of the controller in the image (in pixels), this function
 * calculates the physical distance of the controller from the camera (in cm).
 *
 * By default, this function's parameters are set up for the PS Eye camera in
 * wide angle view. You can set different parameters using the function
 * psmove_tracker_set_distance_parameters().
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param radius The radius for which the distance should be calculated, the
 *               radius is returned by psmove_tracker_get_position()
 **/
ADDAPI float
ADDCALL psmove_tracker_distance_from_radius(PSMoveTracker *tracker,
        float radius);

/**
 * \brief Set the parameters for the distance mapping function
 *
 * This function sets the parameters for the Pearson VII distribution
 * function that's used to map radius values to distance values in
 * psmove_tracker_distance_from_radius(). By default, the parameters are
 * set up so that they work well for a PS Eye camera in wide angle mode.
 *
 * The function is defined as in: http://fityk.nieto.pl/model.html
 *
 * distance = height / ((1+((radius-center)/hwhm)^2 * (2^(1/shape)-1)) ^ shape)
 *
 * \param tracker A valid \ref PSMoveTracker handle
 * \param height The height parameter of the Pearson VII distribution
 * \param center The center parameter of the Pearson VII distribution
 * \param hwhm The hwhm parameter of the Pearson VII distribution
 * \param shape The shape parameter of the Pearson VII distribution
 **/
ADDAPI void
ADDCALL psmove_tracker_set_distance_parameters(PSMoveTracker *tracker,
        float height, float center, float hwhm, float shape);

/**
 * \brief Destroy an existing tracker instance and free allocated resources
 *
 * This will shut down the tracker, clean up all state information for
 * tracked controller as well as close the camera device. Return values
 * for functions returning data pointing to internal tracker data structures
 * will become invalid after this function call.
 *
 * \param tracker A valid \ref PSMoveTracker handle
 **/
ADDAPI void
ADDCALL psmove_tracker_free(PSMoveTracker *tracker);

#ifdef __cplusplus
}
#endif

#endif
