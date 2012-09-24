
// Import the PS Move API Package
import io.thp.psmove.*;

// Define two controllers, "a" and "b"
PSMove a, b;

void setup() {
  // Open a connection to the controllers
  a = new PSMove(0);
  b = new PSMove(1);
  
  // Blink the LEDs white on start
  a.set_leds(255, 255, 255);
  a.update_leds();
  b.set_leds(255, 255, 255);
  b.update_leds();
}

void draw() {
  int trigger;
  
  // Read all data from a
  while (a.poll() != 0) { /* ... */ }

  // Get trigger press (0..255) from a
  trigger = a.get_trigger();
  // Set green brightness on b
  b.set_leds(0, trigger, 0);
  b.update_leds();

  // Read all data from b
  while (b.poll() != 0) { /* ... */ }
  
  // Get trigger press (0..255) from b
  trigger = b.get_trigger();
  // Set red brightness on a
  a.set_leds(trigger, 0, 0);
  a.update_leds();
}

