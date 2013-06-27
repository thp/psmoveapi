
// Import the PS Move API Package
import io.thp.psmove.*;

color sphereColor = color(255,255,255);
float blinker = 1.0;

PSMove [] moves;

void setup() {
  moves = new PSMove[psmoveapi.count_connected()];
  for (int i=0; i<moves.length; i++) {
    moves[i] = new PSMove(i);
  }
}

void draw() {
  
  for (int i=0; i<moves.length; i++) {
    handle(i, moves[i]); // Take care of each move controller
  }
  
}

void handle(int i, PSMove move)
{
   while (move.poll () != 0) {}
  int buttons = move.get_buttons();
  
  if ((buttons & Button.Ext_SharpShooter.swigValue()) != 0) {
    
    if ((buttons & Button.Btn_SHARPSHOOTER_RELOAD.swigValue()) != 0) {

    }
    
    if ((buttons & Button.Btn_SHARPSHOOTER_TRIGGER.swigValue()) != 0) {
        move.set_rumble(255);
        blinker = random(0.0,1.0);
    }
    else { 
      move.set_rumble(0);
      blinker = 1.0;
    }
    
    if ((buttons & Button.Btn_WEAPON1.swigValue()) != 0) {
        sphereColor = color(int(255*blinker), 0, 0);
    }
    else if ((buttons & Button.Btn_WEAPON2.swigValue()) != 0) {
        sphereColor = color(0, int(255*blinker), 0);
    }
    else if ((buttons & Button.Btn_WEAPON3.swigValue()) != 0) {
        sphereColor = color(0, 0, int(255*blinker));
    }
  }
  
  int r = (int)red(sphereColor);
  int g = (int)green(sphereColor);
  int b = (int)blue(sphereColor);
  
  move.set_leds(r,g,b);
  move.update_leds();
}
