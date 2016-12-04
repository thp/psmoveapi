
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

//-- includes ----
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "psmove.h"
#include "psmove_orientation.h"
#include "math/psmove_vector.h"

//-- constants -----
#define LED_RANGE_TARGET 320.f
#define STABILIZE_WAIT_TIME_MS 1000
#define DESIRED_MAGNETOMETER_SAMPLE_COUNT 100

enum eCalibrationState
{
	Calibration_MeasureBExtents,
	Calibration_WaitForGravityAlignment,
	Calibration_MeasureBDirection,
	Calibration_Complete,
};

//-- prototypes -----
static bool is_move_stable_and_aligned_with_gravity(PSMove *move);

//-- public methods ----
int
main(int arg, char** args)
{
	if (!psmove_init(PSMOVE_CURRENT_VERSION)) {
		fprintf(stderr, "PS Move API init failed (wrong version?)\n");
		exit(1);
	}

    int i;
    int count = psmove_count_connected();

    printf("Connected controllers for calibration: %d\n", count);

    for (i=0; i<count; i++) 
	{
        PSMove *move = psmove_connect_by_id(i);

        if (move == NULL)
        {
            printf("Failed to open PS Move #%d\n", i);
            continue;
        }

		// Make sure accelerometer is calibrated
		// Otherwise we can't tell if the controller is sitting upright
		if (psmove_has_calibration(move))
		{
			if (psmove_connection_type(move) == Conn_Bluetooth)
			{
				float old_range = 0;
				enum eCalibrationState calibration_state = Calibration_MeasureBExtents;

				// Reset existing magnetometer calibration state
				psmove_reset_magnetometer_calibration(move);

				printf("Calibrating PS Move #%d\n", i);
				printf("[Step 1 of 2: Measuring extents of the magnetometer]\n");
				printf("Rotate the controller in all directions.\n");
				fflush(stdout);

				while (calibration_state == Calibration_MeasureBExtents)
				{
					if (psmove_poll(move))
					{
						unsigned int pressed, released;
						psmove_get_button_events(move, &pressed, &released);

						/* This call updates min/max values if exceeding previously stored values */
						psmove_get_magnetometer_3axisvector(move, NULL);

						/* Returns the minimum delta (max-min) across dimensions (x, y, z) */
						float range = psmove_get_magnetometer_calibration_range(move);

						/* Update the LEDs to indicate progress */
						int percentage = (int)(100.f * range / LED_RANGE_TARGET);
						if (percentage > 100)
						{
							percentage = 100;
						}
						else if (percentage < 0)
						{
							percentage = 0;
						}

						psmove_set_leds(
							move,
							(unsigned char)((255 * (100 - percentage)) / 100),
							(unsigned char)((255 * percentage) / 100),
							0);
						psmove_update_leds(move);

						if (pressed & Btn_MOVE)
						{
							psmove_set_leds(move, 0, 0, 0);
							psmove_update_leds(move);

							// Move on to the 
							calibration_state = Calibration_WaitForGravityAlignment;
						}
						else if (range > old_range)
						{
							old_range = range;
							printf("\rPress the MOVE button when value stops changing: %.1f", range);
							fflush(stdout);
						}
					}
				}

				printf("\n\n[Step 2 of 2: Measuring default magnetic field direction]\n");
				printf("Stand the controller on a level surface with the Move button facing you.\n");
				printf("This will be the default orientation of the move controller.\n");
				printf("Measurement will start once the controller is aligned with gravity and stable.\n");
				fflush(stdout);

				//TODO: Wait for accelerometer stabilization and button press
				// Sample the magnetometer field for several frames and average
				// Store as the default magnetometer direction
				while (calibration_state != Calibration_Complete)
				{
					int stable_start_time = psmove_util_get_ticks();
					PSMove_3AxisVector identity_pose_average_m_vector = *k_psmove_vector_zero;
					int sample_count = 0;

					while (calibration_state == Calibration_WaitForGravityAlignment)
					{
						if (psmove_poll(move))
						{
							if (is_move_stable_and_aligned_with_gravity(move))
							{
								int current_time = psmove_util_get_ticks();
								int stable_duration = (current_time - stable_start_time);

								if ((current_time - stable_start_time) >= STABILIZE_WAIT_TIME_MS)
								{
									calibration_state = Calibration_MeasureBDirection;
								}
								else
								{
									printf("\rStable for: %dms/%dms                        ", stable_duration, STABILIZE_WAIT_TIME_MS);
								}
							}
							else
							{
								stable_start_time = psmove_util_get_ticks();
								printf("\rMove Destabilized! Waiting for stabilization.");
							}
						}
					}

					printf("\n\nMove stabilized. Starting magnetometer sampling.\n");
					while (calibration_state == Calibration_MeasureBDirection)
					{
						if (psmove_poll(move))
						{
							if (is_move_stable_and_aligned_with_gravity(move))
							{
								PSMove_3AxisVector m;

								psmove_get_magnetometer_3axisvector(move, &m);
								psmove_3axisvector_normalize_with_default(&m, k_psmove_vector_zero);

								identity_pose_average_m_vector = psmove_3axisvector_add(&identity_pose_average_m_vector, &m);
								sample_count++;

								if (sample_count > DESIRED_MAGNETOMETER_SAMPLE_COUNT)
								{
									float N = (float)sample_count;

									// The average magnetometer direction was recorded while the controller
									// was in the cradle pose
									identity_pose_average_m_vector = psmove_3axisvector_divide_by_scalar_unsafe(&identity_pose_average_m_vector, N);

									psmove_set_magnetometer_calibration_direction(move, &identity_pose_average_m_vector);

									calibration_state = Calibration_Complete;
								}
								else
								{
									printf("\rMagnetometer Sample: %d/%d                   ", sample_count, DESIRED_MAGNETOMETER_SAMPLE_COUNT);
								}
							}
							else
							{
								calibration_state = Calibration_WaitForGravityAlignment;
								printf("\rMove Destabilized! Waiting for stabilization.\n");
							}
						}
					}
				}

				psmove_save_magnetometer_calibration(move);
			}
			else
			{
				printf("Ignoring non-Bluetooth PS Move #%d\n", i);
			}
		}
		else
		{			
			char *serial= psmove_get_serial(move);

			printf("\nController #%d has bad calibration file (accelerometer values won't be correct).\n", i);

			if (serial != NULL)
			{
				printf("Please delete %s.calibration and re-run \"psmove pair\" with the controller plugged into usb.", serial);
				free(serial);
			}
			else
			{
				printf("Please re-run \"psmove pair\" with the controller plugged into usb.");
			}
		}

        printf("\nFinished PS Move #%d\n", i);
        psmove_disconnect(move);
    }

    return 0;
}

//-- private methods -----
static bool
is_move_stable_and_aligned_with_gravity(PSMove *move)
{
	const float k_cosine_10_degrees = 0.984808f;

	PSMove_3AxisVector k_identity_gravity_vector;
	PSMove_3AxisVector acceleration_direction;
	float acceleration_magnitude;
	bool isOk;

	// Get the direction the gravity vector should be pointing 
	// while the controller is in cradle pose.
	psmove_get_identity_gravity_calibration_direction(move, &k_identity_gravity_vector);
	psmove_get_accelerometer_frame(move, Frame_SecondHalf, 
		&acceleration_direction.x, &acceleration_direction.y, &acceleration_direction.z);
	acceleration_magnitude = psmove_3axisvector_normalize_with_default(&acceleration_direction, k_psmove_vector_zero);

	isOk =
		is_nearly_equal(1.f, acceleration_magnitude, 0.1f) &&
		psmove_3axisvector_dot(&k_identity_gravity_vector, &acceleration_direction) >= k_cosine_10_degrees;

	return isOk;
}
