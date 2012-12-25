/* 
 * Filter connected controllers by connection type (USB or Bluetooth)
 * and show their battery level, MAC adress and connection type
 *
 * Press 'R' to update the readings
 *
 * Known issue: get_battery() always returns 0
 *
 */

// Import the PS Move API Package
import io.thp.psmove.*;
import java.util.Arrays;

// This is the list where we will store the connected 
// controllers and their id (MAC address) as a Key.
HashMap<String, PSMove> controllers;

// The same controller connected via USB and Bluetooth 
// shows twice. If enabled, USB controllers will be replaced 
// with their Bluetooth counterpart when found. Otherwise,
// it is "first in first served".
boolean priority_bluetooth = true;

int total_connected, unique_connected;

int [] batteryLevels;

// Battery enum values
final int Batt_MIN           = 0;
final int Batt_20Percent     = 1;
final int Batt_40Percent     = 2;
final int Batt_60Percent     = 3;
final int Batt_80Percent     = 4;
final int Batt_MAX           = 5;
final int Batt_CHARGING      = 6;
final int Batt_CHARGING_DONE = 7;

// Connection type enum values. connection_type() returns...
final int Conn_Bluetooth = 0; // if the controller is connected via Bluetooth
final int Conn_USB       = 1; // if the controller is connected via USB
final int Conn_Unknown   = 2; // on error


//--- SETUP ----------------------------------------------------------

void setup() {
  
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
  
    String id = move.get_serial();
    String connection = get_connection_name(move.getConnection_type());

    if (!controllers.containsKey(id)) { // Check for duplicates
      try { 
        controllers.put(id, move);        // Add the id (MAC address) and controller to the list
        println("Found "+id+" via "+connection);
      }
      catch (Exception ex) {
        println("Error trying to register Controller #"+i+" with address "+id);
        ex.printStackTrace();
      }
      unique_connected++; // We just added one unique controller
    }
    else {
      if(connection == "Bluetooth" && priority_bluetooth) {
        PSMove duplicate_move = controllers.get(id);
        String duplicate_connection = get_connection_name(duplicate_move.getConnection_type()); // 
        
        controllers.put(id, move);     // Overwrite the controller at this id
        println("Found "+id+" via "+connection+" (overwrote "+duplicate_connection+")");
      }
      else {
        println("Found "+id+" via "+connection+" (duplicate ignored)");
      }
    }
  }
  // The size of the list matches the number of actual controllers connected
  batteryLevels = new int[unique_connected];

  println(" ");  
  print_status(controllers); // Show 

} // END of SETUP



//--- DRAW ----------------------------------------------------------

void draw() {
  update_battery_levels();
} // END of DRAW

//---------------------------------------------------------------------

void keyPressed() {
  if(key == 'R' || key == 'r') {
   println("r");
   print_status(controllers);
  } 
}

// Get the current battery level of each controller and update the list accordingly
void update_battery_levels() {
  int i=0;
   for (String id: controllers.keySet()) {
     PSMove move = controllers.get(id);     // Give me the controller with that MAC address
     batteryLevels[i] = move.get_battery(); // Save the battery level of the current controller
     i++;
   }
}

// Print the current battery level, mac adress and connection type of each controller
void print_status( HashMap<String, PSMove> controllers) {
  int i = 0;
  for (String id: controllers.keySet()) {
   PSMove move = controllers.get(id);     // Give me the controller with that MAC address
   
   int connection_type = move.getConnection_type(); // How is that controller connected to the computer?
   String connection_type_name = get_connection_name(connection_type);
   
   String battery_level_name = get_battery_level_name(batteryLevels[i]);
   
   println("Controller #"+i+" | Battery "+battery_level_name+" | MAC address: "+id+ " | Connection type: "+connection_type_name);
   i++;
  }
}

// Translate the connection type from int (enum) to a readable form
String get_connection_name(int connection_type) {
    switch(connection_type) {
      case Conn_Bluetooth:  return "Bluetooth";
      case Conn_USB :       return "USB";
      case Conn_Unknown :   return "Connection error";
      default:              return "Error in get_connection_name()";
    }
}

// Translate the battery level from int (enum) to a readable form
String get_battery_level_name(int battery_level) {
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

