class MoveController extends PSMove { // Controller inherits from all the properties and methods of PSMove
  
  boolean active; // Was our controller registered?
  
  // Constructor
  MoveController(int i) {
    super(i); // Let's call the constructor from the mother class (PSMove)
    active = false;
  }
  
  // Remember the registered status of the controller
  public void activate() {
    active = true;
  }
  
  // Keep track that the controller's registration was cancelled
  public void deactivate() {
    active = false;
  }
  
  public boolean isActive() {
    return active;
  }
}
