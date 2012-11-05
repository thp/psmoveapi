
// Import the PS Move API Package
import io.thp.psmove.*;

// Tracker and controller handles
PSMoveTracker tracker;
PSMove [] controllers; // Define an array of controllers

// Variables for storing the camera image
PImage img;
byte [] pixels;

void setup() {
  size(640, 480);

  int connected = psmoveapi.count_connected();

  // This is only fun if we actually have controllers
  if (connected == 0) {
    print("WARNING: No controllers connected.");
  }

  controllers = new PSMove[connected];

  // Fill the array with controllers and light them up in white
  for (int i = 0; i<controllers.length; i++) {
    controllers[i] = new PSMove(i);
    controllers[i].set_leds(255, 255, 255);
    controllers[i].update_leds();
  }

  tracker = new PSMoveTracker(); // Create a tracker object

  tracker.set_mirror(1); // Mirror the tracker image horizontally

  // Try to calibrate each move
  for (int i = 0; i<controllers.length; i++) {
    while (tracker.enable (controllers[i]) != Status.Tracker_CALIBRATED);
  } // Every call to the controllers will have to use a loop like this one and controllers[i] to refer to each Move
}

void draw() {
  tracker.update_image();
  tracker.update();

  // Get the pixels from the tracker image and load them into a PImage
  PSMoveTrackerRGBImage image = tracker.get_image();
  if (pixels == null) {
    pixels = new byte[image.getSize()];
  }
  image.get_bytes(pixels);
  if (img == null) {
    img = createImage(image.getWidth(), image.getHeight(), RGB);
  }
  img.loadPixels();
  for (int i=0; i<img.pixels.length; i++) {
    // We need to AND the values with 0xFF to convert them to unsigned values
    img.pixels[i] = color(pixels[i*3] & 0xFF, pixels[i*3+1] & 0xFF, pixels[i*3+2] & 0xFF);
  }

  img.updatePixels();
  image(img, 0, 0); // Display the tracker image in the sketch window

  for (int i = 0; i<controllers.length; i++) {

    float x[] = new float[1];
    float y[] = new float[1];
    float r[] = new float[1];

    tracker.get_position(controllers[i], x, y, r);

    println("controllers["+i+"]: x= "+ x[0] +" y= "+ y[0] + " r= " + r[0]);
  }
}

