/* 
02/11/2022 - D Barker combined 'ArduGauge_Base_Display' with the 'Look_Up_Table_Dev'


-----Info-------
For performance, the gauge is displayed as a sweeping arc. The  sweepingarc is constructed of multiple segments, each created using  'fillTriangle' function
For non-linear sensors, such as thermistors, a lookup table is used. This also works for linear sensors. 


-----How To Setup-------
1. Define the min/max values of the guage face 
2. Define the minimum and maximum for signal range. e.g. 10 - 50 degrees.
3. Define the analogue levels which correspond to the max and min values.

-----Notes-------
For best performance, ensure the following:
1. Ensure min and max gauge imits are outside expected signal input range
2. Gauge divisions should be divisible by the gauge range. e.g. gauge 0 to 80 should have 8 divisions.
3. maxSegNo should be divisible by the arcSpan to prevent rounding errors. e.g. a 180 degree span should be split into 90 segments.



To Do:
display bitmaps - done on other dev code
funtion to centre the text. check chars in string then centre?
Decimal place for displayed value
  
*/


//----------------------------User Config----------------------------------//

char dataType[] = "Air Temp";   //The measurment which is being displayed. E.g. pressure, temperature...

float correctionFctr = 1.00;    //Multiplier for the input signal. Cf = Actual Value/Derived value. Adjust dependant on your hardware by using calibrated instruments.

//Gauge Setup
int sigInMin = 10;    //The min value of whatever parameter is incoming via the analog pin.
int sigInMax = 50;    //The max value of whatever parameter is incoming via the analog pin.
int gaugeMin = 0;   //Min value of gauge. should be smaller than min expected input
int gaugeMax = 60;   //Max value on the gauge. Shuld be greater than max expected input
int divisions = 6;  //Number of division around circumference. NOTE: this should be divisible by the gauge range. e.g gauge 0 to 80 should have 8 divisions. Number of lines = divisions + 1.

//Arc settings
const int arcStart = -180;   //Start position of the arc in degrees 
const int arcEnd = 0;   //End position of the arc in degrees
int maxSegNo = 90;    //Max number of segments divided by arcSpan gives the segment size. More segs gives finer resolution but takes longer to sweep

//----------------------------Other parameters----------------------------------//
#define CodeVersion "Dev"   //Version of the code 

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"

//Define I/O Pins
#define TFT_DC 8
#define TFT_CS 9  
Adafruit_GC9A01A tft(TFT_CS, TFT_DC);//Hardware SPI

const int analogPin = A0;

//Gauge Settings
int xcen = 120;     //Disiplay centre in pixels.
int ycen = 120;     //Display centre in pixels.

unsigned long avg = 0;
int readings = 50;  //Number of readings for averaging cycle

//Other variables
int rawVal;   //Raw analog reading for incoming signal
int inputVal;    //The averaged and scaled value of incoming signal. This will be displayed on the gauge
int oldVal;   //For storing the last displayed value.

int arcSpan = arcEnd - arcStart;    //Angle which the arc will span
int segSize = arcSpan/maxSegNo;   //Size of each segment in the arc
float leadingEdgeAngle;   //Calculated angle of the leading edge of the arc
float lastAngle = 0;    //What the angle was when the arc was last updated
int newSegCount;    //number of segs which should be displayed
int oldSegCount = 0;   //Number of segs when the arc was last updated

void setup() {
  Serial.begin(115200);
  Serial.println("ArduGauge Development");

  //initialize the display
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);  //clear screen on startup
  
  startupScreen();    //Display startup screen
  
  drawFace();     //Draw the static gauge features on the screen
}

void loop(void) 
{
  //Take multiple analog readings to get average
  for(int i = 0; i < readings; i++)
  {
    //rawVal = analogRead(analogPin); //Read analog pin
    rawVal = getTemp(); //Derive temp from lookuptable
    avg = avg+rawVal;   //Add it to running total
  }

  avg = avg/readings;  //divide by number of readings to get actual average

  //Serial.println(avg);
  //inputVal = map(avg, 0, 1023, sigInMin, sigInMax);   //Map analog value to specified range for expected signal input. Use this if NOT using lookuptable

  inputVal = avg*correctionFctr;    //Correction factor will adjust the averaged value
  
  //Check if the input signal is outside gauge range. Could occur when gauge range doesnt cover entire lookup table range.
  if (inputVal > gaugeMax || inputVal < gaugeMin){
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
  leadingEdgeAngle = map(inputVal, gaugeMin, gaugeMax, arcStart, arcEnd);  //Get the angle of arc leading edge
  //leadingEdgeMarker(oldAngle, leadingEdgeAngle);    //Function to dispay marker at leading edge of arc (helps for debugging)

  if (newSegCount > oldSegCount)  //If new number of segs is greater than what is displayed then...
  {
    drawArc(oldSegCount, 120, 120, 20, GC9A01A_BLUE);   //Print another segment to increase arc
    oldSegCount++;  //increment by one, then itll loop through incrementing until target is reached
    //Serial.println("Increased arc");
  }
  else if (newSegCount < oldSegCount)  //If new number of segs is less than what is displayed then...
  {
    drawArc(oldSegCount - 1, 120, 120, 20, GC9A01A_BLACK);    //Delete the last segment to decrease arc
    oldSegCount--;  //decrement by one, then itll loop through decrementing until target is reached
    //Serial.println("Reduced arc");
  }
  else    //If the new number of arc segs is the same as what is displayed then...
  {
    //Serial.println("No change to arc");
  }

  //Check if displayed value needs updating
  if(inputVal != oldVal)
  {
    printVal(inputVal, oldVal);    //Erase old value and print new 
    oldVal = inputVal;   //Update 'old' value
    //Serial.println("value updated");
  }

  delay(100); //wait before re-checking
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

//Print value in centre of display. 'a' is new value. 'b' is old value to delete first
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
  int rad1 = 100;  //Radius which lines are drawn on
  int rad2 = 85;  //Radius which numbers are drawn on
  
  //Loop through and draw lines around circumference
  for(int i = 0; i < (divisions + 1); i++)
  {
    //Calculate start and end coords of line
    float sx = cos((interval*i + arcStart)*  3.141 / 180);
    float sy = sin((interval*i + arcStart)*  3.141 / 180);
    int x0 = sx * (rad1 - lineLen) + xcen;
    int y0 = sy * (rad1 - lineLen) + ycen;
    int x1 = sx * rad1 + xcen;
    int y1 = sy * rad1 + ycen;

    //Draw the line
    tft.drawLine(x0, y0, x1, y1, GC9A01A_WHITE);
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
  tft.setCursor(35, 160);
  tft.setTextSize(3);
  tft.setTextColor(GC9A01A_WHITE);
  tft.print(dataType);

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
