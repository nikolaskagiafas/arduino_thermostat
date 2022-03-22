/********************************************************************/
// First we include the libraries
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
/********************************************************************/
//Temperature
// Data wire is plugged into pin 4 on the Arduino 
#define ONE_WIRE_BUS 4
double temps [24] = {0};  // Keeps the 24 temperature measurements during 2 minutes. 
int index = 0;  // Index for the temps array. 
double meanTempVal = 0;  // The mean value of the 24 temperature measurements of the 2 previous minutes.
double lastTempVal = 0;  // The temperature value that was recorded in the previous 5 seconds.
bool situation = false;  // A flag variable used for checking if it is the first time in series a low or a high temperature has been detected.
int situation2 = 0;  // A flag variable used for checking if someone stops being close to the device.

#define led_red 11  // The red led is connected to the pin 11.
#define led_blue 12  // The blue led is connected to the pin 12.
#define led_white 13  // The white led is connected to the pin 13.

//Timer 1
#define Timer_initVal 3035  // The value, in which timer1 starts counting.
int counter5sec = 0;  // The variable used to count 5 seconds.
int counter2mins = 0;  // The variable used to count 2 minutes.
int counter10sec = 0;  // The variable used to count 10 seconds.
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices   
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
/********************************************************************/ 
/* LCD display */
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address to 0x27 for a 16 chars and 2 line display.
bool powerLCD = false;  // Flag to power up LCD.

/*proximity*/
#define distanceVal 15  // A constant equal to 15cm, used by the distance sensor.
  
const int trigPin = 7;  // Triggering pin of the distance sensor.
const int echoPin = 8;  // Echoing pin of the distance sensor.

long duration;  // The duration of the transmission of the sound wave by the distance sensor.
int distance;  // The distance that the sound wave covers.
bool powerLCDProx = false;
double lastMeanVal = 0;

unsigned long lastTimeVal = 0;  // We check proximity every 5 seconds and we measure them by an increasing time interval. This is the smaller value of the time interval.
unsigned long newTimeVal =0;  // This is the bigger value of the time interval.

//  The initialization-setup function.
void setup(void) 
{ 
  // Clearing the global interrupt flag. Interrupts globally disabled.
  cli();

  // Initializing the timer1.
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1<<CS12);  // Set the prescaler to 256.
  TIMSK1 |= (1<<TOIE1);  // Enable timer1 overflow interrupt. 
  TCNT1 = Timer_initVal; // Start counting from 3035, until the timer overflows.
  
  //  Start serial port. 
  Serial.begin(9600);
    
  //  Start up the library. 
  sensors.begin();  // Start the DS18B20 temperature sensor.

  //  Setting the pins, which are going to be used in input or output mode.
  pinMode(led_red, OUTPUT);
  pinMode(led_blue, OUTPUT);
  pinMode(led_white, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT); 

  //  Initializing the first temperature corresponding to the first 5 seconds of the first two-minute sampling period. 
  temps[index] = sensors.getTempCByIndex(0);
  lastTempVal = sensors.getTempCByIndex(0);

  //  Turn on the appropriate leds, if there is a low or high temperature value in the first 5 seconds of the first two-minute sampling period.
  if(lastTempVal >= 30)
  {
    digitalWrite(led_red, HIGH);
    digitalWrite(led_white, HIGH);
   }else{
    digitalWrite(led_red, LOW);
    digitalWrite(led_white, LOW);
   }
   if(lastTempVal <= 25)
   {
    digitalWrite(led_blue, HIGH);
   }else{
    digitalWrite(led_blue, LOW);
   }
    
  //  Initializing the LCD Monitor.
  lcd.init();
  lcd.noBacklight();

  //  Setting the global interrupt flag. Interrupts globally enabled.
  sei();
} 

//  This is the main function of the program, which is called for execution repeatedly.  
void loop(void) 
{  
 newTimeVal = millis();

 //  Check for proximity every 500 microseconds. 
 if (newTimeVal - lastTimeVal >= 500)
 {
  lastTimeVal = newTimeVal;
  checkProx();
  
  //  Check if two minutes have passed so that the lcd monitor shows the mean temperature.
  if (!powerLCD){

    //  Check if someone has come closer than 15cm to the device.
    if (powerLCDProx) 
    {
      LCDWhenClose();
    }else{
      tempLowHigh();     
    }
   }  
 }
}

//  Check when we have to print to the LCD that a low or a high temperature is recorded. 
void tempLowHigh(){

      //  Clear the LCD monitor only if it is the first time in series a low or a high temperature has been detected. If the temperature continues to be low or high, do not clear it. 
      if (situation == false)
      {
        lcd.clear();
      }

      //  Clear the LCD monitor at the moment when someone stops being close to the device. 
      if (situation2 == 1)
      {
        lcd.clear();
      }

      //  If a low or a high temperature is detected, print it in the down line of the LCD monitor.
      lcd.setCursor(0, 1);

      //  Check if a low or a high temperature was previously recorded.
      if(lastTempVal >= 30)
      {
        lcd.backlight();
        lcd.print("High Temp");

        //  Give these values in these flag variables, so that the LCD will not be cleared for second time in a row. Avoid the LCD flickering.
        situation = true;
        situation2 = 2;
      }else if(lastTempVal <= 25){
        lcd.backlight();
        lcd.print("Low Temp");

        //  Give these values in these flag variables, so that the LCD will not be cleared for second time in a row. Avoid the LCD flickering.
        situation = true;
        situation2 = 2;
      }else{
        lcd.clear();  //  Clears the LCD screen and positions the cursor in the upper-left corner.
        lcd.noBacklight();
        situation = false;
      }
}

//  Print what is necessary in the LCD screen, if someone has been closer than 15cm to the device.
void LCDWhenClose()
{
  lcd.backlight();  //  Activate the backlight of the LCD screen.
  lcd.setCursor(0, 0);  //  Position the cursor in the upper-left corner.

  lcd.print("LastMean:");  //  Print the last mean temperature.
  lcd.print(lastMeanVal);

  lcd.setCursor(0, 1);  //  Position the cursor in the down-left corner.
  
  lcd.print("LastTemp:");  //  Print the last temperature recorded.
  lcd.print(lastTempVal);
}

//  Check if someone is closer than 15cm to the device.  
void checkProx()
{
  digitalWrite(trigPin, LOW);  
  delayMicroseconds(2);  //  This small delay will not affect the code negatively. (even the timer1 changes its value every 16 microseconds)
  
  //  Set the trigPin on HIGH state for 10 microseconds.
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  //  Read the echoPin, return the sound wave travel time in microseconds.
  duration = pulseIn(echoPin, HIGH);
  
  //  Calculate the distance.
  distance= duration*0.034/2;

  //  Check if someone stops being close to the device. Activate this virtual situation.
  if ((distance > distanceVal) && (situation2 == 0))
  {
    situation2 = 1;
  }

  //  Check if someone is close to the device. Activate this virtual situation.
  if (distance < distanceVal)
  {
    powerLCDProx = true;
    situation2 = 0; 
  }else {
    powerLCDProx = false;
  }
}

//  Print what is necessary in the LCD screen after two minutes have passed.
void LCDafter2mins(double meanTempVal, double lastTempVal)
{
  lcd.backlight();  //  Activate the backlight of the LCD screen.
  lcd.clear();  //  Clear the LCD screen and position the cursor in the upper-left corner.
  lcd.setCursor(0, 0);

  lcd.print("Mean Temp:");  //  Print the mean temperature of the previous 2 seconds.
  lcd.print(meanTempVal);

  lcd.setCursor(0, 1);  //  Position the cursor in the down left corner.

  //  Check if the previous temperature value is high or low and if so, print it.
  if(lastTempVal >= 30)
  {
    lcd.print("High Temp");
  }else if(lastTempVal <= 25){
    lcd.print("Low Temp");
  }
}

//  The interrupt service routine of the timer1, in case of overflow.
ISR(TIMER1_OVF_vect)
{
  //  Reinitialize the counter in the value of 3036.
  TCNT1 = 3036;

  //  Increase the counter of 5 and 120 seconds by one, because one second has passed.
  counter5sec++;
  counter2mins++;

  //  If two minutes have passed, count 10 seconds.
  if(powerLCD) counter10sec++;

  //  Check if 5 seconds have passed.
  if (counter5sec == 5)
  {
    //  Reinitialize the counter of the 5 seconds.
    counter5sec = 0;

    //  Request for the current temperature being recorded by the sensor and save it.
    sensors.requestTemperatures();
    temps[index] = sensors.getTempCByIndex(0);
    lastTempVal = temps[index];
    
    Serial.println(temps[index]);

    //  Increase the temps array index.
    index++;

    //  Turn on the appropriate leds, if a low or a high temperature has been detected.
    if(lastTempVal >= 30)
    {
      digitalWrite(led_red, HIGH);
      digitalWrite(led_white, HIGH);
    }else{
      digitalWrite(led_red, LOW);
      digitalWrite(led_white, LOW);
    }
    if(lastTempVal <= 25)
    {
      digitalWrite(led_blue, HIGH);
    }else{
      digitalWrite(led_blue, LOW);
    }
  }

  //  Check if two minutes have passed.
  if (counter2mins == 120)
  {
    //  Reinitialize the counter of the two minutes, as well as the index of the temps array.
    counter2mins = 0;
    index = 0;

    //  Enable the LCD to print what is necessary after two minutes have passed.
    powerLCD = true;

    //  Find the mean temperature value of the previous 2 minutes.
    for (int i=0 ; i<24; i++)
    {
      meanTempVal = meanTempVal + temps[i]/24;
    }

    //  Reinitialize the temps array, so that it is used for the next two minutes.
    temps[24] = {0};

    //  Make the LCD print what is necessary, after two minutes have passed.
    LCDafter2mins(meanTempVal, lastTempVal);

    //  Save the last mean temperature value recorded.
    lastMeanVal = meanTempVal;

    //  Reinitialize the mean temperature value for the next 2 minutes.
    meanTempVal =0;
  }

  //  Check if the 10 critical seconds after a two-minute time period have passed.
  if(counter10sec == 10 )
  {
    counter10sec = 0;
    lcd.noBacklight();
    lcd.clear(); 
    powerLCD = false;      
  }
}
