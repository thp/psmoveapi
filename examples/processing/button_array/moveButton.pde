class moveButton {

  int isPressed;
  int value; // For analog buttons only (triggers)
  PVector analog; // For analog sticks (navigation controller only)

  moveButton() {
    isPressed = 0;
    value = 0;
    analog = new PVector(0,0);
  }

  void press() {    
    isPressed = 1;
  }

  void release() { 
    isPressed = 0;
  }
  
  boolean getPressed() {
    if(isPressed == 1)   return true;
    return false;
  }

  void setValue(int _val) {   
    value = _val;
    if(value > 0) isPressed = 1;
    else isPressed = 0;
  }
  
  int getValue() {    
    return value;
  }
  
  void setAnalog(float _x, float _y) {
    analog.x = _x;
    analog.y = _y;
  }
  
  PVector getAnalog() {
    return analog;
  }
};
