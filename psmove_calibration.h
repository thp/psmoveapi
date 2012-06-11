
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


#ifndef PSMOVE_CALIBRATION_H
#define PSMOVE_CALIBRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "psmove.h"


/* Positions in the 6-point tumble test */
#define PSMOVE_CALIBRATION_POSITIONS 6

/* Calibration fields for each position (3x accel, 3x gyro, 3x magneto) */
#define PSMOVE_CALIBRATION_FIELDS 9

enum PSMove_Calibration_Method {
    Calibration_None = 0,
    Calibration_Any,
    Calibration_USB,
    Calibration_Custom,
};

typedef struct _PSMoveCalibration PSMoveCalibration;

ADDAPI PSMoveCalibration *
ADDCALL psmove_calibration_new(PSMove *move);

/**
 * Read calibration data from controller storage via USB
 *
 * calibration ... a valid PSMoveCalibration * instance.
 *
 * The calibration data blob will be read from the controller and will be
 * stored inside this calibration object for future use in value mapping.
 *
 * Returns nonzero on success, zero on error (e.g. not connected via USB)
 **/
ADDAPI int
ADDCALL psmove_calibration_read_from_usb(PSMoveCalibration *calibration);


/**
 * Set custom calibration data from 6 point tumble test.
 *
 * calibration ... a valid PSMoveCalibration * instance.
 * positions ... pointer to a two-dimensional array with calibration values.
 * n_positions ... must be 6.
 * n_fields ... must be 9.
 *
 * The positions array is usually a float[6*9]-array containing sensor
 * readings (3x accel, 3x gyro, 3x magneto) for each of the 6 positions,
 * like this: p1.ax, p1.ay, p1.az, p1.gx, ..., p1.mx, p2.ax, ..., p6.mz
 **/
ADDAPI void
ADDCALL psmove_calibration_set_custom(PSMoveCalibration *calibration,
        float *positions, size_t n_positions, size_t n_fields);


/**
 * Dump calibration information to stdout.
 *
 * This ist mostly used for debugging. Don't rely on this function to
 * be available in the future - it might be removed.
 **/
ADDAPI void
ADDCALL psmove_calibration_dump(PSMoveCalibration *calibration);

/**
 * Map raw sensor values to calibrated values.
 *
 * calibration ... a valid PSMoveCalibration * instance.
 * preferred ... the preferred method for mapping values (Any, USB, Custom).
 * input ... pointer to a n-array containing raw values.
 * output ... pointer to a n-array to store output values.
 * n ... 3 (accel only), 6 (accel+gyro) or 9 (accel+gyro+magnetometer)
 *
 * Returns Calibration_None if no calibration data is found, else
 * the method used to map the input data to the output data.
 **/
ADDAPI enum PSMove_Calibration_Method
ADDCALL psmove_calibration_map(PSMoveCalibration *calibration,
        enum PSMove_Calibration_Method preferred,
        int *input, float *output, size_t n);

/**
 * Check if a calibration object supports a given method.
 *
 * calibration ... a valid PSMoveCalibration * instance.
 * method ... the method that needs to be checked (USB or Custom).
 *
 * Returns nonzero if the method is supported, zero otherwise (and on error).
 **/
ADDAPI int
ADDCALL psmove_calibration_supports_method(PSMoveCalibration *calibration,
        enum PSMove_Calibration_Method method);

/**
 * Load the calibration from persistent storage.
 *
 * Returns nonzero on success, zero on error.
 **/
ADDAPI int
ADDCALL psmove_calibration_load(PSMoveCalibration *calibration);


/**
 * Save the calibration to persistent storage.
 *
 * Returns nonzero on success, zero on error.
 **/
ADDAPI int
ADDCALL psmove_calibration_save(PSMoveCalibration *calibration);

/**
 * Destroy a PSMoveCalibration * instance and free() associated memory.
 **/
ADDAPI void
ADDCALL psmove_calibration_destroy(PSMoveCalibration *calibration);


#ifdef __cplusplus
}
#endif

#endif
