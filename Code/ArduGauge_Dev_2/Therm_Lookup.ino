/*  28/10/22 - LookUp Table code
 *  13/03/23 - Anaog vaue is nw passed to the function
 *  
 *  1. Analog value is passed to this function when it is called
 *  2. Find closest value in lookup table
 *  3. Interpolate between the two
 *  4. Return the calculated temperature
 *  
 *  In this dev code, the analog input corresponds to a temperature (thermistor)
 *  The lookup table contains the input voltage and corresponding temperature.

*/

//Declare Input Pins
int potPin = A0;    //Analog pin

//Decalre Variables
float Vin;   //We can store Vin as a float after a conversion

float interpolTemp; //The interpolated temperature to return
float diff;  //Difference between actual analog val and the lookup table value
int closestIndex;   //Store index for closest value in the lookuptable

//Array representing 1st column of the lookup table (eg voltage)
float LkUp_Col_1[] = {4.97, 4.94, 4.89, 4.82, 4.7, 4.52, 4.27, 3.95, 3.56, 3.12, 2.65, 2.2, 1.79, 1.43, 1.13, 0.89, 0.7, 0.55, 0.44, 0.35};    //Column one of lookup table. In this case Temps
//Array representing 2nd column of lookup table (eg temp)
float LkUp_Col_2[] = {-40, -30, -20, -10, 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150};    //Column two of lookup table. In this case Voltage
int arraySize = sizeof(LkUp_Col_1)/sizeof(float); //Calculate number of elements in the array

unsigned long calcTime;   //time taken to complete a loop of code


//---------------------------Function for deriving temperature using the lookup table-------------------------//
float getTemp(int analogIn) {

calcTime = micros(); //Store start time of the loop
  
//First read the analog input and convert to voltage using floating points
Vin = (analogIn/1023.0)*5.0; //Convert the analog input to the voltage range 0-5v.

float closestDiff = 5.0;    //Store lookuptable value which is closest. Start at 5v, for a 0-5v system the actual anaog V can never be further than this

//Now run through the lookup table column containing voltages
//Compare Vin to each array value, noting the closest
for (int i = 0; i < arraySize; i++)
{
  //Check difference between actual value and the lookuptable
  float diff = abs(Vin - LkUp_Col_1[i]);    //'abs' retruns absolute so is positive
  //Is this the closest?
  if(diff < closestDiff){
    closestDiff = diff;   //Store the difference as it is the closest so far
    closestIndex = i;   //Also store the index for the closest value
  }
}

int boundingIndex;  //index of the upper or lower bound of the interpolation range. Calculated either +/- 1 of the closest index
//Check is Vin is bigger or smaller than the closest value in the lookup table
if (Vin > LkUp_Col_1[closestIndex]){
  boundingIndex = closestIndex - 1; //use index below (inversely proportional)
}
else{
   boundingIndex = closestIndex + 1; //use index above (inversely proportional)
}

//Go to interpolation function and return the interpolated temperature value
interpolTemp = interpolate(Vin, LkUp_Col_1[closestIndex], LkUp_Col_1[boundingIndex], LkUp_Col_2[closestIndex], LkUp_Col_2[boundingIndex]);

calcTime = micros()-calcTime;   //Subtract start time from current time to get loop time

//Print the voltage and derived temperature
//Serial.print("V In = "); Serial.print(Vin); Serial.println(" v");
//Serial.print("Temp = "); Serial.print(interpolTemp); Serial.println(" *C");

return interpolTemp;
}


//Function to interpolate between two points in the lookup table and return the interpolated value
float interpolate(float x, float x0, float x1, float y0, float y1)
{

float y = y0+(x-x0)*(y1-y0)/(x1-x0);  //Interpolation equation 

return y; //Return the calculated value
 
}

//End of programme
