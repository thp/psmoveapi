#pragma once

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



#ifdef __cplusplus
extern "C" {
#endif

//-- includes -----
#include "psmove.h"
#include "math/psmove_vector.h"

//-- pre-declarations -----
struct _PSMoveOrientation;
typedef struct _PSMoveOrientation PSMoveOrientation;

//-- constants -----
extern const PSMove_3AxisTransform *k_psmove_zero_transform;

/*! Transforms used by psmove_set_orientation_calibration_transform */
extern const PSMove_3AxisTransform *k_psmove_identity_pose_upright;
extern const PSMove_3AxisTransform *k_psmove_identity_pose_laying_flat;

/*! Transforms used by psmove_set_orientation_sensor_data_transform */
extern const PSMove_3AxisTransform *k_psmove_sensor_transform_identity;
extern const PSMove_3AxisTransform *k_psmove_sensor_transform_opengl;

//-- interface -----
ADDAPI PSMoveOrientation *
ADDCALL psmove_orientation_new(PSMove *move);

ADDAPI void
ADDCALL psmove_orientation_free(PSMoveOrientation *orientation_state);

ADDAPI void
ADDCALL psmove_orientation_set_fusion_type(PSMoveOrientation *orientation_state, enum PSMoveOrientation_Fusion_Type fusion_type);

ADDAPI void
ADDCALL psmove_orientation_set_calibration_transform(PSMoveOrientation *orientation_state, const PSMove_3AxisTransform *transform);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_orientation_get_gravity_calibration_direction(PSMoveOrientation *orientation_state);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_orientation_get_magnetometer_calibration_direction(PSMoveOrientation *orientation_state);

ADDAPI void
ADDCALL psmove_orientation_set_sensor_data_transform(PSMoveOrientation *orientation_state, const PSMove_3AxisTransform *transform);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_orientation_get_accelerometer_vector(PSMoveOrientation *orientation_state, enum PSMove_Frame frame);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_orientation_get_accelerometer_normalized_vector(PSMoveOrientation *orientation_state, enum PSMove_Frame frame);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_orientation_get_gyroscope_vector(PSMoveOrientation *orientation_state, enum PSMove_Frame frame);

ADDAPI PSMove_3AxisVector
ADDCALL psmove_orientation_get_magnetometer_normalized_vector(PSMoveOrientation *orientation_state);

ADDAPI void
ADDCALL psmove_orientation_update(PSMoveOrientation *orientation_state);

ADDAPI void
ADDCALL psmove_orientation_get_quaternion(PSMoveOrientation *orientation_state,
        float *q0, float *q1, float *q2, float *q3);

ADDAPI void
ADDCALL psmove_orientation_reset_quaternion(PSMoveOrientation *orientation_state);

#ifdef __cplusplus
}
#endif
