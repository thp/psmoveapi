/* 
 * Filter connected controllers by connection type (USB or Bluetooth)
 * and show their battery level, MAC adress and connection type
 *
 * Press 'R' to update the readings
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

  println(" ");  
  print_status(controllers); // Show 

} // END of SETUP



//--- DRAW ----------------------------------------------------------

void draw() {
} // END of DRAW

//---------------------------------------------------------------------

void keyPressed() {
  if(key == 'R' || key == 'r') {
   println("r");
   print_status(controllers);
  } 
}

// Print the MAC adress and connection type of each controller
void print_status( HashMap<String, PSMove> controllers) {
  int i = 0;
  for (String id: controllers.keySet()) {
   PSMove move = controllers.get(id);     // Give me the controller with that MAC address
   
   int connection_type = move.getConnection_type(); // How is that controller connected to the computer?
   String connection_type_name = get_connection_name(connection_type);
   
   println("Controller #"+i+" | MAC address: "+id+ " | Connection type: "+connection_type_name);
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
