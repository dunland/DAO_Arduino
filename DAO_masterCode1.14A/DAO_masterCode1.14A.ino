/*OTHERING MACHINES v.1.14A. - COM5/Arduino)
   responsive behavior and dynamics of two communicating devices
   incomingData limited to > 300
   loop fix: long waiting times after sound emission!

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

//OUTPUT PINS:
const int LED = 5;
const int VIBR = 3;
const int TRIGGER = 8;
const int SPEAK = 7;

//reaction and behavior
unsigned long now = 0; //for delay functions
int freqTol = 50;
int waitFactor = 150;
boolean listen = true;

//database variables
const int dbLength = 10;
int db[dbLength];
int ab[dbLength];
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

//smoothing data
const int medianLength = 9;
float median[medianLength];


//variables for decided whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 100;//slope tolerance- higher for steep waves
int timerTol = 3;//timer tolerance- low for complicated waves/higher resolution

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 25;//raise if you have a very noisy signal
byte midpoint = 195;

void setup() {

  Serial.begin(9600);
  randomSeed(analogRead(0));

  pinMode(13, OUTPUT); //led indicator pin
  pinMode(12, OUTPUT); //output pin
  pinMode(SPEAK, OUTPUT); //speaker tone out
  pinMode(TRIGGER, OUTPUT); //speaker transistor trigger
  pinMode(LED, OUTPUT); //for LED --> positive reaction
  pinMode(VIBR, OUTPUT); //for vibrationmotor --> negative reaction

  boolean initEntryExist = true;
  float rndm;
  Serial.print("waiting factor = ");
  Serial.println(waitFactor);
  //fill and print database
  Serial.println("filling database... ");
  for (int i = 0; i < dbLength / 2; i++) {
numgenerator: while (initEntryExist) {
      //generate new random entry dividable by 20
      rndm = random(300, 2500);
      while (int(rndm) % 20 != 0) {
        rndm++;
      }
      //check existance
      for (int i = 0; i < dbLength; i++) {
        if (db[i] == rndm) {
          goto numgenerator;
        }
      } break;
    }

    //add to db
    db[i] = rndm;
    ab[i] += 2;
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

  if (listen) {
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

void printDatabase() {
  for (int i = 0; i < dbLength; i++) {
    Serial.print(db[i]);
    Serial.print(" | ");
  }
  Serial.println();
  for (int i = 0; i < dbLength; i++) {
    Serial.print(ab[i]);
    Serial.print(" | ");
  }
  Serial.println();
}

void sortDatabase(int a[], int size) {
  Serial.println("sorting database...");
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
  printDatabase();
}


void positiveReaction(int playTone) {
  //light LED, then emit same as heard
  Serial.println("######## APPROVE #######");
  now = millis();
  while (millis() < now + 4500) {
    if (millis() % 1000 == 0) {
      Serial.print("light LED / wait.. ");
    }
    digitalWrite(LED, HIGH);
  }
  Serial.println();
  digitalWrite(LED, LOW);

  digitalWrite(TRIGGER, HIGH);
  now = millis();
  while (millis() < now + 300) {
    tone(SPEAK, playTone, 10);
    if (millis() % 200 == 0) {
      Serial.print("producing the same tone.. ");
    }
  }
  Serial.println();
  digitalWrite(TRIGGER, LOW);

  //Abklingzeit
  now = millis();
  while (millis() < now + 2000) {
    if (millis() % 500 == 0) {
      Serial.print("waiting again.. ");
    }
  }
  Serial.println();
  //reset all values to avoid self-recording:
  checkMaxAmp = 0;
  ampTimer = 0;
  for (int i = 0; i < medianLength; i++) {
    median[i] = 0;
  }
  frequency = 0;
}

void negativeReaction(int playTone, unsigned long wait) {
  Serial.println("######## REJECT ########");
  //vibrate:
  now = millis();
  while (millis() < now + 1000) {
    if (millis() % 250) {
      Serial.print("vibrating.. ");
    }
    digitalWrite(VIBR, HIGH);
  }
  digitalWrite(VIBR, LOW);
  Serial.println();

  //wait a bit:
  now = millis();
  while (millis() < now + wait) {
    if (millis() % 1000 == 0) {
      Serial.print(", waiting another ");
      Serial.print(abs(now + wait - millis()));
    }
  }
  Serial.println();

  //play tone:
  digitalWrite(TRIGGER, HIGH);
  now = millis();
  while (millis() < now + 300) {
    tone(SPEAK, playTone, 10);
    if (millis() % 200 == 0) {
      Serial.print("producing another tone.. ");
    }
  }
  Serial.println();
  digitalWrite(TRIGGER, LOW);


  //subside
  now = millis();
  while (millis() < now + 2000) {
    if (millis() % 500 == 0) {
      Serial.print("waiting again.. ");
    }
  }
  Serial.println();
  //reset all values to avoid self-recording:
  checkMaxAmp = 0;
  ampTimer = 0;
  for (int i = 0; i < medianLength; i++) {
    median[i] = 0;
  }
  frequency = 0;


}

void checkIncomingData(int incomingData) {
  if (incomingData > 300) {

    //data (incomingData within acceptable tolerance): increase abundancy
    for (int i = 0; i < dbLength; i++) {
      if (abs(incomingData - db[i]) < freqTol) {
        ab[i]++;
        dataExists = true;
        positiveReaction(incomingData);
        break;
      }
    }

    //data does not exist:
    if (!dataExists) {
      if (!dbFull) {
        Serial.println("######### db fillup ########");
        for (int i = 0; i < dbLength; i++) {
          if (db[i] == 0) {
            Serial.println("incoming data added to database.");
            db[i] = incomingData;
            ab[i]++;
            now = millis();
            while (millis() < now + 250) {
              digitalWrite(LED, HIGH);
            }
            digitalWrite(LED, LOW);
            while (millis() < now + 4750) {
              if (millis() % 200 == 0) {
                Serial.print("doing nothing for a while.. ");
              }
            }
            break;
          }
          if (i == dbLength - 1) {
            Serial.println("----------------end of database reached.----------------");
            dbFull = true;
          }
        }
      }

      //full database: lower abundancy of lowest, overwrite if zero
      if (dbFull) {
        int lowest = ab[0];
        int index = 0;
        for (int i = 0; i < dbLength; i++) {
          if (ab[i] < lowest) {
            lowest = ab[i];
            index = i;
          }
        }
        //lower abundancy
        ab[index]--;
        //if abundancy = 0, add data
        if (ab[index] == 0) {
          Serial.print(db[index]);
          Serial.print(" dropped, ");
          Serial.print(incomingData);
          Serial.println(" added");

          db[index] = incomingData;
          ab[index] = 1;

          //pick closest entry from db and respond
          sortDatabase(db, dbLength);
          unsigned long wait = 0;
          int closest = 0;

          if (index == 0) {
            //entry at start: wait dist
            wait = (db[index + 1] - db[index]) * waitFactor;
            closest = db[index + 1];
            Serial.print("entry at 0, wait = ");
            Serial.println(wait);
          } else if (index == dbLength - 1) {
            wait = (db[index] - db[index - 1]) * waitFactor;
            closest = db[dbLength - 2];
            Serial.print("entry at end, wait = ");
            Serial.println(wait);
            //entry at end: wait dist
          } else {
            //middle: calc diff, wait dist
            if (db[index] - db[index - 1] < db[index + 1] - db[index]) {
              wait = (db[index] - db[index - 1]) * waitFactor;
              closest = db[index - 1];
              Serial.print(db[index]);
              Serial.print(" - ");
              Serial.print(db[index - 1]);
              Serial.print(" < ");
              Serial.print(db[index + 1]);
              Serial.print(" - ");
              Serial.print(db[index]);
              Serial.print(", difference/wait = ");
              Serial.println(wait);
            } else {
              wait = (db[index + 1] - db[index]) * waitFactor;
              closest = db[index + 1];
              Serial.print(db[index]);
              Serial.print(" - ");
              Serial.print(db[index - 1]);
              Serial.print(" > ");
              Serial.print(db[index + 1]);
              Serial.print(" - ");
              Serial.print(db[index]);
              Serial.print(", difference/wait = ");
              Serial.println(wait);
            }
          }
          Serial.print("closest sound = ");
          Serial.println(closest);
          negativeReaction(closest, wait);
        }
      }
    }

    printDatabase();
  } else {
    Serial.println("incomingData < 300");
  }
  listen = true;
}

void loop() {

  checkClipping();

  //digitalWrite(TRIGGER, LOW);

  if (checkMaxAmp > ampThreshold) {
    for (int i = 0; i < medianLength; i++) {
      frequency = 38462 / float(period); //calculate frequency timer rate/period
      median[i] = frequency;
      now = millis();
      while (millis() < now + 40) {
        //delay(40);
      }
      sortFloat(median, medianLength);
    }

    Serial.println();
    //Serial.print("new median = ");
    //Serial.println(median[4]);
    Serial.print("new frequency = ");
    Serial.println(frequency);
    dataExists = false;
    //checkIncomingData(median[4]);
    listen = false;
    checkIncomingData(frequency);
  }

}

