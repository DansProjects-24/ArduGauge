/* 
30/07/2022 - D Barker adapted the adafruit_GC9A01A example code for use with the 1.28inch round LCD
17/02/2023 - Added alternative face design. Utilising triangle to make a dial 'hand'.
https://www.waveshare.com/wiki/1.28inch_LCD_Module#Demo_Codes

I use this code to develop the example code into a 'gauge' style readout.
The purpose is to develop an arduino based gauge, 'ArduGauge', for use in a car

-----Info-------
For performance, the gauge is displayed as a sweeping arc. The  sweepingarc is constructed of multiple segments, each created using  'fillTriangle' function

-----How To Setup-------
1. Define the min/max values of the guage face 
2. Define the minimum and maximum for signal range. e.g. 10 - 50 degrees.
3. Define the analogue levels which correspond to the max and min values.

-----Notes-------
For best performance, ensure the following:
1. Ensure min and max gauge imits are outside expected signal input range
2. Gauge divisions should be divisible by the gauge range. e.g. gauge 0 to 80 should have 8 divisions.
3. maxSegNo should be divisible by the arcSpan to prevent rounding errors. e.g. a 180 degree span should be split into 90 segments.

display bitmaps

display a boot screen. display 'ArduGauge' and code build version
  
*/
#define CodeVersion "Dev"   //Version of the code 

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"

#define TFT_DC 8
#define TFT_CS 9

//Hardware SPI
Adafruit_GC9A01A tft(TFT_CS, TFT_DC);

//Declare other pins
const int analogPin = A0;

//User Cofnig
char dataType[] = {'T', 'e', 's', 't'};   //The measurment which is being displayed. E.g. pressure, temperature...
int xcen = 120;     //Disiplay centre in pixels.
int ycen = 120;     //Display centre in pixels.

//Gauge Settings
int sigInMin = 10;    //The min value of whatever parameter is incoming via the analog pin.
int sigInMax = 50;    //The max value of whatever parameter is incoming via the analog pin.
int gaugeMin = 0;   //Min value of gauge. should be smaller than min expected input
int gaugeMax = 40;   //Max value on the gauge. Shuld be greater than max expected input
int divisions = 8;  //Number of division around circumference. NOTE: this should be divisible by the gauge range. e.g gauge 0 to 80 should have 8 divisions. Number of lines = divisions + 1.

//Arc settings
const int arcStart = -180;   //start of the arc in degrees 
const int arcEnd = 0;   //end of the arc in degrees
int maxSegNo = 90;    //Max number of segments divided by arcSpan gives the segment size. More segs gives finer resolution but takes longer to sweep

unsigned long avg = 0;
int readings = 100;  //Number of readings for averaging cycle

//Other variables
int rawVal;   //Raw analog reading for incoming signal
int inputVal;    //The averaged value for incoming signal
int mappedInputSig;   //Input signal mapped to required range for whatever is being measured
int oldVal;

int arcSpan = arcEnd - arcStart;    //Angle which the arc will span
int segSize = arcSpan/maxSegNo;   //Size of each segment in the arc
float leadingEdgeAngle;   //Calculated angle of the leading edge of the arc
float lastAngle = 0;    //What the angle was when the arc was last updated
int newSegCount;    //number of segs which should be displayed
int oldSegCount = 0;   //Number of segs when the arc was last updated

void setup() {
  Serial.begin(115200);
  Serial.println("GC9A01A Test!");

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);  //clear screen on startup
  
  startupScreen();    //Function displays startup info
  
  drawFace();     //Fucntion to draw static face features on the screen
}

void loop(void) 
{
  //Take multiple analog readings to get average
  for(int i = 0; i < readings; i++)
  {
    rawVal = analogRead(analogPin); //Read analog pin
    avg = avg+rawVal;   //Add it to running total
  }
  
  avg = avg/readings;  //divide by number of readings to get actual average
  
  mappedInputSig = map(avg, 0, 1023, sigInMin, sigInMax);   //Map analog value to specified range for expected signal input

  //Check if the input signal is outside gauge range
  if (mappedInputSig > gaugeMax || mappedInputSig < gaugeMin){
    OoRWarning(true);   //Display the out of range warning
  }
  else{  //clear the warning 
    OoRWarning(false);   //Clear the out of range warning  
  }

  //calculate the number of arc segments needed to fill the arc up to the required angle
  newSegCount = (leadingEdgeAngle - arcStart)/segSize;   //Use angle of filled arc to calculate number of segemnts needed
  newSegCount = newSegCount + 1;  //add one to offset position. Even when gauge reads '0' there is always 1 seg shown
  
  //Find angle of leading edge of arc
  int oldAngle = leadingEdgeAngle;  //store old angle so we can clear it later
  leadingEdgeAngle = map(mappedInputSig, gaugeMin, gaugeMax, arcStart, arcEnd);  //Get the angle of arc leading edge
  //leadingEdgeMarker(oldAngle, leadingEdgeAngle);    //Function to dispay marker at leading edge of arc (helps for debugging)

  //Only update dial if the angle has changed
  if(leadingEdgeAngle != oldAngle)
  {
     unsigned long Tstart = micros();
 
   updateDial(oldAngle, leadingEdgeAngle); //Function to update dial
   Serial.print(micros() - Tstart); Serial.print("   ");
   drawFace();
   Serial.println(micros() - Tstart);
  }
  
//Uncomment below to do a 'sweeping arc'  
//  if (newSegCount > oldSegCount)  //If new number of segs is greater than what is displayed then...
//  {
//    drawArc(oldSegCount, 120, 120, 20, GC9A01A_BLUE);   //Print another segment to increase arc
//    oldSegCount++;  //increment by one, then itll loop through incrementing until target is reached
//    //Serial.println("Increased arc");
//  }
//  else if (newSegCount < oldSegCount)  //If new number of segs is less than what is displayed then...
//  {
//    drawArc(oldSegCount - 1, 120, 120, 20, GC9A01A_BLACK);    //Delete the last segment to decrease arc
//    oldSegCount--;  //decrement by one, then itll loop through decrementing until target is reached
//    //Serial.println("Reduced arc");
//  }
//  else    //If the new number of arc segs is the same as what is displayed then...
//  {
//    //Serial.println("No change to arc");
//  }

  //Check if displayed value needs updating
//  if(mappedInputSig != oldVal)
//  {
//    printVal(mappedInputSig, oldVal);    //Erase old value and print new 
//    oldVal = mappedInputSig;   //Update 'old' value
//    //Serial.println("value updated");
//  }

  //delay(100); //wait before re-checking
}


//Function to print dial at new angle
int updateDial(int oldAngle, int newAngle)
{
  int rad = 90;  //radius of arc sweep by dial hand
  int wth = 10; //width of triangle making the dial hand
  
  //Calculate coords of old triangle
//  float sx = cos((oldAngle)*  3.141 / 180);
//  float sy = sin((oldAngle)*  3.141 / 180);
  int x0 = cos((oldAngle - 90)*  3.141 / 180) * (wth/2);
  int y0 = sin((oldAngle - 90)*  3.141 / 180) * (wth/2);
  int x1 = -x0; //cos((90 + oldAngle)*  3.141 / 180) * (wth/2);
  int y1 = -y0; //sin((90 - oldAngle)*  3.141 / 180) * (wth/2);
  int x2 = cos((oldAngle)*  3.141 / 180) * (rad);
  int y2 = sin((oldAngle)*  3.141 / 180) * (rad);

//Serial.print(x0); Serial.print("   ");Serial.print(y0); Serial.print("   ");Serial.print(x1); Serial.print("   ");Serial.println(y1);

  //Overwrite the old dial in black
  tft.fillTriangle(x0 + xcen, y0 + ycen, x1 + xcen, y1 + ycen, x2 + xcen, y2 + ycen, GC9A01A_BLACK);

  //Reprint the dial center dot
  tft.fillCircle(xcen, ycen, 10, GC9A01A_WHITE);

  //calculate new triangle coords for new position
  x0 = cos((newAngle - 90)*  3.141 / 180) * (wth/2);
  y0 = sin((newAngle - 90)*  3.141 / 180) * (wth/2);
  x1 = -x0; //cos((90 + newAngle)*  3.141 / 180) * (wth/2);
  y1 = -y0; //sin((90 - newAngle)*  3.141 / 180) * (wth/2);
  x2 = cos((newAngle)*  3.141 / 180) * (rad);
  y2 = sin((newAngle)*  3.141 / 180) * (rad);

  tft.fillTriangle(x0+ xcen, y0+ ycen, x1+ xcen, y1+ ycen, x2+ xcen, y2+ ycen, GC9A01A_WHITE);
  
  //Serial.println(newAngle);
}


// #########################################################################
// Draw a circular or elliptical arc with a defined thickness
// #########################################################################
// 
// x,y == coords of centre of arc
// start_angle = 0 - 359
// seg_count = number segments to draw
// rx = x axis radius
// ry = y axis radius
// w  = width of arc in pixels
// colour = 16 bit colour value
//Arduino forum help with this arc. https://forum.arduino.cc/t/adafruit_gfx-fillarc/397741/6
int drawArc(int start_pos, int rx, int ry, int w, unsigned int colour)
{
    //Calculate pair of coordinates for segment leading edge
    float sx = cos((start_pos*segSize + arcStart)*  3.141 / 180);
    float sy = sin((start_pos*segSize + arcStart)*  3.141 / 180);
    int x0 = sx * (rx - w) + xcen;
    int y0 = sy * (ry - w) + ycen;
    int x1 = sx * rx + xcen;
    int y1 = sy * ry + ycen;

    // Calculate pair of coordinates for segment trailing edge
    float sx2 = cos((start_pos*segSize-segSize + arcStart)*  3.141 / 180);
    float sy2 = sin((start_pos*segSize-segSize + arcStart)*  3.141 / 180);
    int x2 = sx2 * (rx - w) + xcen;
    int y2 = sy2 * (ry - w) + ycen;
    int x3 = sx2 * rx + xcen;
    int y3 = sy2 * ry + ycen;

    //Draw two triangles which make up the segment
    tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
    tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
}

//Print value in centre of display.
int printVal(int a, int b)
{
  tft.setCursor(80, 100);
  tft.setTextSize(6);
  
  //Print old text in black to cover it over
  tft.setTextColor(GC9A01A_BLACK);
  if (b < 10) tft.print("0"); //Clear the leading zero for single digits
  tft.print(b);

  //Print the new text in white
  tft.setCursor(80, 100);
  tft.setTextColor(GC9A01A_WHITE);
  if (a < 10) tft.print("0"); //Print leading zero for single digits
  tft.print(a);
}

int leadingEdgeMarker(int oldAngle, int newAngle)
{
  int lineLen = 20;   //Line lenght (in pixels)
  int rad = 120;  //Radius which marker is drawn

  //Calculate start and end coords of old line
  float sx = cos((oldAngle)*  3.141 / 180);
  float sy = sin((oldAngle)*  3.141 / 180);
  int x0 = sx * (rad - lineLen) + xcen;
  int y0 = sy * (rad - lineLen) + ycen;
  int x1 = sx * rad + xcen;
  int y1 = sy * rad + ycen;

  //Draw the line
  tft.drawLine(x0, y0, x1, y1, GC9A01A_BLACK);

  //Calculate start and end coords of new line
  sx = cos((newAngle)*  3.141 / 180);
  sy = sin((newAngle)*  3.141 / 180);
  x0 = sx * (rad - lineLen) + xcen;
  y0 = sy * (rad - lineLen) + ycen;
  x1 = sx * rad + xcen;
  y1 = sy * rad + ycen;

  //Draw the line
  tft.drawLine(x0, y0, x1, y1, GC9A01A_WHITE);
  
}

//Fucntion draws static features of the gauge face
//1. Lines around circumference of the dial face
//2. Numbers for each division around the face
//3. Parameter type as static text, e.g. 'Temperature'
int drawFace()
{
  float interval = (float)arcSpan/divisions;   //Calculate angle interval between each line. Use floats to prevent rounding error.
  float increments = (gaugeMax - gaugeMin)/float(divisions);  //calculate the increment value for displayed digits  
  int lineLen = 10;   //Line lenght (in pixels)
  int rad1 = 80;  //Radius which lines are drawn on
  int rad2 = 114;  //Radius which numbers are drawn on
  
  //Loop through and draw lines around circumference
  for(int i = 0; i < (divisions + 1); i++)
  {
    //Calculate start and end coords of line
    float sx = cos((interval*i + arcStart)*  3.141 / 180);
    float sy = sin((interval*i + arcStart)*  3.141 / 180);
    int x0 = sx * (rad1 + lineLen) + xcen;
    int y0 = sy * (rad1 + lineLen) + ycen;
    int x1 = sx * rad1 + xcen;
    int y1 = sy * rad1 + ycen;

    //Draw the line
    tft.drawLine(x0, y0, x1, y1, GC9A01A_WHITE);
   
    //Serial.print("Line angle: "); Serial.println(interval*i);
  }

  //Now print number corresponding to line divisions
  //The numbers are printed in an off-centre arc to align the cursor with the line
  
  for(int i = 0; i < (divisions + 1); i++)
  {
    int digit = gaugeMin + (i*increments);   //Calculate the digit which is to be displayed at this position
    
    //Calculate cursor positions
    float sx = cos((interval*i + arcStart)*  3.141 / 180);
    float sy = sin((interval*i + arcStart)*  3.141 / 180);
    int x = sx * (rad2 - lineLen) + xcen - 11;  //Offset arc due to cursor always at start of number
    int y = sy * (rad2 - lineLen) + ycen - 7;   //Offset arc due to cursor always at start of number
    
    tft.setCursor(x, y);
    tft.setTextSize(2);
    tft.setTextColor(GC9A01A_WHITE);
    
    //Check if a leading zero is needed
    if(digit < 10 && digit >= 0){
      tft.print("0");tft.print(digit);  //Add leading zero if +ve single digit
    }
    else if(digit < 0 && digit > -10) {
      tft.print("-0");tft.print(digit*-1);  //Add leading zero with negative if -ve single digit
    }
    else {
      tft.print(digit);  
    }
    
  }

  //Print static text
//  tft.setCursor(35, 160);
//  tft.setTextSize(3);
//  tft.setTextColor(GC9A01A_WHITE);
//  tft.print(dataType);

}

int startupScreen()
{
  tft.setTextSize(2);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setCursor(55, 80);
  tft.println("ArduGauge");
  tft.setCursor(45, 100);
  tft.print("Version: "); tft.print(CodeVersion);
  
  delay(1000);    //Display startup screen for specified time
  tft.fillScreen(GC9A01A_BLACK);  //clear screen before proceeding
}

int OoRWarning(int state){   //Display the out of range warning

 //First decide if we are displaying (white) or clearing an old warning (black)
  if (state == true){
    tft.setTextColor(GC9A01A_WHITE);
  }
  else{
    tft.setTextColor(GC9A01A_BLACK);
  }
  
  tft.setCursor(45, 180);
  tft.setTextSize(2);
  tft.println("signal out of");
  tft.setCursor(65, 180+14);
  tft.print("range!");
}
