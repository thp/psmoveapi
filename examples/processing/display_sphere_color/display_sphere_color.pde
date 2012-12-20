/*
 * Give a realistic feedback of the sphere color
 */

import io.thp.psmove.*;

PSMove move;
color sphereColor;
int diam;

void setup() {
  size(300,300);
  colorMode(HSB); // 

  move = new PSMove();    // We need one controller
  sphereColor = color(0); // Default sphere color (0 means ligths off)
  
  diam = (int)(min(width,height)*.9); // Diameter of the circle
}



void draw() {
  background(200);
  
  // We define color values that will vary over time
  int h = (int) map( sin( frameCount*.001 ), -1, 1, 0, 255 );   // Hue
  int s = (int) map( sin( frameCount*.005 ), -1, 1, 200, 255 ); // Saturation
  int b = (int) map( sin( frameCount*.01 ), -1, 1, 0, 255 );    // Brightness
  
  sphereColor = color( h, s, b ); // Set the new color value
  
  moveUpdate(sphereColor);        // Send the new value to the controller
  
  drawColorCircle(sphereColor);   // Display the color in the sketch window
}



// rgb color (0,0,0) is black, but we want it to be white...
void drawColorCircle(color c) {
  
  stroke( 0,0,255 ); // White outline, for style
  strokeWeight(3);
  
  int alpha = (int)brightness(c);   // Transparency
  
  pushMatrix();                     // Temporary adjustment of the coordinates system
  translate( width*.5, height*.5 ); // To the center
  
  fill( 0,0,255 );                  // Paint any shape that follows white
  ellipse( 0,0,diam,diam );         // Draw a white background circle
  
  fill( c, alpha );                 // Set color & transparency
  ellipse( 0,0,diam,diam );         // Draw the color circle
  
  popMatrix();                      // Forget the adjustment of the coord system
  
}



void moveUpdate(color _sphereColor) {
  
  // Convert from HSB to RGB
  int r = (int)red(_sphereColor);
  int g = (int)green(_sphereColor);
  int b = (int)blue(_sphereColor);
  
  move.set_leds(r, g, b); // Send the info to the controller
  move.update_leds();     // Display the new color
  
}



