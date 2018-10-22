
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

//-- includes -----
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "psmove.h"
#include "psmove_private.h"
#include "psmove_orientation.h"

#include "math/psmove_quaternion.hpp"
#include "math/psmove_alignment.hpp"
#include "math/psmove_vector.h"

//-- constants -----
#define SAMPLE_FREQUENCY 120.f

// Madgwick MARG Filter Constants
#define gyroMeasDrift 3.14159265358979f * (0.9f / 180.0f) // gyroscope measurement error in rad/s/s (shown as 0.2f deg/s/s)
#define beta sqrtf(3.0f / 4.0f) * gyroMeasError // compute beta
#define gyroMeasError 3.14159265358979f * (1.5f / 180.0f) // gyroscope measurement error in rad/s (shown as 5 deg/s)
#define zeta sqrtf(3.0f / 4.0f) * gyroMeasDrift // compute zeta

// Complementary ARG Filter constants
#define k_base_earth_frame_align_weight 0.02f

const PSMove_3AxisTransform g_psmove_zero_transform = {{{ {0,0,0}, {0,0,0}, {0,0,0} }}};
const PSMove_3AxisTransform *k_psmove_zero_transform = &g_psmove_zero_transform;

// Calibration Pose transform
const PSMove_3AxisTransform g_psmove_identity_pose_upright = {{{ {1,0,0}, {0,1,0}, {0,0,1} }}};
const PSMove_3AxisTransform *k_psmove_identity_pose_upright = &g_psmove_identity_pose_upright;

const PSMove_3AxisTransform g_psmove_identity_pose_laying_flat = {{{ {1,0,0}, {0,0,-1}, {0,1,0} }}};
const PSMove_3AxisTransform *k_psmove_identity_pose_laying_flat = &g_psmove_identity_pose_laying_flat;

//Sensor Transforms
const PSMove_3AxisTransform g_psmove_sensor_transform_identity = {{{ {1,0,0}, {0,1,0}, {0,0,1} }}};
const PSMove_3AxisTransform *k_psmove_sensor_transform_identity = &g_psmove_sensor_transform_identity;

const PSMove_3AxisTransform g_psmove_sensor_transform_opengl = {{{ {1,0,0}, {0,0,1}, {0,-1,0} }}};
const PSMove_3AxisTransform *k_psmove_sensor_transform_opengl= &g_psmove_sensor_transform_opengl;

//-- structures ----
struct _PSMoveMadwickMARGState
{
    // estimate gyroscope biases error
    glm::quat omega_bias; 
};
typedef struct _PSMoveMadwickMARGState PSMoveMadgwickMARGState;

struct _PSMovComplementaryMARGState
{
    float mg_weight;
};
typedef struct _PSMovComplementaryMARGState PSMoveComplementaryMARGState;

struct _PSMoveOrientation {
    PSMove *move;

    /* Current sampling frequency */
    float sample_freq;

    /* Sample frequency measurements */
    long sample_freq_measure_start;
    long sample_freq_measure_count;

    /* Output value as quaternion */
    glm::quat quaternion;

    /* Quaternion measured when controller points towards camera */
    glm::quat reset_quaternion;

    /* Transforms the gravity and magnetometer calibration vectors recorded when the 
       controller was help upright during calibration. This is needed if you want the "identity pose" 
       to be something besides the one used during calibration */
    PSMove_3AxisTransform calibration_transform;

    /* Transforms the sensor data from PSMove Space to some user defined coordinate space */
    PSMove_3AxisTransform sensor_transform;

    /* Per filter type data */
    enum PSMoveOrientation_Fusion_Type fusion_type;
    struct
    {
        PSMoveMadgwickMARGState madgwick_marg_state;
        PSMoveComplementaryMARGState complementary_marg_state;
    } fusion_state;
};

//-- prototypes -----
static void _psmove_orientation_fusion_imu_update(
    PSMoveOrientation *orientation_state,
    float deltat,
    const glm::vec3 &sensor_gyroscope,
    const glm::vec3 &sensor_accelerometer);
static void _psmove_orientation_fusion_madgwick_marg_update(
    PSMoveOrientation *orientation_state,
    float deltat,
    const glm::vec3 &sensor_gyroscope,
    const glm::vec3 &sensor_accelerometer,
    const glm::vec3 &sensor_magnetometer);
static void _psmove_orientation_fusion_complementary_marg_update(
    PSMoveOrientation *orientation_state,
    float delta_t,
    const glm::vec3 &sensor_gyroscope,
    const glm::vec3 &sensor_acceleration,
    const glm::vec3 &sensor_magnetometer);

//-- public methods -----
PSMoveOrientation *
psmove_orientation_new(PSMove *move)
{
    psmove_return_val_if_fail(move != NULL, NULL);

    if (!psmove_has_calibration(move)) {
        psmove_DEBUG("Can't create orientation - no calibration!\n");
        return NULL;
    }

    PSMoveOrientation *orientation_state = (PSMoveOrientation *)calloc(1, sizeof(PSMoveOrientation));

    orientation_state->move = move;

    /* Initial sampling frequency */
    orientation_state->sample_freq = SAMPLE_FREQUENCY;

    /* Measurement */
    orientation_state->sample_freq_measure_start = psmove_util_get_ticks();
    orientation_state->sample_freq_measure_count = 0;

    /* Initial quaternion */
    orientation_state->quaternion = *k_psmove_quaternion_identity;
    orientation_state->reset_quaternion = *k_psmove_quaternion_identity;

    /* Initialize data specific to the selected filter */
    switch (psmove_get_model(move)) {
        case Model_ZCM1:
            psmove_orientation_set_fusion_type(orientation_state, OrientationFusion_ComplementaryMARG);
            break;
        case Model_ZCM2:
            // No magnetometer on the ZCM2
            psmove_orientation_set_fusion_type(orientation_state, OrientationFusion_MadgwickIMU);
            break;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            break;
    }

    /* Set the transform used re-orient the calibration data used by the orientation fusion algorithm */
    psmove_orientation_set_calibration_transform(orientation_state, k_psmove_identity_pose_laying_flat);

    /* Set the transform used re-orient the sensor data used by the orientation fusion algorithm */
    psmove_orientation_set_sensor_data_transform(orientation_state, k_psmove_sensor_transform_opengl);

    return orientation_state;
}

void
psmove_orientation_set_fusion_type(PSMoveOrientation *orientation_state, enum PSMoveOrientation_Fusion_Type fusion_type)
{
    orientation_state->fusion_type = fusion_type;

    switch (fusion_type)
    {
    case OrientationFusion_None:
    case OrientationFusion_MadgwickIMU:
        // No initialization
        break;
    case OrientationFusion_MadgwickMARG:
        {
            PSMoveMadgwickMARGState *marg_state = &orientation_state->fusion_state.madgwick_marg_state;

            marg_state->omega_bias = *k_psmove_quaternion_zero;
        }
        break;
    case OrientationFusion_ComplementaryMARG:
        {
            PSMoveComplementaryMARGState *marg_state = &orientation_state->fusion_state.complementary_marg_state;

            // Start off fully using the rotation from earth-frame.
            // Then drop down 
            marg_state->mg_weight = 1.f;
        }
        break;
    default:
        break;
    }
}

void
psmove_orientation_set_calibration_transform(PSMoveOrientation *orientation_state, const PSMove_3AxisTransform *transform)
{
    psmove_return_if_fail(orientation_state != NULL);
    psmove_return_if_fail(transform != NULL);

    orientation_state->calibration_transform= *transform;
}

void
psmove_orientation_set_sensor_data_transform(PSMoveOrientation *orientation_state, const PSMove_3AxisTransform *transform)
{
    psmove_return_if_fail(orientation_state != NULL);
    psmove_return_if_fail(transform != NULL);

    orientation_state->sensor_transform= *transform;
}

PSMove_3AxisVector
psmove_orientation_get_gravity_calibration_direction(PSMoveOrientation *orientation_state)
{
    psmove_return_val_if_fail(orientation_state != NULL, *k_psmove_vector_zero);

    PSMove_3AxisVector identity_g;
    psmove_get_identity_gravity_calibration_direction(orientation_state->move, &identity_g);

    // First apply the calibration data transform.
    // This allows us to pretend the "identity pose" was some other orientation the vertical during calibration
    identity_g= psmove_3axisvector_apply_transform(&identity_g, &orientation_state->calibration_transform);

    // Next apply the sensor data transform.
    // This allows us to pretend the sensors are in some other coordinate system (like OpenGL where +Y is up)
    identity_g= psmove_3axisvector_apply_transform(&identity_g, &orientation_state->sensor_transform);

    return identity_g;
}

PSMove_3AxisVector
psmove_orientation_get_magnetometer_calibration_direction(PSMoveOrientation *orientation_state)
{
    psmove_return_val_if_fail(orientation_state != NULL, *k_psmove_vector_zero);

    PSMove_3AxisVector identity_m;
    psmove_get_identity_magnetometer_calibration_direction(orientation_state->move, &identity_m);

    // First apply the calibration data transform.
    // This allows us to pretend the "identity pose" was some other orientation the vertical during calibration
    identity_m= psmove_3axisvector_apply_transform(&identity_m, &orientation_state->calibration_transform);

    // Next apply the sensor data transform.
    // This allows us to pretend the sensors are in some other coordinate system (like OpenGL where +Y is up)
    identity_m= psmove_3axisvector_apply_transform(&identity_m, &orientation_state->sensor_transform);

    return identity_m;
}

PSMove_3AxisVector
psmove_orientation_get_accelerometer_vector(PSMoveOrientation *orientation_state, enum PSMove_Frame frame)
{
    psmove_return_val_if_fail(orientation_state != NULL, *k_psmove_vector_zero);

    float ax, ay, az;
    psmove_get_accelerometer_frame(orientation_state->move, frame, &ax, &ay, &az);

    // Apply the "identity pose" transform
    PSMove_3AxisVector a = psmove_3axisvector_xyz(ax, ay, az);
    a= psmove_3axisvector_apply_transform(&a, &orientation_state->sensor_transform);

    return a;
}

PSMove_3AxisVector
psmove_orientation_get_accelerometer_normalized_vector(PSMoveOrientation *orientation_state, enum PSMove_Frame frame)
{
    psmove_return_val_if_fail(orientation_state != NULL, *k_psmove_vector_zero);

    PSMove_3AxisVector a= psmove_orientation_get_accelerometer_vector(orientation_state, frame);

    // Normalize the accelerometer vector
    psmove_3axisvector_normalize_with_default(&a, k_psmove_vector_zero);

    return a;
}

PSMove_3AxisVector
psmove_orientation_get_gyroscope_vector(PSMoveOrientation *orientation_state, enum PSMove_Frame frame)
{
    psmove_return_val_if_fail(orientation_state != NULL, *k_psmove_vector_zero);

    float omega_x, omega_y, omega_z;
    psmove_get_gyroscope_frame(orientation_state->move, frame, &omega_x, &omega_y, &omega_z);

    // Apply the "identity pose" transform
    PSMove_3AxisVector omega = psmove_3axisvector_xyz(omega_x, omega_y, omega_z);
    omega= psmove_3axisvector_apply_transform(&omega, &orientation_state->sensor_transform);

    return omega;
}

PSMove_3AxisVector
psmove_orientation_get_magnetometer_normalized_vector(PSMoveOrientation *orientation_state)
{
    psmove_return_val_if_fail(orientation_state != NULL, *k_psmove_vector_zero);

    PSMove_3AxisVector m;
    psmove_get_magnetometer_3axisvector(orientation_state->move, &m);
    m= psmove_3axisvector_apply_transform(&m, &orientation_state->sensor_transform);

    // Make sure we always return a normalized direction
    psmove_3axisvector_normalize_with_default(&m, k_psmove_vector_zero);

    return m;
}

void
psmove_orientation_update(PSMoveOrientation *orientation_state)
{
    psmove_return_if_fail(orientation_state != NULL);

    int frame_half;
    long now = psmove_util_get_ticks();

    if (now - orientation_state->sample_freq_measure_start >= 1000) 
    {
        float measured = ((float)orientation_state->sample_freq_measure_count) /
            ((float)(now-orientation_state->sample_freq_measure_start))*1000.f;
        psmove_DEBUG("Measured sample_freq: %f\n", measured);

        orientation_state->sample_freq = measured;
        orientation_state->sample_freq_measure_start = now;
        orientation_state->sample_freq_measure_count = 0;
    }

    /* We get 2 measurements per call to psmove_poll() */
    orientation_state->sample_freq_measure_count += 2;

    glm::quat quaternion_backup = orientation_state->quaternion;
    float deltaT = 1.f / fmax(orientation_state->sample_freq, SAMPLE_FREQUENCY); // time delta = 1/frequency

    for (frame_half=0; frame_half<2; frame_half++) 
    {
        switch (orientation_state->fusion_type)
        {
        case OrientationFusion_None:
            break;
        case OrientationFusion_MadgwickIMU:
            {
                // Get the sensor data transformed by the sensor_transform
                PSMove_3AxisVector a= 
                    psmove_orientation_get_accelerometer_normalized_vector(orientation_state, (enum PSMove_Frame)(frame_half));
                PSMove_3AxisVector omega= 
                    psmove_orientation_get_gyroscope_vector(orientation_state, (enum PSMove_Frame)(frame_half));

                // Apply the filter
                _psmove_orientation_fusion_imu_update(
                    orientation_state,
                    deltaT,
                    /* Gyroscope */
                    glm::vec3(omega.x, omega.y, omega.z),
                    /* Accelerometer */
                    glm::vec3(a.x, a.y, a.z));
            }
            break;
        case OrientationFusion_MadgwickMARG:
            {
                PSMove_3AxisVector m = psmove_orientation_get_magnetometer_normalized_vector(orientation_state);
                PSMove_3AxisVector a = psmove_orientation_get_accelerometer_normalized_vector(orientation_state, (enum PSMove_Frame)(frame_half));
                PSMove_3AxisVector omega = psmove_orientation_get_gyroscope_vector(orientation_state, (enum PSMove_Frame)(frame_half));
                _psmove_orientation_fusion_madgwick_marg_update(orientation_state, deltaT,
                                                                /* Gyroscope */
                                                                glm::vec3(omega.x, omega.y, omega.z),
                                                                /* Accelerometer */
                                                                glm::vec3(a.x, a.y, a.z),
                                                                /* Magnetometer */
                                                                glm::vec3(m.x, m.y, m.z));
            }
            break;
        case OrientationFusion_ComplementaryMARG:
            {
                PSMove_3AxisVector m= 
                    psmove_orientation_get_magnetometer_normalized_vector(orientation_state);
                PSMove_3AxisVector a= 
                    psmove_orientation_get_accelerometer_normalized_vector(orientation_state, (enum PSMove_Frame)(frame_half));
                PSMove_3AxisVector omega= 
                    psmove_orientation_get_gyroscope_vector(orientation_state, (enum PSMove_Frame)(frame_half));

                // Apply the filter
                _psmove_orientation_fusion_complementary_marg_update(
                    orientation_state,
                    deltaT,
                    /* Gyroscope */
                    glm::vec3(omega.x, omega.y, omega.z),
                    /* Accelerometer */
                    glm::vec3(a.x, a.y, a.z),
                    /* Magnetometer */
                    glm::vec3(m.x, m.y, m.z));
            }
            break;
        }

        if (!psmove_quaternion_is_valid(orientation_state->quaternion)) 
        {
            psmove_DEBUG("Orientation is NaN!");
            orientation_state->quaternion = quaternion_backup;
        }
    }
}

void
psmove_orientation_get_quaternion(PSMoveOrientation *orientation_state,
        float *q0, float *q1, float *q2, float *q3)
{
    psmove_return_if_fail(orientation_state != NULL);

    const glm::quat &reset_quaternion = orientation_state->reset_quaternion;
    const glm::quat &current_quaternion = orientation_state->quaternion;
    glm::quat result= reset_quaternion * current_quaternion;

    if (q0) {
        *q0 = result.w;
    }

    if (q1) {
        *q1 = result.x;
    }

    if (q2) {
        *q2 = result.y;
    }

    if (q3) {
        *q3 = result.z;
    }
}

void
psmove_orientation_reset_quaternion(PSMoveOrientation *orientation_state)
{
    psmove_return_if_fail(orientation_state != NULL);

    glm::quat q_inverse = glm::conjugate(orientation_state->quaternion);

    psmove_quaternion_normalize_with_default(q_inverse, *k_psmove_quaternion_identity);
    orientation_state->reset_quaternion= q_inverse;
}

void
psmove_orientation_free(PSMoveOrientation *orientation_state)
{
    psmove_return_if_fail(orientation_state != NULL);

    free(orientation_state);
}

// -- Orientation Filters ----

// This algorithm comes from Sebastian O.H. Madgwick's 2010 paper:
// "An efficient orientation filter for inertial and inertial/magnetic sensor arrays"
// https://www.samba.org/tridge/UAV/madgwick_internal_report.pdf
static void _psmove_orientation_fusion_imu_update(
    PSMoveOrientation *orientation_state,
    float delta_t,
    const glm::vec3 &current_omega,
    const glm::vec3 &current_g)
{
    // Current orientation from earth frame to sensor frame
    glm::quat SEq = orientation_state->quaternion;
    glm::quat SEq_new = SEq;

    // Compute the quaternion derivative measured by gyroscopes
    // Eqn 12) q_dot = 0.5*q*omega
    glm::quat omega = glm::quat(0.f, current_omega.x, current_omega.y, current_omega.z);
    glm::quat SEqDot_omega = (SEq * 0.5f) *omega;

    if (is_nearly_equal(glm::dot(current_g, current_g), 0.f, k_normal_epsilon*k_normal_epsilon))
    {
        // Get the direction of the gravitational fields in the identity pose		
        PSMove_3AxisVector identity_g= psmove_orientation_get_gravity_calibration_direction(orientation_state);
        glm::vec3 k_identity_g_direction = glm::vec3(identity_g.x, identity_g.y, identity_g.z);

        // Eqn 15) Applied to the gravity vector
        // Fill in the 3x1 objective function matrix f(SEq, Sa) =|f_g|
        glm::vec3 f_g;
        psmove_alignment_compute_objective_vector(SEq, k_identity_g_direction, current_g, f_g, NULL);

        // Eqn 21) Applied to the gravity vector
        // Fill in the 4x3 objective function Jacobian matrix: J_gb(SEq)= [J_g]
        glm::mat3x4 J_g;
        psmove_alignment_compute_objective_jacobian(SEq, k_identity_g_direction, J_g);

        // Eqn 34) gradient_F= J_g(SEq)*f(SEq, Sa)
        // Compute the gradient of the objective function
        glm::vec4 gradient_f = J_g * f_g;
        glm::quat SEqHatDot =
            glm::quat(gradient_f[0], gradient_f[1], gradient_f[2], gradient_f[3]);

        // normalize the gradient
        psmove_quaternion_normalize_with_default(SEqHatDot, *k_psmove_quaternion_zero);

        // Compute the estimated quaternion rate of change
        // Eqn 43) SEq_est = SEqDot_omega - beta*SEqHatDot
        glm::quat SEqDot_est = SEqDot_omega + SEqHatDot*(-beta);

        // Compute then integrate the estimated quaternion rate
        // Eqn 42) SEq_new = SEq + SEqDot_est*delta_t
        SEq_new = SEq + SEqDot_est*delta_t;
    }
    else
    {
        SEq_new = SEq + SEqDot_omega*delta_t;
    }

    // Make sure the net quaternion is a pure rotation quaternion
    SEq_new= glm::normalize(SEq_new);

    // Save the new quaternion back into the orientation state
    orientation_state->quaternion= SEq_new;
}

// This algorithm comes from Sebastian O.H. Madgwick's 2010 paper:
// "An efficient orientation filter for inertial and inertial/magnetic sensor arrays"
// https://www.samba.org/tridge/UAV/madgwick_internal_report.pdf
static void 
_psmove_orientation_fusion_madgwick_marg_update(
    PSMoveOrientation *orientation_state,
    float delta_t,
    const glm::vec3 &current_omega,
    const glm::vec3 &current_g,
    const glm::vec3 &current_m)
{
    // If there isn't a valid magnetometer or accelerometer vector, fall back to the IMU style update
    if (is_nearly_equal(glm::dot(current_g, current_g), 0.f, k_normal_epsilon*k_normal_epsilon) ||
        is_nearly_equal(glm::dot(current_m, current_m), 0.f, k_normal_epsilon*k_normal_epsilon))
    {
        _psmove_orientation_fusion_imu_update(
            orientation_state,
            delta_t,
            current_omega,
            current_g);
        return;
    }

    PSMoveMadgwickMARGState *marg_state = &orientation_state->fusion_state.madgwick_marg_state;

    // Current orientation from earth frame to sensor frame
    glm::quat SEq = orientation_state->quaternion;

    // Get the direction of the magnetic fields in the identity pose.	
    // NOTE: In the original paper we converge on this vector over time automatically (See Eqn 45 & 46)
    // but since we've already done the work in calibration to get this vector, let's just use it.
    // This also removes the last assumption in this function about what 
    // the orientation of the identity-pose is (handled by the sensor transform).
    PSMove_3AxisVector identity_m= psmove_orientation_get_magnetometer_calibration_direction(orientation_state);
    glm::vec3 k_identity_m_direction = glm::vec3(identity_m.x, identity_m.y, identity_m.z);

    // Get the direction of the gravitational fields in the identity pose
    PSMove_3AxisVector identity_g= psmove_orientation_get_gravity_calibration_direction(orientation_state);
    glm::vec3 k_identity_g_direction = glm::vec3(identity_g.x, identity_g.y, identity_g.z);

    // Eqn 15) Applied to the gravity and magnetometer vectors
    // Fill in the 6x1 objective function matrix f(SEq, Sa, Eb, Sm) =|f_g|
    //                                                               |f_b|
    glm::vec3 f_g;
    psmove_alignment_compute_objective_vector(SEq, k_identity_g_direction, current_g, f_g, NULL);

    glm::vec3 f_m;
    psmove_alignment_compute_objective_vector(SEq, k_identity_m_direction, current_m, f_m, NULL);

    // Eqn 21) Applied to the gravity and magnetometer vectors
    // Fill in the 4x6 objective function Jacobian matrix: J_gb(SEq, Eb)= [J_g|J_b]
    glm::mat3x4 J_g;
    psmove_alignment_compute_objective_jacobian(SEq, k_identity_g_direction, J_g);

    glm::mat3x4 J_m;
    psmove_alignment_compute_objective_jacobian(SEq, k_identity_m_direction, J_m);

    // Eqn 34) gradient_F= J_gb(SEq, Eb)*f(SEq, Sa, Eb, Sm)
    // Compute the gradient of the objective function
    glm::vec4 gradient_f = J_g*f_g + J_m*f_m;
    glm::quat SEqHatDot =
        glm::quat(gradient_f[0], gradient_f[1], gradient_f[2], gradient_f[3]);

    // normalize the gradient to estimate direction of the gyroscope error
    psmove_quaternion_normalize_with_default(SEqHatDot, *k_psmove_quaternion_zero);

    // Eqn 47) omega_err= 2*SEq*SEqHatDot
    // compute angular estimated direction of the gyroscope error
    glm::quat omega_err = (SEq*2.f) * SEqHatDot;

    // Eqn 48) net_omega_bias+= zeta*omega_err
    // Compute the net accumulated gyroscope bias
    glm::quat omega_bias= marg_state->omega_bias;
    omega_bias = omega_bias + omega_err*zeta*delta_t;
    omega_bias.w = 0.f; // no bias should accumulate on the w-component
    marg_state->omega_bias= omega_bias;

    // Eqn 49) omega_corrected = omega - net_omega_bias
    glm::quat omega = glm::quat(0.f, current_omega.x, current_omega.y, current_omega.z);
    glm::quat corrected_omega = omega + (-omega_bias);

    // Compute the rate of change of the orientation purely from the gyroscope
    // Eqn 12) q_dot = 0.5*q*omega
    glm::quat SEqDot_omega = (SEq * 0.5f) * corrected_omega;

    // Compute the estimated quaternion rate of change
    // Eqn 43) SEq_est = SEqDot_omega - beta*SEqHatDot
    glm::quat SEqDot_est = SEqDot_omega + SEqHatDot*(-beta);

    // Compute then integrate the estimated quaternion rate
    // Eqn 42) SEq_new = SEq + SEqDot_est*delta_t
    glm::quat SEq_new = SEq + SEqDot_est*delta_t;

    // Make sure the net quaternion is a pure rotation quaternion
    SEq_new= glm::normalize(SEq_new);

    // Save the new quaternion back into the orientation state
    orientation_state->quaternion= SEq_new;
}

static void
_psmove_orientation_fusion_complementary_marg_update(
    PSMoveOrientation *orientation_state,
    float delta_t,
    const glm::vec3 &current_omega,
    const glm::vec3 &current_g,
    const glm::vec3 &current_m)
{
    // Get the direction of the magnetic fields in the identity pose	
    PSMove_3AxisVector identity_m= psmove_orientation_get_magnetometer_calibration_direction(orientation_state);
    glm::vec3 k_identity_m_direction = glm::vec3(identity_m.x, identity_m.y, identity_m.z);

    // Get the direction of the gravitational field in the identity pose
    PSMove_3AxisVector identity_g= psmove_orientation_get_gravity_calibration_direction(orientation_state);
    glm::vec3 k_identity_g_direction = glm::vec3(identity_g.x, identity_g.y, identity_g.z);

    // Angular Rotation (AR) Update
    //-----------------------------
    // Compute the rate of change of the orientation purely from the gyroscope
    // q_dot = 0.5*q*omega
    glm::quat q_current= orientation_state->quaternion;

    glm::quat q_omega = glm::quat(0.f, current_omega.x, current_omega.y, current_omega.z);
    glm::quat q_derivative = (q_current*0.5f) * q_omega;

    // Integrate the rate of change to get a new orientation
    // q_new= q + q_dot*dT
    glm::quat q_step = q_derivative * delta_t;
    glm::quat ar_orientation = q_current + q_step;

    // Make sure the resulting quaternion is normalized
    ar_orientation= glm::normalize(ar_orientation);

    // Magnetic/Gravity (MG) Update
    //-----------------------------
    const glm::vec3* mg_from[2] = { &k_identity_g_direction, &k_identity_m_direction };
    const glm::vec3* mg_to[2] = { &current_g, &current_m };
    glm::quat mg_orientation;

    // Always attempt to align with the identity_mg, even if we don't get within the alignment tolerance.
    // More often then not we'll be better off moving forward with what we have and trying to refine
    // the alignment next frame.
    psmove_alignment_quaternion_between_vector_frames(
        mg_from, mg_to, 0.1f, q_current, mg_orientation);

    // Blending Update
    //----------------
    // The final rotation is a blend between the integrated orientation and absolute rotation from the earth-frame
    float mg_wight = orientation_state->fusion_state.complementary_marg_state.mg_weight;
    orientation_state->quaternion= 
        psmove_quaternion_normalized_lerp(ar_orientation, mg_orientation, mg_wight);

    // Update the blend weight
    orientation_state->fusion_state.complementary_marg_state.mg_weight =
        lerp_clampf(mg_wight, k_base_earth_frame_align_weight, 0.9f);
}
