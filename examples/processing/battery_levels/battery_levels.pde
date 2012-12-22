
// Import the PS Move API Package
import io.thp.psmove.*;
import java.util.Arrays;

// This is the list where we will store the connected 
// controllers and their id (MAC address) as a Key.
HashMap<String, PSMove> controllers;

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

void setup() {
  
  println("Looking for controllers...");
  println(" ");

  int total_connected = psmoveapi.count_connected();
  int unique_connected = 0; // Number of actual controllers connected (without duplicates)

  controllers = new HashMap<String, PSMove>(); // Create the list of controllers

  // This is only fun if we actually have controllers
  if (total_connected == 0) {
    print("WARNING: No controllers connected.");
  }

  // Filter via connection type to avoid duplicates
  // (a controller connected via USB shows twice)
  for (int i = 0; i<total_connected; i++) {

    PSMove move = new PSMove(i);

    //int connection_type = move.connection_type_get(); // How is that controller connected to the computer
    //String connection_type_name = get_connection_name(connection_type);
    String connection_type_name = "unknown"; // fix connection_type_get() problem
    
    String address = move.get_serial(); // String or Char ?

    if (!controllers.containsKey(address)) { // Check for duplicates
      try { 
        controllers.put(address, move); // Add the id (MAC address) and controller to the list
      }
      catch (Exception ex) {
        println("Error trying to register Controller #"+i+" with address "+address);
        ex.printStackTrace();
      }
      println("Controller #"+i+" – MAC address: "+address+ " – Connection type: "+connection_type_name);
      unique_connected++; // We just added one actual controller
    }
  }
  // The size of the list matches the number of actual controllers connected
  batteryLevels = new int[unique_connected];
  
  // Separator
  println(" ");
  println("======================================================");
}

void draw() {

  int i=0;
  for (String id: controllers.keySet() ) { // For each entry
    PSMove move = controllers.get(id);     // Give me the controller with that MAC address
    batteryLevels[i] = move.get_battery();
    println("Controller #"+i+" ("+id+"): Battery Level ("+batteryLevels[i]+")");
    i++;
  }
  // Separator
  println("======================================================");
}

// Translate the connection type into a readable form
String get_connection_name(int connection_type) {
    switch(connection_type) {
      case Conn_Bluetooth:  return "Bluetooth";
      case Conn_USB :       return "USB";
      case Conn_Unknown :   return "Connection error";
      default:              return "Error in get_connection_name()";
    }
}

