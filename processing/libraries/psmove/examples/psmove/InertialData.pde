int x=0, oldX=0;

int oldAX_y=100, oldAY_y=100, oldAZ_y=100;
int oldGX_y=300, oldGY_y=300, oldGZ_y=300; 
int oldMX_y=500, oldMY_y=500, oldMZ_y=500;

SimpleIMUFilter imuFilter;

float RawAccelX=0, RawAccelY=0, RawAccelZ=0;
float AccelX=0, AccelY=0, AccelZ=0;
float RawGyroX=0, RawGyroY=0, RawGyroZ=0;
float GyroX=0, GyroY=0, GyroZ=0;
float FilteredAccelX=0, FilteredAccelY=0, FilteredAccelZ=0;

long lastTime;
long interval = 5;

// Constant for converting raw accelerometer values to 9.8m/s^2, i.e. rawAccel/gConst=accel (in G's)
float gConst = 4326f; 

// Constant for converting raw gyro values to radians/s, i.e. rawGyro/rConst=gyro (in rad/s)
float rConst = 7509.8f;

void setupInertialData()
{
  imuFilter = new SimpleIMUFilter();
  lastTime = System.currentTimeMillis();
}

boolean udpateInertialData(PSMove theMove)
{
  long now = System.currentTimeMillis();
  float dt = (float)(now - lastTime);
  if (dt < interval)
    return false;
    
  lastTime = now;

  // Move inertial data cursor forward
  oldX = x;  
  x += 5;
  
  if (x >= WIDTH) 
  {
    x = 0;
    oldX = 0;
    drawBackgrounds();
  }  
  
  RawAccelX = theMove.getAx();
  RawAccelY = theMove.getAy();
  RawAccelZ = theMove.getAz();

  RawGyroX = theMove.getGx();
  RawGyroY = theMove.getGy();
  RawGyroZ = theMove.getGz();

  AccelX = RawAccelX/gConst;
  AccelY = RawAccelY/gConst;
  AccelZ = RawAccelZ/gConst;
  
  GyroX = RawGyroX/rConst;
  GyroY = RawGyroY/rConst;
  GyroZ = RawGyroZ/rConst;

  imuFilter.UpdateIMU(GyroX, GyroY, GyroZ, AccelX, AccelY, AccelZ, dt);
  
  FilteredAccelX = imuFilter.RwEstx;
  FilteredAccelY = imuFilter.RwEsty;
  FilteredAccelZ = imuFilter.RwEstz;

  return true;
}


void drawInertialData()
{
  // Accelerometer 
  stroke(255,0,0); // Red
  int AX_y = (int)map(themove.getAx(), -32000f, 32000f, 0, 199);
  line(oldX, oldAX_y, x, AX_y);
  oldAX_y = AX_y;

  stroke(0,255,0); // Green
  int AY_y = (int)map(themove.getAy(), -32000f, 32000f, 0, 199);
  line(oldX, oldAY_y, x, AY_y);
  oldAY_y = AY_y;

  stroke(0,16,255); // Blue
  int AZ_y = (int)map(themove.getAz(), -32000f, 32000f, 0, 199);
  line(oldX, oldAZ_y, x, AZ_y);
  oldAZ_y = AZ_y;


  // Gyrometer
  stroke(255,0,0); // Red
  int GX_y = (int)map(themove.getGx(), -32000f, 32000f, 200, 399);
  line(oldX, oldGX_y, x, GX_y);
  oldGX_y = GX_y;

  stroke(0,255,0); // Green
  int GY_y = (int)map(themove.getGy(), -32000f, 32000f, 200, 399);
  line(oldX, oldGY_y, x, GY_y);
  oldGY_y = GY_y;

  stroke(0,16,255); // Blue
  int GZ_y = (int)map(themove.getGz(), -32000f, 32000f, 200, 399);
  line(oldX, oldGZ_y, x, GZ_y);
  oldGZ_y = GZ_y;


  // Filtered Accel
//  stroke(255,0,0); // Red
  stroke(255,255,0); // Red
  int MX_y = (int)map(FilteredAccelX, -6f, 6f, 400, 599);
  line(oldX, oldMX_y, x, MX_y);
  oldMX_y = MX_y;

//  stroke(0,255,0); // Green
  stroke(255,0,255); // Green
  int MY_y = (int)map(FilteredAccelY, -6f, 6f, 400, 599);
  line(oldX, oldMY_y, x, MY_y);
  oldMY_y = MY_y;

//  stroke(0,16,255); // Blue
  stroke(0,255,128); // Blue
  int MZ_y = (int)map(FilteredAccelZ, -6f, 6f, 400, 599);
  line(oldX, oldMZ_y, x, MZ_y);
  oldMZ_y = MZ_y;
}

void printSensorData()
{
  print("AccelXYZ: ");
  print(themove.getAx());
  print(" ");
  print(themove.getAy());
  print(" ");
  print(themove.getAz());
  print("    ");
  
  print("GyroXYZ: ");
  print(themove.getGx());
  print(" ");
  print(themove.getGy());
  print(" ");
  print(themove.getGz());
  print("    ");
  
  print("MagnetXYZ: ");
  print(themove.getMx());
  print(" ");
  print(themove.getMy());
  print(" ");
  print(themove.getMz());
  println();
}

