
// Import the PS Move API Package
import io.thp.psmove.*;

PSMove [] moves;

void setup() {
  moves = new PSMove[psmoveapi.count_connected()];
  for (int i=0; i<moves.length; i++) {
    moves[i] = new PSMove(i);
  }
}

void draw() {
  
  for (int i=0; i<moves.length; i++) {
    handle(i, moves[i]);
  }
  
}

void handle(int i, PSMove move)
{
  float [] ax = {0.f}, ay = {0.f}, az = {0.f};
  float [] gx = {0.f}, gy = {0.f}, gz = {0.f};
  float [] mx = {0.f}, my = {0.f}, mz = {0.f};

  while (move.poll () != 0) {  } // get button presses here

  move.get_accelerometer_frame(io.thp.psmove.Frame.Frame_SecondHalf, ax, ay, az);
  move.get_gyroscope_frame(io.thp.psmove.Frame.Frame_SecondHalf, gx, gy, gz);
  move.get_magnetometer_vector(mx, my, mz);
  
  int glow = (int)map(sin(frameCount*.02), -1, 1, 20, 200);

  boolean flip = isFlipped(ay[0], my[0]);
  if (!flip) { // Pointing up-> green
    move.set_leds(0, glow, glow); 
    move.set_rumble(0);
  }
  else { // Pointing down -> flashing red + rumble
    move.set_leds((int)random(255), 0, 0);
    move.set_rumble(255);
  }
  move.update_leds();
}

boolean isFlipped(float ay, float my) {
  if (ay<0 && my>0) return true;
  return false;
}

