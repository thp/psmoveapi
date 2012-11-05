
// Import the PS Move API Package
import io.thp.psmove.*;

PSMove [] moves;

final int WIDTH = 50;
final int HEIGHT = 10;
final int BORDER = 20;
final int SPACING = 5;
final int BIG_SPACING = 30;

void setup() {
  size(640, 480);

  moves = new PSMove[psmoveapi.count_connected()];
  for (int i=0; i<moves.length; i++) {
    moves[i] = new PSMove(i);
  }
}

String fmtNum(float [] x, float [] y, float [] z)
{
  return nf(x[0], 0, 2) + "," + nf(y[0], 0, 2) + "," + nf(z[0], 0, 2);
}

int draw_box(float [] reading, int x, int y)
{
  stroke(255);
  
  fill(0);
  rect(x, y, WIDTH, HEIGHT);
  
  if (abs(reading[0]) > 1.2) {
    fill(255, 0, 0);
  } else {
    fill(255);
  }
  rect(x + WIDTH/2, y, WIDTH/2*max(-1., min(1., reading[0])), HEIGHT);
  
  return x + WIDTH + SPACING;
}

int draw_readings(float [] rx, float [] ry, float [] rz, int x, int y)
{
  x = draw_box(rx, x, y);
  x = draw_box(ry, x, y);
  x = draw_box(rz, x, y);
  
  return x + BIG_SPACING;
}

void handle(int i, PSMove move)
{
  float [] ax = {0.f}, ay = {0.f}, az = {0.f};
  float [] gx = {0.f}, gy = {0.f}, gz = {0.f};
  float [] mx = {0.f}, my = {0.f}, mz = {0.f};

  while (move.poll() != 0) {}
   
  move.get_accelerometer_frame(io.thp.psmove.Frame.Frame_SecondHalf, ax, ay, az);
  move.get_gyroscope_frame(io.thp.psmove.Frame.Frame_SecondHalf, gx, gy, gz);
  move.get_magnetometer_vector(mx, my, mz);
  
  int x = BORDER;
  int y = BORDER + 20 + (HEIGHT + SPACING) * i;
  
  fill(128);
  textSize(HEIGHT);
  text("#" + (i+1) + ": ", x, y+HEIGHT);
  x += 50;
  
  x = draw_readings(ax, ay, az, x, y);
  x = draw_readings(gx, gy, gz, x, y);
  x = draw_readings(mx, my, mz, x, y);
}

void draw() {
  background(0);
  
  fill(255, 255, 0);
  textSize(10);
  text("PS Move API Processing Sensor Example (ax, ay, az, gx, gy, gz, mx, my, mz)", 10, 20);

  for (int i=0; i<moves.length; i++) {
    handle(i, moves[i]);
  }
}


