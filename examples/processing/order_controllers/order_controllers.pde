
// Connect several controllers via Bluetooth.
// Launch the sketch and successively press the MOVE button 
// of each controller to activate them in the desired order.

// This sketch also shows an example of extending the PSMove class

// Import the PS Move API Package
import io.thp.psmove.*;

MoveController [] controllers; // MoveController is a class extending PSMove
ArrayList<MoveController> ordered_controllers; // The list will grow for each activated controller

boolean isRunning = false; // True when at least one controller was activated

void setup() {

  frameRate(3);

  // How many controllers are we talking about exactly?
  int num_connected = psmoveapi.count_connected();

  // This is our dynamic list of artivated controllers
  ordered_controllers = new ArrayList<MoveController>();

  // Instanciate and populate the array with controllers
  controllers = new MoveController[num_connected];
  for (int i=0; i<controllers.length; i++) {
    controllers[i] = new MoveController(i);
  }
}

void draw() {
  await_activation(); // check for activation and register controllers

  if(isRunning)       // Once we have at least one active controller...
    display_chaser(); // Loop through the active controllers and lit them up in turns
}

// Wait for a button press from the inactive controllers and register them
void await_activation() {
  
  for (int i=0; i<controllers.length; i++) {
    boolean isActive;
    MoveController move = controllers[i];
    
    // We want to register only controllers not already active 
    // and for which the button has just been pressed
    if(!move.isActive() && isButtonPressed(i, move))
      register(move);
  }
  
}

boolean isButtonPressed(int i, MoveController move)
{
  while (move.poll () != 0) {
    long [] pressed = {0};
    long [] released = {0};
    move.get_button_events(pressed, released);
    if ((pressed[0] & Button.Btn_MOVE.swigValue()) != 0) {
      println("The Move button has been pressed now. (MoveController #"+i+")");
      return true;
    } 
    else if ((released[0] & Button.Btn_MOVE.swigValue()) != 0) {
      println("The Move button has been released now. (MoveController #"+i+")");
    }
  }
  return false;
}

void register(MoveController move) {
  if(!isRunning) isRunning = true;
  ordered_controllers.add(move);
  move.activate(); // Tell the controller that it has been registered
}

void display_chaser() {

  if(ordered_controllers.size() != 0) {
    int active_i = frameCount % (ordered_controllers.size());
    println("active controller : "+active_i);
    
    // All lights off and one light on
    for (int i=0; i<ordered_controllers.size(); i++) {
      MoveController move = ordered_controllers.get(i);
      if(i != active_i) move.set_leds(0, 0, 0);
      else  move.set_leds(255, 0, 0);
      move.update_leds();
    }
  }
  else {
    println("No active controller. Can't display chaser."); 
  }
  
}

void shutdown() {
   for (int i=0; i<controllers.length; i++) {
     MoveController move = controllers[i];
     move.set_leds(0,0,0); // Lights off
     move.set_rumble(0); // Lights off
   } 
}

void stop() {
  shutdown();    // We clean after ourselves (rumble and lights off)
  super.stop();  // Whatever Processing usually does at shutdown
}
