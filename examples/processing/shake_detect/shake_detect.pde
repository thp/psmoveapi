/*
 * A simple but effective "shake" motion detection
 * Works best on left to right shaking motion
 */

import io.thp.psmove.*;

PSMove move;

color sphereColor;
int rumbleLevel;

boolean isShaken;
int shakeCount;

int threshold = 9; // The higher the number, the harder it is to shake
int falloff = 5;    // Max nb of frames it takes for the shake state to wear off

void setup() {
  size(200,200);
  
  move = new PSMove();    // We need one controller
  sphereColor = color(0); // Default sphere color (0 means ligths off)
}

void draw() {
  
  if(isShaken) { 
    sphereColor = color(255,168,0);
    rumbleLevel = 255;
  }
  
  else {
    sphereColor = color(0,200,200);
    rumbleLevel = 0;
  }
  
  moveUpdate( rumbleLevel, sphereColor );
}


void moveUpdate(int _rumbleLevel, color _sphereColor) {
  
  float [] ax = {0.f}, ay = {0.f}, az = {0.f};
  float [] gx = {0.f}, gy = {0.f}, gz = {0.f};
  float [] mx = {0.f}, my = {0.f}, mz = {0.f};
  
  while (move.poll () != 0) {

    move.get_accelerometer_frame(io.thp.psmove.Frame.Frame_SecondHalf, ax, ay, az);
    move.get_gyroscope_frame(io.thp.psmove.Frame.Frame_SecondHalf, gx, gy, gz);
    move.get_magnetometer_vector(mx, my, mz);
    
    detectShake(ax, az, threshold, falloff); // check if the accelerometers send extreme values
  }
  
  move.set_rumble(_rumbleLevel);

  int r = (int)red(_sphereColor);
  int g = (int)green(_sphereColor);
  int b = (int)blue(_sphereColor);
  move.set_leds(r, g, b);
  move.update_leds(); 
}


void detectShake(float [] _xAcc, float [] _zAcc, int _threshold, int _falloff) {
  
  if(abs(_xAcc[0]) > 1.2 || abs(_zAcc[0]) > 1.2) {
    shakeCount+=2;
  }
  
  if(shakeCount > _threshold) {
    isShaken = true;
    println("Stop shaking me!!");
    if(shakeCount > _threshold + _falloff) shakeCount = _threshold + _falloff;
  }
  
  else {
    isShaken = false;
  } 
  
  if(shakeCount>0) shakeCount--;
}

void keyPressed() {
  if (key == 27) { // escape key
    quit();
  }
}

void quit() {
  move.set_rumble(0);
  move.set_leds(0, 0, 0);
  move.update_leds(); // we switch of the sphere and rumble before we quit
  exit();
}

