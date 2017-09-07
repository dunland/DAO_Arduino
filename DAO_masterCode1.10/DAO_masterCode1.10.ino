/*OTHERING MACHINES v.1.10. - after computer breakdown
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
   also, my mid-point is at 107 (ca. 2V), this needs to be double-checked.
*/

//reaction and behavior
int actMode = 1;
unsigned long now = 0; //for delay functions
int playTone = 0;

//database variables
const int dbLength = 10;
int db[dbLength];
int abundancy[dbLength];
boolean dataExists = false;
boolean dbFull = false;

//clipping indicator variables
boolean clipping = 0;

//data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally
int timer[10];//sstorage for timing of events
int slope[10];//storage for slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//storage for period of wave
byte index = 0;//current storage index
float frequency;//storage for frequency calculations
int maxSlope = 0;//used to calculate max slope as trigger point
int newSlope;//storage for incoming slope data

const int medianLength = 9; //for smoothing data
float median[medianLength];

int incomingData = 0; //saved signal to compare with database


//variables for decided whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 60;//slope tolerance- adjust this if you need
int timerTol = 5;//timer tolerance- adjust this if you need

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 4;//raise if you have a very noisy signal
byte midpoint = 92;

void setup() {

  Serial.begin(9600);

  pinMode(13, OUTPUT); //led indicator pin
  pinMode(12, OUTPUT); //output pin
  pinMode(7, OUTPUT); //speaker tone out
  pinMode(8, OUTPUT); //speaker transistor trigger

  //fill and print database
  for (int i = 0; i < dbLength / 2; i++) {
    float rndm = random(50, 500);
    while (int(rndm) % 20 != 0) {
      rndm++;
    }

    db[i] = rndm;
    abundancy[i]++;
    Serial.print(db[i]);
    Serial.print(" | ");
  }


  cli();//diable interrupts

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

  PORTB &= B11101111;//set pin 12 low
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
        PORTB |= B00010000;//set pin 12 high
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
        //reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
        PORTB |= B00010000;//set pin 12 high
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

  if (newData == 0 || newData == 1023) { //if clipping
    PORTB |= B00100000;//set pin 13 high- turn on clipping indicator led
    clipping = 1;//currently clipping
  }

  time++;//increment timer at rate of 38.5kHz

  //printing frequency only of signal loud enough
  ampTimer++;//increment amplitude timer
  if (abs(midpoint - ADCH) > maxAmp) {
    maxAmp = abs(midpoint - ADCH);
  }
  if (ampTimer == 1000) {
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }

}

void reset() { //clea out some variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}


void checkClipping() { //manage clipping indicator LED
  if (clipping) { //if currently clipping
    PORTB &= B11011111;//turn off clipping indicator led
    clipping = 0;
  }
}

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

void checkIncomingData(int incomingData) {
  if (incomingData != 0) {

    //data exists:
    for (int i = 0; i < dbLength; i++) {
      if (incomingData == db[i]) {
        Serial.print("incoming data already existent.");
        abundancy[i]++;
        now = millis();
        actMode = 2;
        dataExists = true;
        playTone = incomingData;
        break;
      }
    }

    //data does not exist:
    if (!dataExists) {
      if (!dbFull) {
        for (int i = 0; i < dbLength; i++) {
          if (db[i] == 0) {
            Serial.print("incoming data added to database.");
            db[i] = incomingData;
            abundancy[i]++;
            actMode = 3;
            playTone = incomingData + random(-300, 300); //should be closest instead
            now = millis();
            break;
          }
          if (i == dbLength - 1) {
            Serial.print("end of database reached.");
            dbFull = true;
          }
        }
      }
      if (dbFull) {
        int lowest = abundancy[0];
        int index = 0;
        for (int i = 0; i < dbLength; i++) {
          if (abundancy[i] < lowest) {
            lowest = abundancy[i];
            index = i;
          }
        }
        abundancy[index]--;
        if (abundancy[index] == 0) {
          db[index] = incomingData;
          abundancy[index] = 1;
          actMode = 3;
          playTone = incomingData + random(-300, 300);
          now = millis();
        }
      }
    }

    //print database and abundancies
    for (int i = 0; i < dbLength; i++) {
      Serial.print(db[i]);
      Serial.print(" | ");
    }
      for (int i = 0; i < dbLength; i++) {
      Serial.print(abundancy[i]);
      Serial.print(" | ");
    }
    dataExists = false;
    Serial.println();
  } else {
    Serial.println("incomingData == 0");
  }
}

void loop() {

  checkClipping();

  if (actMode == 1) {
    for (int i = 0; i < medianLength; i++) {
      if (checkMaxAmp > ampThreshold) {
        frequency = 38462 / float(period); //calculate frequency timer rate/period
        median[i] = frequency;
        //Serial.println(frequency);
        delay(10);
        sortFloat(median, medianLength);
      }
    }

    Serial.print("new median = ");
    Serial.println(median[4]);
    incomingData = median[4];
    checkIncomingData(median[4]);
  }

  if (actMode == 2 && millis() > now + 3000) {
    //emit same sound as heard, but stay

    digitalWrite(8, HIGH);
    tone(7, playTone, 500);
  }

  if (actMode == 3 && millis() > now + 3000) {
    //emit closest sound, wait 4s, then move away
    digitalWrite(8, HIGH);
    tone(7, playTone, 500);
  }
}

