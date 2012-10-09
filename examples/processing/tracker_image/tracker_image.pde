
/**
 * Processing example for getting the camera image
 * Thomas Perl <m@thp.io>; 2012-10-07
 **/
import io.thp.psmove.*;

PSMove move;
PSMoveTracker tracker;
byte [] pixels;
PImage img;

void setup() {
    size(640, 480);
    move = new PSMove();
    move.set_leds(255, 255, 255);
    move.update_leds();
    tracker = new PSMoveTracker();
    while (tracker.enable(move) != Status.Tracker_CALIBRATED);
}

void draw() {
    tracker.update_image();
    tracker.update();

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
    image(img, 0, 0);
}

