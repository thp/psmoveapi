import io.thp.psmove.*;


PSMove themove;

int x=0, oldX=0;
int oldAX_y=100, oldAY_y=100, oldAZ_y=100;
int oldGX_y=300, oldGY_y=300, oldGZ_y=300; 
int oldMX_y=500, oldMY_y=500, oldMZ_y=500;
int WIDTH=1280, HEIGHT=720;

boolean ledSliderChecked = false;
int checkFill = 0;

int sensorRowHeight = 200;
int infoPanelRowHeight = 120;

boolean toggle = false;

PFont fontA;

HScrollbar hsR, hsG, hsB;


void setup()
{
  size(WIDTH, HEIGHT);

  noStroke();
  hsR = new HScrollbar(width-256-16, height-100, 256, 16, 2);
  hsG = new HScrollbar(width-256-16, height-60, 256, 16, 2);
  hsB = new HScrollbar(width-256-16, height-20, 256, 16, 2);

  fontA = loadFont("Monospaced-24.vlw");
  textFont(fontA, 24);

  int theCount = psmoveapi.count_connected(); // A HA HA

  print("Found ");
  print(theCount);
  println(" contollers");
  
  if (theCount > 0)
    themove = new PSMove();
  else 
    println("could not connect to a move controller");
    

  background(0);
  stroke(160,160,160);

  drawBackgrounds();
}

  String buttonString = "test";

void draw()
{
  themove.poll();

  updateLEDSlider();
  
  drawInertialData();
  drawButtonData();
  drawLEDSlider();
  
  // Get the position of the top scrollbar
  // and convert to a value to display the top image 
//  float topPos = hs1.getPos()-width/2;
  
  // printSensorData();

  oldX = x;  
  x += 5;
  
  if (x >= WIDTH) 
  {
    x = 0;
    oldX = 0;
    drawBackgrounds();
  }

  fadeAll();
  //setLEDState();  
}

void setLEDState()
{
  if (toggle) {
     themove.set_leds(255,0,0);
  } else {
     themove.set_leds(0,0,255);
  }

  themove.update_leds();  
  toggle = !toggle;
}

void fadeAll()
{
  fill(0,0,0,4);
  rect(0,0,WIDTH,HEIGHT);
}

void drawBackgrounds()
{
    stroke(160,160,160);
    fill(0);
  
    rect(0,0,WIDTH,HEIGHT);
    rect(0,0,WIDTH,sensorRowHeight);
    rect(0,sensorRowHeight, WIDTH,sensorRowHeight);
    rect(0,sensorRowHeight*2, WIDTH,sensorRowHeight);
    rect(0,sensorRowHeight*3, WIDTH,sensorRowHeight);   
}


void drawInertialData()
{
  // Accelerometer 
  stroke(255,0,0); // Red
  int AX_y = (int)map(themove.getAx(), -32000f, 32000f, 0, 199);
  line(oldX, oldAX_y, x, AX_y);
  oldAX_y = AX_y;

  stroke(0,255,0); // Green
  int AY_y = (int)map(themove.getAy(), -32000f, 32000f, 0, 199);
  line(oldX, oldAY_y, x, AY_y);
  oldAY_y = AY_y;

  stroke(0,0,255); // Blue
  int AZ_y = (int)map(themove.getAz(), -32000f, 32000f, 0, 199);
  line(oldX, oldAZ_y, x, AZ_y);
  oldAZ_y = AZ_y;


  // Gyrometer
  stroke(255,0,0); // Red
  int GX_y = (int)map(themove.getGx(), -32000f, 32000f, 200, 399);
  line(oldX, oldGX_y, x, GX_y);
  oldGX_y = GX_y;

  stroke(0,255,0); // Green
  int GY_y = (int)map(themove.getGy(), -32000f, 32000f, 200, 399);
  line(oldX, oldGY_y, x, GY_y);
  oldGY_y = GY_y;

  stroke(0,0,255); // Blue
  int GZ_y = (int)map(themove.getGz(), -32000f, 32000f, 200, 399);
  line(oldX, oldGZ_y, x, GZ_y);
  oldGZ_y = GZ_y;

  
  // Magenetometer
  stroke(255,0,0); // Red
  int MX_y = (int)map(themove.getMx(), -1024f, 1024f, 400, 599);
  line(oldX, oldMX_y, x, MX_y);
  oldMX_y = MX_y;

  stroke(0,255,0); // Green
  int MY_y = (int)map(themove.getMy(), -1024f, 1024f, 400, 599);
  line(oldX, oldMY_y, x, MY_y);
  oldMY_y = MY_y;

  stroke(0,0,255); // Blue
  int MZ_y = (int)map(themove.getMz(), -1024f, 1024f, 400, 599);
  line(oldX, oldMZ_y, x, MZ_y);
  oldMZ_y = MZ_y;
}

void printSensorData()
{
  print("AccelXYZ: ");
  print(themove.getAx());
  print(" ");
  print(themove.getAy());
  print(" ");
  print(themove.getAz());
  print("    ");
  
  print("GyroXYZ: ");
  print(themove.getGx());
  print(" ");
  print(themove.getGy());
  print(" ");
  print(themove.getGz());
  print("    ");
  
  print("MagnetXYZ: ");
  print(themove.getMx());
  print(" ");
  print(themove.getMy());
  print(" ");
  print(themove.getMz());
  println();
}

int fontheight=24;

void drawButtonData()
{
 fill(255);
  int yOffset = sensorRowHeight * 3;
  buttonString = ComposeButtonString();
  
  text("Buttons: " + buttonString, 10, yOffset + 50);
  text("R", width-256-16-32, height-92);
  text("G", width-256-16-32, height-52);
  text("B", width-256-16-32, height-12);

  text("Set LEDs: ", width-256-256, height-92);

  // Draw Checkbox
  stroke(255);
  fill(checkFill);
  rect(width-256-128, height-107, 16,16);
  
}


boolean overRect(int x, int y, int width, int height) 
{
  if (mouseX >= x && mouseX <= x+width && 
      mouseY >= y && mouseY <= y+height) {
    return true;
  } else {
    return false;
  }
}


void mousePressed()
{
  if (overRect(width-256-128, height-107, 16,16)) {
    ledSliderChecked = !ledSliderChecked;
     checkFill = (ledSliderChecked ? 255 : 0); 
      println("woot");
  }
  
}

String ComposeButtonString()
{
  return "test";  
}

void updateLEDSlider()
{
  hsR.update();
  hsG.update();
  hsB.update();
  
  if (ledSliderChecked) {
    
  }
}

void drawLEDSlider()
{

  hsR.display();
  hsG.display();
  hsB.display();
}
