/*ArduGauge V1.0 Hardware Test Code
 * Code for testing the ArduGauge V1.0 PCB
 * 
 * Base test: Print info via serial monitor to confirm MCU has been programmed and fucntions correctly.
 * 
 * LCD Test:  Print to the LCD confirming primary fucntionality.
 * 
 * I/O Test: Read analog inputs and print values via serial terminal.
 * 
 * 21/01/2023
 * Added a counter to incriment displayed value to check LCD refresh 
 * 
 */


//Test stages. Set true/false to select which will run 
bool testLCD = true;  //Print text to LCD to confirm functionality
bool IO_ReadTest = true;  //read inputs to test 

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"

//Define I/O Pins
#define TFT_DC 8  //Set by the hardware
#define TFT_CS 9  //Set by the hardware  
Adafruit_GC9A01A tft(TFT_CS, TFT_DC);//Hardware SPI

//Declare pins
const int AnalogCH_0 = A0;   //Pin for analog channel 0
const int AnalogCH_1 = A1;   //Pin for analog channel 1
const int AnalogCH_2 = A2;   //Pin for analog channel 2
const int AnalogCH_3 = A3;   //Pin for analog channel 3
const int VoltageCH = A6;   //Pin connected to onboard voltage divider for reading Vin (12v)
const int Button = 2;   //Pin connected to the button input

//Other parameters
float V_supply;    //Store value for voltage supply calculated from voltage divider
int counter = 0;    //Start the counter from 0;

void setup() {
  Serial.begin(115200);

  pinMode(Button, INPUT_PULLUP);  //Use internal pullup for button pin

  //This runs for all test stages.
  //Print via serial terminal
  Serial.print("ArduGauge V1.0 Hardware Test Code");
  Serial.println("Hello World");
  delay(1000);
  
  //if 'true', initialize LCD for testing
  if(testLCD == true)
  {
    //Initialize the display
    Serial.println("Initializing LCD...");
    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(GC9A01A_BLACK);  //clear screen on startup

    //Print to the LCD
    Serial.println("Printing to LCD...");
    tft.setCursor(30, 60);
    tft.setTextSize(3);
    tft.setTextColor(GC9A01A_WHITE);
    tft.println("ArduGauge");
    tft.setCursor(80, 90);
    tft.println("V1.0");
    tft.setCursor(10, 120);
    tft.println("Hello World!");

    delay(1000);  //Wait before clearing screen
    tft.fillScreen(GC9A01A_BLACK);  //clear screen
    //Print rounded rectangle to add something to the display
    tft.fillRoundRect(50, 150, 140, 5, 2, GC9A01A_BLUE);  //x, y, w, h, rad, colour
  }

}

void loop() 
{
  //Read input values and print
  if(IO_ReadTest == true)
  {
    V_supply = ((analogRead(VoltageCH)/1023.0)*5.0); //Convert analog input to a voltage
    V_supply = V_supply * 3.0;  //Account for hardware voltage divider dividing actual voltage supply by 3.
    
    //Read analog values and print to serial monitor
    Serial.print("CH0 "); Serial.print(analogRead(AnalogCH_0)); Serial.print("\t");
    Serial.print("CH1 "); Serial.print(analogRead(AnalogCH_1)); Serial.print("\t");
    Serial.print("CH2 "); Serial.print(analogRead(AnalogCH_2)); Serial.print("\t");
    Serial.print("CH3 "); Serial.print(analogRead(AnalogCH_3)); Serial.print("\t");
    
    Serial.print("CH_V "); Serial.print(analogRead(VoltageCH)); Serial.print("\t");
    Serial.print(V_supply); Serial.print(" v"); Serial.print("\t");

    Serial.print("Btn State "); Serial.print(digitalRead(Button)); Serial.print("\t");
    
    Serial.println("   ");
    
  }

  //Print and refresh text on LCD. Count upwards.
  if(testLCD == true)
  {
    printVal(counter, counter-1);
   
    counter++; //Incriment the counter value
  }
  delay(500);
}


//Function to overwrite the value by printing the last value in black
int printVal(int newVal, int oldVal)
{
  //overwrite old text in black. dont use the clear screen 
    tft.setTextSize(6);

    //Overwrite number in black
    tft.setCursor(50, 100);
    tft.setTextColor(GC9A01A_BLACK);
    tft.print(oldVal);
    
    
    //Now print new number in white
    tft.setCursor(50, 100);
    tft.setTextColor(GC9A01A_WHITE);
    tft.print(newVal);
}
