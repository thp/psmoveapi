
// Import the PS Move API Package
import io.thp.psmove.*;

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
  while (move.poll () != 0) {
  long [] pressed = {0};
  long [] released = {0};
  move.get_button_events(pressed, released);
  if ((pressed[0] & Button.Btn_MOVE.swigValue()) != 0) {
      println("The Move button has been pressed now. (Controller #"+i+")");
    } else if ((released[0] & Button.Btn_MOVE.swigValue()) != 0) {
      println("The Move button has been released now. (Controller #"+i+")");
    }
  }
}
