
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
  int seq_old = 0;
  int seq = move.poll();
  if ((seq_old > 0) && ((seq_old % 16) != (seq - 1))) {
  println("Controller #"+i+": frame dropped"); // dropped frame
  }
  seq_old = seq;
}

