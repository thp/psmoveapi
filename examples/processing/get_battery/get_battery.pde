
// Import the PS Move API Package
import io.thp.psmove.*;

PSMove [] moves;

// Battery enum values
final int Batt_MIN           = 0x00;
final int Batt_20Percent     = 0x01;
final int Batt_40Percent     = 0x02;
final int Batt_60Percent     = 0x03;
final int Batt_80Percent     = 0x04;
final int Batt_MAX           = 0x05;
final int Batt_CHARGING      = 0xEE;
final int Batt_CHARGING_DONE = 0xEF;

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
     int battery_level = move.get_battery();
     String level_name = to_string( battery_level );
     println( "Controller #"+i+": "+ level_name);
   }
}

// Translate the battery level from int (enum) to a readable form
String to_string(int battery_level) {
    switch(battery_level) {
      case Batt_MIN:            return "low";
      case Batt_20Percent :     return "20%";
      case Batt_40Percent :     return "40%";
      case Batt_60Percent :     return "60%";
      case Batt_80Percent :     return "80%";
      case Batt_MAX :           return "100%";
      case Batt_CHARGING :      return "charging...";
      case Batt_CHARGING_DONE : return "fully charged";
      default:                  return "[Error in get_battery_level_name()]";
    }
}
