/* 22/01/2023 - ArduGauge accelerometer display
Demonstrates how ArduGauge could be used for displaying acceleration/breaking and lateral G's.

Uses ArduGauge PCB with the 1.28" round LCD. 
MPU6050 is connected to the I2C bus for getting X and Y acceleration.

*/

const int Ox = 120, Oy = 120;   //Origin coords in pixels. The display is 240x240
const int x_max = 80, y_max = 80; //Max deviation (in pixels) that the spot will move from origin

int x_co, y_co; //X&Y coordinates where the spot will be drawn
int last_x_co, last_y_co;   //Previously printed coordinates of the spot (used fr clearing it)

unsigned long loopStart, loopTime;



//Store the raw values from the sensor
int16_t ax_raw, ay_raw, az_raw;

//Scaling factor for raw acceleration data
float a_scale = 16384.0;

//Include libraries 
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include "I2Cdev.h"
#include "MPU6050.h"

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

//Define I/O Pins for the LCD
#define TFT_DC 8
#define TFT_CS 9  
Adafruit_GC9A01A tft(TFT_CS, TFT_DC);//Hardware SPI

//Declare the sensor
MPU6050 accelgyro;    //declare module name

void setup() {
  Serial.begin(115200);

  //initialize the display
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);  //clear screen on startup

  // join I2C bus (I2Cdev library doesn't do this automatically)
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
      Wire.begin();
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
      Fastwire::setup(400, true);
  #endif

  //Initialize the MPU6050
  accelgyro.initialize(); // initialize MPU-6050 module

  //Draw static text in the setup to reduce main loop time by 1/4
  drawStatic(); //Go to function for drawing static 'face' features

}

void loop() {
  loopStart = micros();   //Store start time of loop

  //Get accel values from the sensor
  accelgyro.getAcceleration(&ax_raw, &ay_raw, &az_raw);

  //Convert to ... G's?
  float ax = ax_raw / a_scale;
  float ay = ay_raw / a_scale;

  int x_co = ax * 100;  //Mulitply to get coordinate of dot in pixels
  int y_co = ay * 100;  //Mulitply to get coordinate of dot in pixels

  //Cap the y coordinate
  if (x_co > x_max){
    x_co = x_max;
  }
  if (x_co < x_max*-1){
    x_co = x_max*-1;
  }
  
  //Cap the y coordinate
  if (y_co > y_max){
    y_co = y_max;
  }
  if (y_co < y_max*-1){
    y_co = y_max*-1;
  }
  
//  Serial.print("Accel: ");
//  Serial.print(ax); Serial.print("\t");
//  Serial.print(ay); Serial.print("\t");
//  Serial.print(x_co); Serial.print("\t");
//  Serial.println(y_co);

  drawCrossHairs();
  
  //Draw spot at the calculated xy offsets from dispay center
  drawDot(x_co, y_co, last_x_co, last_y_co);

  //Store the coordinates after printing the spot
  last_x_co = x_co;
  last_y_co = y_co;

  delay(15);

  loopTime = micros()- loopStart; //Calculate loop time
  Serial.println(loopTime);
  
}

//Draws the static features on the display.
int drawCrossHairs()
{
  //Draw horizontal line
  tft.drawLine(0, Oy, 240, Oy, GC9A01A_WHITE);
  //Draw vertical line
  tft.drawLine(Ox, 30, Ox, 225, GC9A01A_WHITE);
}
  
int drawStatic()
{  
  //char at size 1 is 5 x 7 pixels
  tft.setTextSize(2);
  tft.setTextColor(GC9A01A_WHITE); //GC9A01A_BLACK
  
  //Print at top
  tft.setCursor(95, 0);
  tft.println("Brake");

  //Print at bottom
  tft.setCursor(95, 226);
  tft.print("Accel");

  //Print at right 
  tft.setCursor(190, 105);
  tft.println("Lat+");

  //Print at left
  tft.setCursor(0, 105);
  tft.print("Lat-");
}

int drawDot(int x1, int y1, int x0, int y0)
{
  tft.fillCircle(Ox+x0, Oy+y0, 6, GC9A01A_BLACK);   //Clear the dot by printing it black
  tft.fillCircle(Ox+x1, Oy+y1, 6, GC9A01A_RED);   //Draw dot at offsets from centre
}
