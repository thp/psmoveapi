
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
        int buttons = move.get_buttons();
        if ((buttons & Button.Btn_MOVE.swigValue()) != 0) {
            println("The MOVE button is currently pressed. (Controller #"+i+")");
       }
   }
}
