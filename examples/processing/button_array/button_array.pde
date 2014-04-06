/*
Create a button array to store the values and press status of each button/trigger
 */

import io.thp.psmove.*;

PSMove move;

int triggerValue;
boolean isTriggerPressed, isMovePressed, isSquarePressed, isTrianglePressed, isCrossPressed, isCirclePressed, isStartPressed, isSelectPressed, isPsPressed; 

int rumbleLevel;

color sphereColor, defaultColor;
int r, g, b;

moveButton[] moveButtons = new moveButton[9];  // The move controller has 9 buttons                 

void setup() {
  
  move = new PSMove();      // We need one controller
  defaultColor = color( 100, 100, 100 ); // Default sphere color

  moveInit();             // Create the buttons
}

void draw() {
  
  sphereColor = defaultColor;
  
  if(isTriggerPressed) {
    println("Trigger button is "+ floor(triggerValue / 255.0 * 100.0) +"% pressed ["+triggerValue+"]");
    rumbleLevel = triggerValue;
  }
  else {
    rumbleLevel = 0; // No rumble when the trigger is not pressed
  }
  
  if(isMovePressed) {
    println("Move button is pressed");
    // The rumble & color values will be passed to the API at the end of draw()
    sphereColor = color(random(0,255), 0, 0);
  }
  else {
    sphereColor = defaultColor;
  }
  
  if(isSquarePressed) {
    println("Square button is pressed");
    sphereColor =  color( 255, 20, 100 );
  }
  
  if(isTrianglePressed) {
    println("Triangle button is pressed");
    sphereColor =  color( 80, 255, 10 );
  }
  
  if(isCrossPressed) {
    println("Cross button is pressed");
    sphereColor =  color( 10, 80, 255 );
  }
  
  if(isCirclePressed) {
    println("Circle button is pressed");
    sphereColor =  color( 255, 20, 10 );
  }
  
  if(isSelectPressed) {
    println("Select button is pressed");
  }
  
  if(isStartPressed) {
    println("Start button is pressed");
  }
  
  if(isPsPressed) {
    println("PS button is pressed");
  }
  
  moveUpdate(rumbleLevel, sphereColor);  // This is where we pass the values to the controller
}


//-------------------------------------------------------------

void moveInit() {
   for (int i=0; i<moveButtons.length; i++) {
    moveButtons[i] = new moveButton();
  } 
}

void moveUpdate(int _rumbleLevel, color _sphereColor) {
   // Read buttons  
  while (move.poll () != 0) {

    int trigger = move.get_trigger();
    moveButtons[0].setValue(trigger);

    int buttons = move.get_buttons();
    if ((buttons & Button.Btn_MOVE.swigValue()) != 0) {
      moveButtons[1].press();
    } 
    else {
      moveButtons[1].release();
    }
    if ((buttons & Button.Btn_SQUARE.swigValue()) != 0) {
      moveButtons[2].press();
    } 
    else {
      moveButtons[2].release();
    }
    if ((buttons & Button.Btn_TRIANGLE.swigValue()) != 0) {
      moveButtons[3].press();
    } 
    else {
      moveButtons[3].release();
    }
    if ((buttons & Button.Btn_CROSS.swigValue()) != 0) {
      moveButtons[4].press();
    } 
    else {
      moveButtons[4].release();
    }
    if ((buttons & Button.Btn_CIRCLE.swigValue()) != 0) {
      moveButtons[5].press();
    } 
    else {
      moveButtons[5].release();
    }
    if ((buttons & Button.Btn_SELECT.swigValue()) != 0) {
      moveButtons[6].press();
    } 
    else {
      moveButtons[6].release();
    }
    if ((buttons & Button.Btn_START.swigValue()) != 0) {
      moveButtons[7].press();
    } 
    else {
      moveButtons[7].release();
    }
    if ((buttons & Button.Btn_PS.swigValue()) != 0) {
      moveButtons[8].press();
    } 
    else {
      moveButtons[8].release();
    }
  }
  
  // Store the values in conveniently named variables
  triggerValue         = moveButtons[0].value;
  isTriggerPressed     = moveButtons[0].getPressed(); // The trigger is considered pressed if value > 0
  isMovePressed        = moveButtons[1].getPressed();
  isSquarePressed      = moveButtons[2].getPressed();
  isTrianglePressed    = moveButtons[3].getPressed();
  isCrossPressed       = moveButtons[4].getPressed();
  isCirclePressed      = moveButtons[5].getPressed();
  isSelectPressed      = moveButtons[6].getPressed();
  isStartPressed       = moveButtons[7].getPressed();
  isPsPressed          = moveButtons[8].getPressed();
  
  move.set_rumble(_rumbleLevel);
  
  r = (int)red(_sphereColor);
  g = (int)green(_sphereColor);
  b = (int)blue(_sphereColor);
  move.set_leds(r, g, b);
  move.update_leds();
}

