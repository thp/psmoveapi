//=====================================================================================================
// MadgwickAHRS.h
//=====================================================================================================
//
// Implementation of Madgwick's IMU and AHRS algorithms.
// See: http://www.x-io.co.uk/node/8#open_source_ahrs_and_imu_algorithms
//
// Date			Author          Notes
// 29/09/2011	SOH Madgwick    Initial release
// 02/10/2011	SOH Madgwick	Optimised for reduced CPU load
// 02/08/2012   Thomas Perl     Modifications for PS Move API integration
//
//=====================================================================================================
#ifndef MadgwickAHRS_h
#define MadgwickAHRS_h

#ifdef __cplusplus
extern "C" {
#endif


//----------------------------------------------------------------------------------------------------
// Variable declaration

extern volatile float beta;				// algorithm gain

//---------------------------------------------------------------------------------------------------
// Function declarations

// quaternion -> pointer to a float[4]
// sampleFreq -> sample frequency in Hz

void MadgwickAHRSupdate(float *quaternion, float sampleFreq,
        float ax, float ay, float az,
        float gx, float gy, float gz,
        float mx, float my, float mz);

void MadgwickAHRSupdateIMU(float *quaternion, float sampleFreq,
        float ax, float ay, float az,
        float gx, float gy, float gz);

#ifdef __cplusplus
}
#endif

#endif
//=====================================================================================================
// End of file
//=====================================================================================================
