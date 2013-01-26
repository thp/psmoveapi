
// Import the PS Move API Package
import io.thp.psmove.*;

int total_connected, unique_connected;

HashMap<String, PSMove> controllers;

// Battery enum values
final int Batt_MIN           = 0x00;
final int Batt_20Percent     = 0x01;
final int Batt_40Percent     = 0x02;
final int Batt_60Percent     = 0x03;
final int Batt_80Percent     = 0x04;
final int Batt_MAX           = 0x05;
final int Batt_CHARGING      = 0xEE; // 238
final int Batt_CHARGING_DONE = 0xEF; // 239

// enum values returned by PSMove.connection_type()
final int Conn_Bluetooth = 0; // if the controller is connected via Bluetooth
final int Conn_USB       = 1; // if the controller is connected via USB
final int Conn_Unknown   = 2; // on error

void setup() {
  prepare();
}

void draw() {
  update();
}

// Register controllers connected via Bluetooth and discard others
void prepare() {

  println("Looking for controllers...");
  println("");

  total_connected = psmoveapi.count_connected();
  unique_connected = 0; // Number of actual controllers connected (without duplicates)
  
  controllers = new HashMap<String, PSMove>(); // Create the list of controllers

  // This is only fun if we actually have controllers
  if (total_connected == 0) {
    print("WARNING: No controllers connected.");
  }

  // Filter via connection type to avoid duplicates
  for (int i = 0; i<total_connected; i++) {

    PSMove move = new PSMove(i);
  
    String serial = move.get_serial();
    int type = move.getConnection_type();
    String connection = connection_toString(type);

    if (connection=="Bluetooth") { // Only register bluetooth controllers
      try { 
        controllers.put(serial, move);        // Add the id (MAC address) and controller to the list
        println("Found "+serial+" via "+connection+" (registered)");
      }
      catch (Exception ex) {
        println("Error trying to register Controller #"+i+" with address "+serial);
        ex.printStackTrace();
      }
      unique_connected++; // We just added one unique controller
    }
    else {
      println("Found "+serial+" via "+connection+" (ignored)");
    }
  }
  if ( unique_connected > 0 )
    println("Finished registering "+unique_connected+" controllers.");
  else
    println("Oops... No bluetooth controller found.");
  println("");
}

void update() {
  for (String id: controllers.keySet()) {
    PSMove move = controllers.get(id);     // Give me the controller with that MAC address
    try {
      handle(move); // Take care of each move controller
    }
    catch (Exception ex) {
      println("Fatal error in handle( controllers.get(\""+id+"\") ). The sketch will now quit.");
      ex.printStackTrace();
      exit();
    }
  } 
}

void handle(PSMove move)
{
  //println("in handle()");
  String serial = move.get_serial();
  Integer battery_level = -1;
  
  while (move.poll () != 0) {
     //println("in poll()");
     battery_level = move.get_battery();
     String level_name = to_string( battery_level );
     println( "Controller ("+serial+"): "+ level_name);
     color sphere_color = get_color( battery_level );
     int r = (int)red(sphere_color);
     int g = (int)green(sphere_color);
     int b = (int)blue(sphere_color);
     move.set_leds(r,g,b);
     move.update_leds();
   }
}

color get_color ( Integer battery_level ) {
  
  int glw = (int)map( sin( frameCount*.03 ), -1, 1, 10, 150 ); // Glow
  int rnd = (int)random(0, 50); // Random intensity
  
  color sphere_color = color(0); // By default, the sphere is switched off
   switch(battery_level) {
    case Batt_MIN:            sphere_color = color( rnd, 0,     0 ); break; // Flickering red
    case Batt_20Percent :     sphere_color = color( 255, 150,   0 ); break; // orange
    case Batt_40Percent :     sphere_color = color( 120, 120,   0 ); break; // yellow
    case Batt_60Percent :     sphere_color = color( 120, 240,   3 ); break; // Light green
    case Batt_80Percent :     sphere_color = color(  50, 240,   0 ); break; // Bright green
    case Batt_MAX :           sphere_color = color(   0, 255,   0 ); break; // White
    case Batt_CHARGING :      sphere_color = color(   0, glw, glw ); break; // Glowing blue
    case Batt_CHARGING_DONE : sphere_color = color(   0, 255, 255 ); break; // Blue
    default:                  sphere_color = color(0);               break;
  }
  return sphere_color;
}

// Translate the connection type from int (enum) to a readable form
String connection_toString(int type) {
  switch(type) {
    case Conn_Bluetooth:  return "Bluetooth";
    case Conn_USB :       return "USB";
    case Conn_Unknown :   return "Connection error";
    default:              return "Error in connection_toString()";
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
