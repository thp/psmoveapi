
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
  float [] ax = {0.f}, ay = {0.f}, az = {0.f};

  while (move.poll () != 0) {
    move.get_accelerometer_frame(io.thp.psmove.Frame.Frame_SecondHalf, ax, ay, az);
      
    float accX = ax[0];
    float accY = ay[0];
    float accZ = az[0];
    // Convert rad/s to RPM
    accX = accX * 60 / (2*PI);
    accY = accY * 60 / (2*PI);
    accZ = accZ * 60 / (2*PI);

    println("Move #"+i+" rotation X: %.2f RPM: "+accX);
    println("Move #"+i+" rotation Y: %.2f RPM: "+accY);
    println("Move #"+i+" rotation Z: %.2f RPM: "+accZ);
    println("-----------------------------------------");
  }
}

