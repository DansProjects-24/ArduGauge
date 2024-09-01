/* 
02/11/2022 - D Barker. Progressed from 'Dev_1'. Combined 'ArduGauge_Base_Display' with the 'Look_Up_Table_Dev'
04/03/2023 - Now 'Dev_2'. Replaced sweeping arc with more traditional 'hand'.
06/03/2023 - Added bitmap icon to display.

-----Info-------
For non-linear sensors, such as thermistors, a lookup table is used. This also works for linear sensors. 


-----How To Setup-------
1. Define the min/max values of the guage face 
2. Define the minimum and maximum for signal range. e.g. 10 - 50 degrees.
3. Define the analogue levels which correspond to the max and min values.

-----Notes-------
For best performance, ensure the following:
1. Ensure min and max gauge imits are outside expected signal input range
2. Gauge divisions should be divisible by the gauge range. e.g. gauge 0 to 80 should have 8 divisions.


To Do:
Function to centre the text. check chars in string then centre?
Decimal place for displayed value
Negative gauge values.
Stop dial from going outside gauge limits. If value is outside gauge range then 'cap' the value to stay at limit.

*/


//----------------------------User Config----------------------------------//

char dataType[] = "*C";   //The measurment which is being displayed. E.g. pressure, temperature...

float correctionFctr = 1.00;    //Multiplier for the input signal. Cf = Actual Value/Derived value. Adjust dependant on your hardware by using calibrated instruments.

//Gauge Setup
int sigInMin = 0;    //The min value of whatever parameter is incoming via the analog pin.
int sigInMax = 60;    //The max value of whatever parameter is incoming via the analog pin.
int gaugeMin = 0;   //Min value of gauge. should be smaller than min expected input
int gaugeMax = 60;   //Max value on the gauge. Shuld be greater than max expected input
int divisions = 6;  //Number of division around circumference. NOTE: this should be divisible by the gauge range. e.g gauge 0 to 80 should have 8 divisions. Number of lines = divisions + 1.

//Arc settings
const int arcStart = -180;   //Start position of the arc in degrees 
const int arcEnd = 0;   //End position of the arc in degrees

//----------------------------Other parameters----------------------------------//
#define CodeVersion "Dev 2"   //Version of the code 

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
int readings = 20;  //Number of readings for averaging cycle

//Other variables
int rawVal;   //Raw analog reading for incoming signal
int Temp;    //The averaged and scaled value of incoming signal. This will be displayed on the gauge
int oldVal;   //For storing the last displayed value.

int arcSpan = arcEnd - arcStart;    //Angle which the arc will span
float leadingEdgeAngle;   //Calculated angle of the leading edge of the arc
float lastAngle = 0;    //What the angle was when the arc was last updated

void setup() {
  Serial.begin(115200);
  Serial.println("ArduGauge Development");

  //initialize the display
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);  //clear screen on startup
  
  startupScreen();    //Display startup screen

  //Draw static display features. Only prints a startup
  drawStaticface();

  //Draw dynamic display features including static text which is over-written and must be re-written.
  drawDynamicface();
}

void loop(void) 
{
  unsigned long loopStart = millis();

  avg = 0;  //Reset the stored avergae to 0 before re-taking the readings
  //Take multiple analog readings to get average
  for(int i = 0; i < readings; i++)
  {
    rawVal = analogRead(analogPin); //Read analog pin
    //rawVal = getTemp(); //Derive temp from lookuptable
    avg = avg+rawVal;   //Add it to running total
  }
  
  avg = avg/readings;  //divide by number of readings to get actual average
  
  //Pass the analogue value through the lookup table function to get temperature
  Temp = getTemp(avg);
  //Serial.println(Temp);
  Temp = Temp*correctionFctr;    //Correction factor will adjust the averaged value
  
  //Check if the input signal is outside gauge range. Could occur when gauge range doesnt cover entire lookup table range.
  if (Temp > gaugeMax || Temp < gaugeMin){
    OoRWarning(true);   //Display the out of range warning
  }
  else{  //clear the warning 
    OoRWarning(false);   //Clear the out of range warning  
  }
  
  //Find angle of leading edge of arc
  int oldAngle = leadingEdgeAngle;  //store old angle so we can clear it later
  leadingEdgeAngle = map(Temp, gaugeMin, gaugeMax, arcStart, arcEnd);  //Get the angle of arc leading edge

  //Only update dial if the angle has changed
  if(leadingEdgeAngle != oldAngle)
  {
   //Serial.print("Updating dial to "); Serial.println(Temp);
   updateDial(oldAngle, leadingEdgeAngle); //Function to update dial
   drawDynamicface();
  }

  //Check if displayed value needs updating
  if(Temp != oldVal)
  {
    printVal(Temp, oldVal);    //Erase old value and print new 
    oldVal = Temp;   //Update stored 'old' value
    //Serial.println("value updated");
  }
  //delay(100); //wait before re-checking

  //Serial.println(millis() - loopStart);
}

//---------------------------------------End of Main Loop------------------------------------

//Function to print dial at new angle
int updateDial(int oldAngle, int newAngle)
{
  int rad = 90;  //radius of arc sweep by dial hand
  int wth = 10; //width of triangle making the dial hand
  
  //Calculate coords of old triangle
  int x0 = cos((oldAngle - 90)*  3.141 / 180) * (wth/2);
  int y0 = sin((oldAngle - 90)*  3.141 / 180) * (wth/2);
  int x1 = -x0;
  int y1 = -y0;
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
  x1 = -x0;
  y1 = -y0;
  x2 = cos((newAngle)*  3.141 / 180) * (rad);
  y2 = sin((newAngle)*  3.141 / 180) * (rad);

  //Draw triangle at points offset from gage center.
  tft.fillTriangle(x0+ xcen, y0+ ycen, x1+ xcen, y1+ ycen, x2+ xcen, y2+ ycen, GC9A01A_WHITE);
  
  //Serial.println(newAngle);
}


//Print value in centre of display. 'a' is new value. 'b' is old value to delete first
int printVal(int a, int b)
{
  tft.setTextSize(3);
  tft.setCursor(35, 150);
  //Print old text in black to cover it over
  tft.setTextColor(GC9A01A_BLACK);
  //if (b < 10) tft.print("0"); //Clear the leading zero for single digits
  tft.print(b); tft.print(char(0xF7)); tft.print("C");

  //Print the new text in white
  tft.setCursor(35, 150);
  tft.setTextColor(GC9A01A_WHITE);
  //if (a < 10) tft.print("0"); //Print leading zero for single digits
  tft.print(a); tft.print(char(0xF7)); tft.print("C");
}

//Fucntion draws static features of the gauge face. Should be called only on startup.
//1. Numbers for each division around the face (divisions drawn as dynamic)
int drawStaticface()
{
  float interval = (float)arcSpan/divisions;   //Calculate angle interval between each line. Use floats to prevent rounding error.
  float increments = (gaugeMax - gaugeMin)/float(divisions);  //calculate the increment value for displayed digits  
  int lineLen = 8;   //Line lenght (in pixels)
  int rad1 = 90;  //Radius which lines are drawn on
  int rad2 = 114;  //Radius which numbers are drawn on

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

  //Function to display custom bitmap at given coordinates
  printSymbol(130, 120);  
    
  }

  //Print static text
  //tft.setCursor(35, 140);
  //tft.setTextSize(3);
  //tft.setTextColor(GC9A01A_WHITE);
  //tft.print(dataType);
 
}

//Fucntion draws dynamic features of the gauge face. Includes static text which is overwirtten by dial and must be re-written
//1. Lines around circumference of the dial face
int drawDynamicface()
{
  float interval = (float)arcSpan/divisions;   //Calculate angle interval between each line. Use floats to prevent rounding error.
  float increments = (gaugeMax - gaugeMin)/float(divisions);  //calculate the increment value for displayed digits  
  int lineLen = 8;   //Line lenght (in pixels)
  int rad1 = 90;  //Radius which lines are drawn on
  int rad2 = 114;  //Radius which numbers are drawn on
  
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
  tft.setCursor(65, 180+16);
  tft.print("range!");
}
