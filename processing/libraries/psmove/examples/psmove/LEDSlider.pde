HScrollbar hsR, hsG, hsB;
int r=128,g=128,b=128;

boolean ledSliderChecked = false;
int checkFill = 0;


void setupLEDSlider()
{

  addMouseWheelListener(new java.awt.event.MouseWheelListener() { 
    public void mouseWheelMoved(java.awt.event.MouseWheelEvent evt) { 
      mouseWheel(evt.getWheelRotation());
  }}); 
  
  noStroke();
  hsR = new HScrollbar(width-256-16, height-100,   272, 16,   4);
  hsG = new HScrollbar(width-256-16, height-60,    272, 16,   4);
  hsB = new HScrollbar(width-256-16, height-20,    272, 16,   4);
}


void updateLEDSlider()
{
  hsR.update();
  hsG.update();
  hsB.update();
  
  if (ledSliderChecked) {
    r = (int)hsR.spos - hsR.xpos;
    g = (int)hsG.spos - hsG.xpos;
    b = (int)hsB.spos - hsB.xpos;
  }
}

void drawLEDSlider()
{
  fill(255);
  int yOffset = sensorRowHeight * 3;
  
  text("R", width-256-16-32, height-92);
  text("G", width-256-16-32, height-52);
  text("B", width-256-16-32, height-12);

  text("Set LEDs: ", width-256-256, height-92);

  // Draw Checkbox
  stroke(255);
  fill(checkFill);
  rect(width-256-128, height-107, 16,16);


  hsR.display();
  hsG.display();
  hsB.display();
}



void mousePressed()
{
  if (overRect(width-256-128, height-107, 16,16)) {
    ledSliderChecked = !ledSliderChecked;
     checkFill = (ledSliderChecked ? 255 : 0); 
  } 
}

void mouseWheel(int delta) 
{
  float magnitude = 4;
  
  if (overRect(width-256-16, height-108, 264, 16)) 
  {
    int newR = (int)(hsR.spos - hsR.xpos + delta*magnitude);

    if (newR < 255 && newR > 0) 
      hsR.newspos = newR + hsR.xpos;
    else if (newR > 255) 
      hsR.newspos = 255 + hsR.xpos;
    else if (newR < 0) 
      hsR.newspos = 0 + hsR.xpos;
  }
  else if (overRect(width-256-16, height-68, 272, 16)) 
  {
    int newG = (int)(hsG.spos - hsG.xpos + delta*magnitude);

    if (newG < 255 && newG > 0) 
      hsG.newspos = newG + hsG.xpos;
    else if (newG > 255) 
      hsG.newspos = 255 + hsG.xpos;
    else if (newG < 0) 
      hsG.newspos = 0 + hsG.xpos;
  }
  else if (overRect(width-256-16, height-28, 272, 16)) {
    int newB = (int)(hsB.spos - hsB.xpos + delta*magnitude);

    if (newB < 255 && newB > 0) 
      hsB.newspos = newB + hsB.xpos;
    else if (newB > 255) 
      hsB.newspos = 255 + hsB.xpos;
    else if (newB < 0) 
      hsB.newspos = 0 + hsB.xpos;
  }
  
  println(delta); 
}


boolean overRect(int x, int y, int width, int height) 
{
  if (mouseX >= x && mouseX <= x+width && mouseY >= y && mouseY <= y+height) {
    return true;
  }

  return false;
}

void setLEDState()
{
  if (ledSliderChecked) { 
     themove.set_leds(r,g,b);
  } else {
     themove.set_leds(0,0,0); 
  }
  
  themove.update_leds();  
}
