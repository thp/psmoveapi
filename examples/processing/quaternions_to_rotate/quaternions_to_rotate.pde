// Import the Quaternion class from Toxiclib
// Get the lib at: http://toxiclibs.org/downloads/
import toxi.geom.Quaternion;

// Import the PS Move API Package
import io.thp.psmove.*;


PSMove move;

Quaternion orientation;

void setup() {
  size(200, 200, P3D);
  
  orientation = new Quaternion(); // With a little help from Toxiclib
  
  move = new PSMove(0);
  move.enable_orientation(1); // We need to tell the PSMove object to activate sensor fusion
  move.reset_orientation(); // Set the values of the quaternions to their start values [1, 0, 0, 0] 
}

void draw() {
  background(155);
  
  float [] quat0 = {0.f}, quat1 = {0.f}, quat2 = {0.f}, quat3 = {0.f};

  while (move.poll() != 0) {}
  
  move.get_orientation(quat0, quat1, quat2, quat3);
  
  // Get the individual values
  float w = quat0[0];
  float x = quat1[0];
  float y = quat2[0];
  float z = quat3[0];

  orientation.set(w,x,y,z);
  //println("quatW = "+w+" | quatX = "+x+" | quatY = "+y+" | quatZ = "+z);
  
  // Converts the quaternion into a float array consisting of: rotation angle in radians, rotation axis x,y,z
  float[] axisAngle = orientation.toAxisAngle();
  
  float angle = axisAngle[0];
  float vecX = axisAngle[1];
  float vecY = axisAngle[2];
  float vecZ = axisAngle[3];

  long [] pressed = {0};  // Button press events
  long [] released = {0}; // Button release events
  
  // Reset the values of the quaternions to [1, 0, 0, 0] when the MOVE button is pressed
  move.get_button_events(pressed, released);
  if ((pressed[0] & Button.Btn_MOVE.swigValue()) != 0) {
    move.reset_orientation();
    println("Orientation reset");
  }

  lights();
  pushMatrix();
  translate(width*.5f, height*.5f);
  rotate(angle, -vecX, vecY, -vecZ); // The rotation directions for X and Z have to be inverted
  fill(255);
  scale(height*.05);
  box(4, 2, 10);
  popMatrix();
  
}


