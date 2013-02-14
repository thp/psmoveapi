
// Import the PS Move API Package
import io.thp.psmove.*;

PSMove move;

void setup() {
  move = new PSMove(0);
  move.enable_orientation(1); // We need to tell the PSMove object to activate sensor fusion
  move.reset_orientation(); // Set the values of the quaternions to their start values [1, 0, 0, 0] 
}

void draw() {
  float [] quat0 = {0.f}, quat1 = {0.f}, quat2 = {0.f}, quat3 = {0.f};

  while (move.poll() != 0) {}
  
  move.get_orientation(quat0, quat1, quat2, quat3);
  
  println("quatW = "+quat0[0]+" | quatX = "+quat1[0]+" | quatY = "+quat2[0]+" | quatZ = "+quat3[0]);
}


