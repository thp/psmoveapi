
// Import the PS Move API Package
import io.thp.psmove.*;

PSMove [] moves;

final int WIDTH = 50;
final int HEIGHT = 10;
final int BORDER = 30;
final int SPACING = 5;
final int BIG_SPACING = 30;

void setup() {
  size(640, 480);

  moves = new PSMove[psmoveapi.count_connected()];
  for (int i=0; i<moves.length; i++) {
    moves[i] = new PSMove(i);
    moves[i].enable_orientation(1); // PSMove_Bool (0 = False, 1 = True)
    if(moves[i].has_orientation() == 1) {
      moves[i].reset_orientation(); // Sets the 
      println("Orientation enabled for PS Move #"+i);
    }
    else
      println("Orientation tracking is not available for controller #"+i);
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

int draw_readings(float [] rw, float [] rx, float [] ry, float [] rz, int x, int y)
{
  x = draw_box(rw, x, y);
  x = draw_box(rx, x, y);
  x = draw_box(ry, x, y);
  x = draw_box(rz, x, y);
  
  return x + BIG_SPACING;
}

void handle(int i, PSMove move)
{
  float [] quat0 = {0.f}, quat1 = {0.f}, quat2 = {0.f}, quat3 = {0.f};

  while (move.poll() != 0) {}
  
  move.get_orientation(quat0, quat1, quat2, quat3);
  
  println("quatW = "+quat0[0]+" | quatX = "+quat1[0]+" | quatY = "+quat2[0]+" | quatZ = "+quat3[0]);
  
  int x = BORDER;
  int y = BORDER + 20 + (HEIGHT + SPACING) * i;
  
  fill(128);
  textSize(HEIGHT);
  text("#" + (i+1) + ": ", x, y+HEIGHT);
  x += 50;
  
  x = draw_readings(quat0, quat1, quat2, quat3, x, y);
  
  long [] pressed = {0};  // Button press events
  long [] released = {0}; // Button release events
  
  // Reset the values of the quaternions to [1, 0, 0, 0] when the MOVE button is pressed
  move.get_button_events(pressed, released);
  if ((pressed[0] & Button.Btn_MOVE.swigValue()) != 0) {
    move.reset_orientation();
    println("Orientation reset for controller #"+i);
  }
  
}

void draw() {
  background(0);
  
  fill(255, 255, 0);
  textSize(10);
  text("PS Move API Processing Orientation Example (quatW, quatX, quatY, quatZ)", 10, 20);
  fill(0, 255, 255);
  text("Press the MOVE button to reset the orientation values", 10, 33);

  for (int i=0; i<moves.length; i++) {
    handle(i, moves[i]);
  }
}


