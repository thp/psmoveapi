#include <stdio.h>
#include "psmove_vector_math.h"

void
psmove_vector_rotate(float vector[3], float quaternion[4], float product[3])
{
    // https://github.com/jrowberg/keyglove/blob/master/controller/arduino/keyglove/support_helper_3dmath.cpp#L177
    float vector_as_quaternion[4] = {
        0,
        vector[0],
        vector[1],
        vector[2]
    };

    float result1[4];
    float result2[4];

    float conjugate[4];
    psmove_quaternion_conjugate(quaternion, conjugate);

    psmove_quaternion_times_quaternion(quaternion, vector_as_quaternion, result1);
    psmove_quaternion_times_quaternion(result1, conjugate, result2);

    product[0] = result2[1];
    product[1] = result2[2];
    product[2] = result2[3];
}

void
psmove_quaternion_times_quaternion(float q1[4], float q2[4], float product[4])
{
  product[0] = q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2] - q1[3]*q2[3];
  product[1] = q1[0]*q2[1] + q1[1]*q2[0] + q1[2]*q2[3] - q1[3]*q2[2];
  product[2] = q1[0]*q2[2] - q1[1]*q2[3] + q1[2]*q2[0] + q1[3]*q2[1];
  product[3] = q1[0]*q2[3] + q1[1]*q2[2] - q1[2]*q2[1] + q1[3]*q2[0];
}

void
psmove_quaternion_conjugate(float quaternion[4], float conjugate[4])
{
    conjugate[0] = quaternion[0];
    conjugate[1] = -quaternion[1];
    conjugate[2] = -quaternion[2];
    conjugate[3] = -quaternion[3];
}

void
psmove_vector_plus_vector(float vector1[3], float vector2[3], float difference[3])
{
    difference[0] = vector1[0] + vector2[0];
    difference[1] = vector1[1] + vector2[1];
    difference[2] = vector1[2] + vector2[2];
}

void
psmove_vector_minus_vector(float vector1[3], float vector2[3], float difference[3])
{
    difference[0] = vector1[0] - vector2[0];
    difference[1] = vector1[1] - vector2[1];
    difference[2] = vector1[2] - vector2[2];
}
