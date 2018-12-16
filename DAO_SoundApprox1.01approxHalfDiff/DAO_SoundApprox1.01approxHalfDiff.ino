#include <elapsedMillis.h>

/*OTHERING MACHINES v.3.01 alternativer Code #1:
  2.12.2018
   ein Zufallswert in dB
   incoming freq dividable --> respond ownData
   incoming freq NOT dividable --> find new value halfway between imconing and own data, respond new
   random start values: 100-3000, not forced-dividable by 20
   ampThreshold = 20
   beep duration = 600

   TODO: Farbkombination aus R und B ermöglichen

  !!---Conversion of float to int in modulo might be cause for errors!---!!


   Voltages need to be checked and translated to 8-bit
   CHECK FOR TESTS WETHER BATTERY OR USB POWERED! might have an influence on voltage/sensitivity


   for the course 'Digital Artifactual Objections' held in the summer term 2017 at HfK Bremen
   by David Unland

  based on the Code by Amanda Ghassaei (see below)*/


//generalized wave freq detection with 38.5kHz sampling rate and interrupts
//by Amanda Ghassaei
//https://www.instructables.com/id/Arduino-Frequency-Detection/
//Sept 2012

/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   INPUT = ANALOG PIN 0
   for sensitivity/realiability adjustments, alter slopeTol, timerTol, ampThreshold
*/


//--------------------------------------variables for decision whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 3;//slope tolerance- higher for steep waves
int timerTol = 15;//timer tolerance- low for complicated waves/higher resolution

//---------------------------------------variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 20;//raise if you have a very noisy signal
byte midpoint = 127;


//------------------------------------OUTPUT PINS:
const int LED_info = 5;
const int LED_R = 6;
const int LED_G = 10;
const int LED_B = 9;
const int VIBR = 3;
const int SPEAK = 8;

//------------------------------------reaction and behavior
float myTone;
elapsedMillis now = 0;
int freqTol = 50;
unsigned long respondFactor; //will be declared after randomSeed is set
unsigned long waitFactor; //will be declared after randomSeed is set
boolean listen = true;
//LED intensity
float red;
float green;
float blue;
float redAb = 0;
float blueAb = 0;
float greenAb = 0;
float abTot = 0; //total abundancy of frequencies in database
boolean ledState = LOW;
int subsideAfterReaction = 25000;
int toneDuration = 600; //length of sound emission in milliseconds

//------------------------------------database variables
//const int dbLength = 3;
const int dbLength = 1;
int db[dbLength];
int ab[dbLength];
boolean dataExists = false;

//------------------------------------clipping indicator variables
boolean clipping = 0;

//------------------------------------data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends values to store in timer[] occasionally
int timer[10];//sstorage for timing of events
int slope[10];//storage for slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//storage for period of wave
byte index = 0;//current storage index
float frequency;//storage for frequency calculations
int maxSlope = 0;//used to calculate max slope as trigger point
int newSlope;//storage for incoming slope data
boolean periodFound = false;

////------------------------------------smoothing data
const int medianLength = 9;
float median[medianLength];


//-------------------------------------------------------------------------------------------------------------
//-----------------------------------------------SETUP---------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
void setup()
{
  //-----------------------------------------------waiting time and pinout
  //Serial.begin(115200);
  randomSeed(analogRead(0));
  respondFactor = random(100, 201); //that is a factor of 0.1 to 0.2 seconds
  waitFactor = respondFactor * 600; //time to pass before timedStatement

  pinMode(13, OUTPUT); //led indicator pin
  pinMode(12, OUTPUT); //output pin
  pinMode(SPEAK, OUTPUT); //speaker tone out
  pinMode(LED_info, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(VIBR, OUTPUT); //for vibrationmotor --> negative reaction

  //-----------------------------------------------LED startup check
  now = 0;
  analogWrite(LED_R, 0);
  while (now < 150) {
    //do nothing
  }
  digitalWrite(LED_R, 255);
  now = 0;
  analogWrite(LED_G, 0);
  while (now < 150) {
    //do nothing
  }
  digitalWrite(LED_G, 255);
  now = 0;
  analogWrite(LED_B, 0);
  while (now < 150) {
    //do nothing
  }
  digitalWrite(LED_B, 255);
  now = 0;
  digitalWrite(LED_info, HIGH);
  while (now < 150) {
    //do nothing
  }
  digitalWrite(LED_info, LOW);

  //-----------------------------------------------fill and print database
  //1. fills database
  //2. lights RGB for 3s
  //Serial.print("initial dB entry = ");
  //generate single random entry dividable by 20
  myTone = int(random(100, 3001));
  /*while (int(myTone) % 20 != 0) {
    myTone++;
    }*/
  //Serial.println(db[0]);
  /*if (myTone >= 0 && myTone < 1500) {
    pinMode(LED_R, OUTPUT);
    }
    if (myTone >= 750 && myTone < 2250) {
    pinMode(LED_G, OUTPUT);
    }
    if (myTone >= 1250 && myTone < 2750) {
    pinMode(LED_B, OUTPUT);
    }*/
  lightRGB();
  now = 0;
  while (now < 3000) {
    //do nothing / light LED
  }
  unlightRGB();


  //------------------------------------------------------------------------------------------------------------------
  //-----------------------------------------------INTERRUPTS---------------------------------------------------------
  //------------------------------------------------------------------------------------------------------------------
  cli();//disable interrupts

  //set up continuous sampling of analog pin 0 at 38.5kHz

  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;

  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only

  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements

  sei();//enable interrupts
}

ISR(ADC_vect) {//when new ADC value ready

  if (listen) {

    PORTD |= B00100000; //set pin 5 high
    //PORTB &= B11101111;//set pin 12 low
    prevData = newData;//store previous value
    newData = ADCH;//get value from A0
    if (prevData < midpoint && newData >= midpoint) { //if increasing and crossing midpoint
      newSlope = newData - prevData;//calculate slope
      if (abs(newSlope - maxSlope) < slopeTol) { //if slopes are ==
        //record new data and reset time
        slope[index] = newSlope;
        timer[index] = time;
        time = 0;
        if (index == 0) { //new max slope just reset
          //PORTB |= B00010000;//set pin 12 high
          noMatch = 0;
          index++;//increment index
        }
        else if (abs(timer[0] - timer[index]) < timerTol && abs(slope[0] - newSlope) < slopeTol) { //if timer duration and slopes match
          //sum timer values
          totalTimer = 0;
          for (byte i = 0; i < index; i++) {
            totalTimer += timer[i];
          }
          period = totalTimer;//set period
          periodFound = true;
          //reset new zero index values to compare with
          timer[0] = timer[index];
          slope[0] = slope[index];
          index = 1;//set index to 1
          //PORTB |= B00010000;//set pin 12 high
          noMatch = 0;
        }
        else { //crossing midpoint but not match
          index++;//increment index
          if (index > 9) {
            reset();
          }
        }
      }
      else if (newSlope > maxSlope) { //if new slope is much larger than max slope
        maxSlope = newSlope;
        time = 0;//reset clock
        noMatch = 0;
        index = 0;//reset index
      }
      else { //slope not steep enough
        noMatch++;//increment no match counter
        if (noMatch > 9) {
          reset();
        }
      }
    }

    //    if (newData == 0 || newData == 1023) { //if clipping
    //      //PORTB |= B00100000;//set pin 13 high- turn on clipping indicator led
    //      clipping = 1;//currently clipping
    //    }

    time++;//increment timer at rate of 38.5kHz

    //printing frequency only of signal loud enough
    ampTimer++;//increment amplitude timer
    if (abs(midpoint - ADCH) > maxAmp) {
      maxAmp = abs(midpoint - ADCH);
    }
    if (ampTimer == 1000) {
      //Serial.println("ampTimer refreshed");
      ampTimer = 0;
      checkMaxAmp = maxAmp;
      maxAmp = 0;
      periodFound = false;
    }
  }
}
//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------FUNCTIONS---------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------



//-----------------------------------------------Frequency Detection
void reset() { //clear out some variables
  index = 0;//reset index
  noMatch = 0;//reset match counter
  maxSlope = 0;//reset slope
  time = 0;
  checkMaxAmp = 0;
  maxAmp = 0;
}


//void checkClipping() { //manage clipping indicator LED
//  if (clipping) { //if currently clipping
//    PORTB &= B11011111;//turn off clipping indicator led
//    clipping = 0;
//  }
//}


//-----------------------------------------------sortFloat
void sortFloat(float a[], int size) {
  for (int i = 0; i < (size - 1); i++) {
    for (int o = 0; o < (size - (i + 1)); o++) {
      if (a[o] > a[o + 1]) {
        float t = a[o];
        a[o] = a[o + 1];
        a[o + 1] = t;
      }
    }
  }
}


//-----------------------------------------------sortDatabase
/*void sortDatabase(int a[], int size) {
  for (int i = 0; i < (size - 1); i++) {
    for (int o = 0; o < (size - (i + 1)); o++) {
      if (a[o] > a[o + 1]) {
        float t = a[o];
        float b = ab[o]; //abundancy array
        a[o] = a[o + 1];
        ab[o] = ab[o + 1];
        a[o + 1] = t;
        ab[o + 1] = b;
      }
    }
  }
  //printDatabase();
  }
*/

//-----------------------------------------------do Nothing
void doNothing(unsigned long waitingTime) {
  now = 0;
  //Serial.print("do nothing: ");
  //Serial.println(waitingTime);
  while (now < waitingTime) {
    //do nothing
  }
}

//-----------------------------------------------do Nothing
void subside(unsigned long waitingTime) {
  now = 0;
  boolean blinky = false;
  while (now < waitingTime)
  {
    if (now % 2000 == 0)
    {
      if (blinky)
      {
        digitalWrite(LED_info, HIGH);
        delay(200);
        digitalWrite(LED_info, LOW);
        blinky = false;
      } else
      {
        blinky = true;
      }
    }
  }
  digitalWrite(LED_info, LOW);
}

//-----------------------------------------------timed sound emission
//1. play (random from database) and light RGB
//2. wait 4s
void timedStatement() {
  int playTimed = db[int(random(0, dbLength))];
  lightRGB();
  now = 0;
  while (now < toneDuration) {
    tone(SPEAK, playTimed, 10);
    //if (millis() % 200 == 0) {
    //Serial.print(" ..emitting most frequent tone = ");
    //Serial.print(playTimed);
    //}
  }
  //reset();
  //Serial.println();
  unlightRGB();
  doNothing(4000); //this is used to avoid self-referential loop?
}

//-----------------------------------------------Light LED:
void lightRGB()
{
  //format int = abs(deviation)*(current sink (max=255) / maxDeviation)
  //--> the greater the deviation, the higher the current sink)
  float red = (myTone - 750) * 255 / 750;
  float green = (myTone - 1500) * 255 / 750;
  float blue = (myTone - 2250) * 255 / 750;

  analogWrite(LED_R, min(255, abs(red)));
  analogWrite(LED_G, min(255, abs(green)));
  analogWrite(LED_B, min(255, abs(blue)));

}

//-----------------------------------------------Unlight LED
//sets all colors to 0
void unlightRGB() {

  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
}

//-----------------------------------------------positive Reaction
//lights RGB-LED until waiting time exceeded,
//then emits same as heard
//subsiding: no lights
void positiveReaction(int incomingFrequency)
{
  lightRGB();
  doNothing(waitFactor);
  unlightRGB();

  now = 0;
  while (now < toneDuration) {
    tone(SPEAK, incomingFrequency, 10);
  }
  //reset();

  subside(subsideAfterReaction);
}

//-----------------------------------------------Negative Reaction
//1. waiting time: light RGB
//2. calculate freq of halfway note between incomingData and myTone
//3. set new value = half of difference
//4. LEDs off, play new tone
//5. subsiding: no lights
void negativeReaction(int incomingFrequency, unsigned long wait)
{
  //vibrate:
  //  now = 0;
  //  while (now < 200) {}
  //  now = 0;
  //  while (now < 300) {
  //    digitalWrite(VIBR, HIGH);
  //  }
  //  digitalWrite(VIBR, LOW);

  //waiting time
  lightRGB();
  doNothing(wait);
  unlightRGB();

  int newValue = abs((incomingFrequency + myTone) / 2);
  myTone = newValue;
  
  //play tone:
  lightRGB();
  now = 0;
  while (now < toneDuration)
  {
    tone(SPEAK, myTone, 10);
  }
  //reset();
  //myTone = incomingFrequency;
  unlightRGB();

  //subsiding time
  subside(subsideAfterReaction);
}

//------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------LOOP---------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
//1. constant RGB off
//2. timed emission, if millis dividable by waitFactor
//3. check for amplitude- and period-trigger
//(3.1. react accordingly)
//(3.2. clear variables, wait)
void loop()
{
  //checkClipping();

  if (listen) {
    PORTD |= B00100000; //set pin 5 high
  } else
  {
    PORTD &= B11011111; //set pin 5 low
  }

  //unlightRGB();

  if (millis() % (waitFactor) <= 300)
  {
    digitalWrite(LED_info, LOW);
    listen = false;
    timedStatement();
    listen = true;
    digitalWrite(LED_info, HIGH);
  }

  if (checkMaxAmp > ampThreshold && periodFound)
  {
    for (int i = 0; i < medianLength; i++) {
      frequency = 38462 / float(period); //calculate frequency timer rate/period
      median[i] = frequency;
      delay(10);
      sortFloat(median, medianLength);
    }

    listen = false;
    digitalWrite(LED_info, LOW);
    dataExists = false;
    //if (median[4] >= 300 && median[4] <= 2400)
    //if (median[4] <= 2800)
    //{
    if (int(median[4]) % int(myTone) <= 20 || int(myTone) % int(median[4]) <= 20) //observe this for possible conversion errors!
    {
      positiveReaction(median[4]);
    } else if (int(median[4]) % int(myTone) >= 20 || int(myTone) % int(median[4]) >= 20)
    {
      unsigned int wait;
      if (median[4] > myTone)
      {
        wait = median[4] - myTone * respondFactor;
      }
      else
      {
        wait = myTone - median[4] * respondFactor;
      }
      negativeReaction(median[4], wait);
    }

    //reset all values to avoid self-recording:
    //reset();
    //time = 0;//reset clock
    //noMatch = 0;
    index = 0;//reset index
    maxAmp = 0;
    checkMaxAmp = 0;
    ampTimer = 0;

    doNothing(3000);
    //}
    listen = true;
  }


  if (millis() % 200 <= 10)
  {
    if (millis() % 3600000 <= 200)
    {
      now = 0;
      digitalWrite(LED_R, HIGH);
      while (now < 150) {
        //do nothing
      }
      digitalWrite(LED_R, LOW);
      now = 0;
      digitalWrite(LED_G, HIGH);
      while (now < 150) {
        //do nothing
      }
      digitalWrite(LED_G, LOW);
      now = 0;
      digitalWrite(LED_B, HIGH);
      while (now < 150) {
        //do nothing
      }
      digitalWrite(LED_B, LOW);
      now = 0;
      digitalWrite(LED_info, HIGH);
      while (now < 150) {
        //do nothing
      }
      digitalWrite(LED_info, LOW);

      myTone = int(random(100, 2751));

      if (myTone >= 0 && myTone < 1500) {
        analogWrite(LED_R, 255);
      }
      if (myTone >= 750 && myTone < 2250) {
        analogWrite(LED_G, 255);
      }
      if (myTone >= 1250 && myTone < 2750) {
        analogWrite(LED_B, 255);
      }
      lightRGB();
      now = 0;
      while (now < 3000) {
        //do nothing / light LED
      }
      unlightRGB();
    }
  }
}

