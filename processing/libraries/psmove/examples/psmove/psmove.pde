import io.thp.psmove.*;
import processing.serial.*;

PSMove themove;

int WIDTH=1280, HEIGHT=720;
int sensorRowHeight = 200;
int infoPanelRowHeight = 120;

PFont fontA;
int fontheight=24;


void setup()
{
  size(WIDTH, HEIGHT);
  
  setupInertialData();
  setupLEDSlider();  

  // Setup fonts  
  fontA = loadFont("Monospaced-24.vlw");
  textFont(fontA, 24);

  // Connect to controllers
  int theCount = psmoveapi.count_connected();
  print("Found "); print(theCount); println(" contollers");
  if (theCount >= 1)
    themove = new PSMove(0);    
  else 
    println("could not connect to a move controller");
  
  smooth();
    
  drawBackgrounds();
} 

void draw()
{
  themove.poll();
  
  boolean didUpdateSensors = udpateInertialData(themove);
  if (didUpdateSensors) {
    drawInertialData();
  }
  
  updateLEDSlider();
  drawLEDSlider();
  
  setLEDState();  
}


void drawBackgrounds()
{
    background(0);  
    stroke(160,160,160);
    fill(96);

    rect(0,0,WIDTH,sensorRowHeight);
    rect(0,sensorRowHeight, WIDTH,sensorRowHeight);
    rect(0,sensorRowHeight*2, WIDTH,sensorRowHeight);
}
