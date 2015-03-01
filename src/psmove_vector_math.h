/**
 * ALL Quaternions in [w, x, y, z] notation!
 */
void psmove_vector_rotate(float vector[3], float quaternion[4], float product[3]);

void psmove_quaternion_times_quaternion(float q1[4], float q2[4], float product[4]);

void psmove_quaternion_conjugate(float quaternion[4], float conjugate[4]);

void psmove_vector_plus_vector(float vector1[3], float vector2[3], float difference[3]);

void psmove_vector_minus_vector(float vector1[3], float vector2[3], float difference[3]);
