/// <summary>	/// Simple IMU filter. Adapted from theory and code at http://starlino.com/imu_guide.html and http://www.starlino.com/imu_kalman_arduino.html
/// Many thanks to Sergiu Baluta of Starlino Electronics for excellent explanations and examples!
/// </summary>
public class SimpleIMUFilter
{	
  public float RwEstx = 0f; 
  public float RwEsty = 0f;
  public float RwEstz = 0f;
  
  public float RwAccx = 0f;
  public float RwAccy = -1.0f;
  public float RwAccz = 0f;
  
  public float RwGyrox = 0f;
  public float RwGyroy = 0f;
  public float RwGyroz = 0f;
  
  public float Awzx = 0f;
  public float Awzy = 0f;
  
  private final float configwGyro = 10f;
  private boolean firstSample = true;
  
  public void SimpleIMUFilter()
  {
  }
  
  public void UpdateIMU(float gx, float gy, float gz, float ax, float ay, float az, float dt)
  {
    float tmpf;  
    int signRzGyro;  
    
    float[] RwAcc = normalize3DVector(ax, ay, az);
    
    RwAccx = RwAcc[0];			
    RwAccy = RwAcc[1];			
    RwAccz = RwAcc[2];			    
    
    if (firstSample)
    {
      RwEstx = RwAccx;
      RwEsty = RwAccy;
      RwEstz = RwAccz;
      firstSample = false;
      return;
    }
    
    //evaluate RwGyro vector
    if(Math.abs(RwEstz) < 0.1)
    {
      //Rz is too small and because it is used as reference for computing Axz, Ayz it's error fluctuations will amplify leading to bad results
      //in this case skip the gyro data and just use previous estimate
      RwGyrox = RwEstx;
      RwGyroy = RwEsty;
      RwGyroz = RwEstz;
    }
    else
    {
      //get angles between projection of R on ZX/ZY plane and Z axis, based on last RwEst
      tmpf = gx * (float)dt;
      Awzx = (float)(Math.atan2(RwEstx, RwEstz) * 180 / Math.PI);
      Awzx += tmpf;
      
      tmpf = gy * dt;
      Awzy = (float)(Math.atan2(RwEsty, RwEstz) * 180 / Math.PI);
      Awzy += tmpf;
      
      //estimate sign of RzGyro by looking in what qudrant the angle Axz is, RzGyro is pozitive if  Axz in range -90 ..90 => cos(Awz) >= 0
      signRzGyro = ( Math.cos(Awzx * Math.PI / 180) >=0 ) ? 1 : -1;
      
      //reverse calculation of RwGyro from Awz angles, for formula deductions see  http://starlino.com/imu_guide.html
      RwGyrox = (float)Math.sin(Awzx * Math.PI / 180);
      RwGyrox /= (float)Math.sqrt( 1 + squared(Math.cos(Awzx * Math.PI / 180)) * squared(Math.tan(Awzy * Math.PI / 180)) );
      
      RwGyroy = (float)Math.sin(Awzy * Math.PI / 180);
      RwGyroy /= (float)Math.sqrt( 1 + squared(Math.cos(Awzy * Math.PI / 180)) * squared(Math.tan(Awzx * Math.PI / 180)) );
      
      RwGyroz = signRzGyro * (float)Math.sqrt(1 - squared(RwGyrox) - squared(RwGyroy));
    }
    
    //combine Accelerometer and gyro readings
    RwEstx = (RwAccx + configwGyro * RwGyrox) / (1+configwGyro);
    RwEsty = (RwAccy + configwGyro * RwGyroy) / (1+configwGyro);
    RwEstz = (RwAccz + configwGyro * RwGyroz) / (1+configwGyro);
    
    //			RwEst = 
    float[] RwEst = normalize3DVector(RwEstx, RwEsty, RwEstz);  
    RwEstx = RwEst[0];
    RwEsty = RwEst[1];
    RwEstz = RwEst[2];
    
    println("rwacc x: " + RwAccx + " y: " + RwAccy + " z: " + RwAccz);
  }
  
  private float[] normalize3DVector(float x, float y, float z)
  {
    // Calculate pythagoras in 3D
    float R = (float)Math.sqrt(squared(x)+squared(y)+squared(z));
    
    // Apply ratio R to all elements, resulting in a value from 0-1
    return new float[] {x/R, y/R, z/R};
    }
  


  private float squared(double x)
  {
    return (float)(x*x);
  }
}

