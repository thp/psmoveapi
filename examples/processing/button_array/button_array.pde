/*
Create a button array to store the values and press status of each button/trigger
 */

import io.thp.psmove.*;

PSMove move;

int triggerValue;
boolean isTriggerPressed, isMovePressed, isSquarePressed, isTrianglePressed, isCrossPressed, isCirclePressed, isStartPressed, isSelectPressed, isPsPressed; 

int rumbleLevel;

color sphereColor;
int r, g, b;

moveButton[] moveButtons = new moveButton[9];  // The move controller has 9 buttons                 

void setup() {
  move = new PSMove();    // We need one controller
  sphereColor = color(0); // Default sphere color (0 means ligths off)

  moveInit();             // Create the buttons
}

void draw() {
  sphereColor = color(0, 255-triggerValue, triggerValue);
  
  if(isMovePressed) {
    rumbleLevel = 255;
    sphereColor = color(random(0,255), 0, 0);
  }
  else {
    rumbleLevel = 0;
  }
  
  moveUpdate(rumbleLevel, sphereColor);           // Get the buttons value (trigger only) and presses, and update actuators/indicators
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
    move.set_leds(0, 255-trigger, trigger);
    moveButtons[0].setValue(trigger);

    int buttons = move.get_buttons();
    if ((buttons & Button.Btn_MOVE.swigValue()) != 0) {
      moveButtons[1].press();
      sphereColor = color((int)(random(255)), 0, 0);
    } 
    else {
      moveButtons[1].release();
      move.set_rumble(0);
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
  triggerValue           = moveButtons[0].value;
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

