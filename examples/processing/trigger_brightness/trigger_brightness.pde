
import io.thp.psmove.*;

PSMove move;

void setup() {
  move = new PSMove();
}

void draw() {
  while (move.poll() != 0) {
    int trigger = move.get_trigger();
    move.set_leds(0, trigger, 0);
    int buttons = move.get_buttons();
    if ((buttons & Button.Btn_MOVE.swigValue()) != 0) {
      move.set_rumble(255);
    } else {
      move.set_rumble(0);
    }
  }
  move.update_leds();
}

