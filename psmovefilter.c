/// <summary>
/// Used to represent the X/Y/Z axes in 3D space.
/// </summary>
#include "math.h"

typedef struct 
{
	double x;
	double y;
	double z;
} Vector3;

			
Vector3 normalize3DVector(Vector3 vector)
{
	// Calculate pythagoras in 3D
	double R = (double)sqrt(vector.x*vector.x + vector.y*vector.y + vector.z*vector.z);
	Vector3 newVector = {vector.x/R, vector.y/R, vector.z/R};
	// Apply ratio R to all elements, resulting in a value from 0-1
	return newVector;
}
		
double squared(double x)
{
	return (double)(x*x);
}

/*
	Vector3 operator +(Vector3 a, Vector3 b)
	{
		return new Vector3(a.x+b.x, a.y+b.y, a.z+b.z);
	}
	Vector3 operator -(Vector3 a, Vector3 b)
	{
		return new Vector3(a.x-b.x, a.y-b.y, a.z-b.z);
	}
	Vector3 operator /(Vector3 a, double b)
	{
		return new Vector3(a.x/b, a.y/b, a.z/b);
	}
*/

Vector3 RwEst  = {0,0,0}; // Filtered value

double configwGyro = 10.0f;

//bool firstSample = true;

Vector3 RwAcc  = {0,-1,0}; // Accelerometer reading
Vector3 RwGyro = {0,0,0}; // Gyro Previous reading?
Vector3 Awz = {0,0,0}; // Awz.x = Projected angle in the XZ plane / Awz.y = Projected angle in the YZ plane / Awz.z = (unused)

const double pi = 3.141592;
int firstSample = 1;

void psmove_filter_imu2(double gx, double gy, double gz, double ax, double ay, double az, double dt)
{
	double tmpf;  
	int signRzGyro;  

	Vector3 theVector = {ax,ay,az};

	RwAcc = normalize3DVector(theVector);

	if (firstSample != 0)
	{
		RwEst = RwAcc;
		firstSample = 0;
		return;
	}

	//evaluate RwGyro vector
	if(abs(RwEst.z) < 0.1)
	{
		//Rz is too small and because it is used as reference for computing Axz, Ayz it's error fluctuations will amplify leading to bad results
		//in this case skip the gyro data and just use previous estimate
		RwGyro = RwEst;
	}
	else
	{
		//get angles between projection of R on ZX/ZY plane and Z axis, based on last RwEst
		tmpf = gx * dt;
		Awz.x = (double)(atan2((double)RwEst.x, (double)RwEst.z) * 180 / pi);
		Awz.x += tmpf;

		tmpf = gy * dt;
		Awz.y = (double)(atan2((double)RwEst.y, (double)RwEst.z) * 180 / pi);
		Awz.y += tmpf;

		//estimate sign of RzGyro by looking in what qudrant the angle Axz is, RzGyro is pozitive if  Axz in range -90 ..90 => cos(Awz) >= 0
		signRzGyro = ( cos((double)Awz.x * pi / 180) >=0 ) ? 1 : -1;

		//reverse calculation of RwGyro from Awz angles, for formula deductions see  http://starlino.com/imu_guide.html
		RwGyro.x = (double)sin(Awz.x * pi / 180);
		RwGyro.x /= (double)sqrt( 1 + squared(cos(Awz.x * pi / 180)) * squared(tan(Awz.y * pi / 180)) );

		RwGyro.y = (double)sin(Awz.y * pi / 180);
		RwGyro.y /= (double)sqrt( 1 + squared(cos(Awz.y * pi / 180)) * squared(tan(Awz.x * pi / 180)) );

		RwGyro.z = signRzGyro * (double)sqrt(1 - squared(RwGyro.x) - squared(RwGyro.y));
	}
	
	//combine Accelerometer and gyro readings
	RwEst.x = (RwAcc.x + configwGyro * RwGyro.x) / (1+configwGyro);
	RwEst.y = (RwAcc.y + configwGyro * RwGyro.y) / (1+configwGyro);
	RwEst.z = (RwAcc.z + configwGyro * RwGyro.z) / (1+configwGyro);

	RwEst = normalize3DVector(RwEst);  
}

int main(){}	
