/*OTHERING MACHINES v.1.19B - COM4/Genuino
   RGB LED Code
   startup LED check
   initial dB reduced to 1 entry
   rgbLED pins: R = 9, G = 10, B = 6 //because PWM-PINs 3 and 11 interfere with tone()
   fillup range now random(300,2401)
   higehst BaudRate
   dbLength = 6
   B = 204 +/- 20

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

//OUTPUT PINS:
const int LED_info = 5;
const int LED_R = 9;
const int LED_G = 10;
const int LED_B = 6;
const int VIBR = 3;
const int TRIGGER = 8;
const int SPEAK = 7;

//reaction and behavior
unsigned long now = 0; //for delay functions
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
float abTot = 0; //total abundancy of frequencies in db

//database variables
const int dbLength = 6;
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
byte slopeTol = 255;//slope tolerance- higher for steep waves
int timerTol = 5;//timer tolerance- low for complicated waves/higher resolution

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 15;//raise if you have a very noisy signal
byte midpoint = 204;

void setup() {

  Serial.begin(115200);
  randomSeed(analogRead(0));
  respondFactor = random(100, 201); //that is a factor of 0.1 to 0.2 seconds
  waitFactor = respondFactor * 600; //time to pass before timedStatement

  pinMode(13, OUTPUT); //led indicator pin
  pinMode(12, OUTPUT); //output pin
  pinMode(SPEAK, OUTPUT); //speaker tone out
  pinMode(TRIGGER, OUTPUT); //speaker transistor trigger
  pinMode(LED_info, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(VIBR, OUTPUT); //for vibrationmotor --> negative reaction

  //LED startup check
  now = millis();
  digitalWrite(LED_R, HIGH);
  while (millis() < now + 150) {
    //do nothing
  }
  digitalWrite(LED_R, LOW);
  now = millis();
  digitalWrite(LED_G, HIGH);
  while (millis() < now + 150) {
    //do nothing
  }
  digitalWrite(LED_G, LOW);
  now = millis();
  digitalWrite(LED_B, HIGH);
  while (millis() < now + 150) {
    //do nothing
  }
  digitalWrite(LED_B, LOW);
  now = millis();
  digitalWrite(LED_info, HIGH);
  while (millis() < now + 150) {
    //do nothing
  }
  digitalWrite(LED_info, LOW);

  //print variables
  Serial.print("respondFactor = ");
  Serial.println(respondFactor);
  Serial.print("waitFactor = ");
  Serial.println(waitFactor);

  //fill and print database
  Serial.print("initial dB entry = ");
  float rndm;
  //generate single random entry dividable by 20
  rndm = random(300, 2401);
  while (int(rndm) % 20 != 0) {
    rndm++;
  }
  db[0] = rndm;
  Serial.println(db[0]);
  ab[0] = 1;
  abTot = 1;
  if (rndm >= 300 && rndm < 1000) {
    redAb++;
  } else if (rndm >= 1000 && rndm < 1700) {
    greenAb++;
  } else if (rndm >= 1700 && rndm < 2401) {
    blueAb++;
  }
  lightLED();



  //  //filling multiple randomly:
  //boolean initEntryExist = true;
  //  for (int i = 0; i < dbLength / 2; i++) {
  //numgenerator: while (initEntryExist) {
  //      //generate new random entry dividable by 20
  //      rndm = random(300, 2401);
  //      while (int(rndm) % 20 != 0) {
  //        rndm++;
  //      }
  //      //check existence
  //      for (int i = 0; i < dbLength; i++) {
  //        if (db[i] == rndm) {
  //          goto numgenerator;
  //        }
  //      } break;
  //    }
  //
  //    //add to db
  //    db[i] = rndm;
  //    ab[i] += 1;
  //    abTot += 1;
  //    Serial.print(db[i]);
  //    Serial.print(" | ");
  //  }




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

void timedStatement() {
  int highest = ab[0];
  int highestIdx = 0;
  for (int i = 0; i < dbLength; i++) {
    if (ab[i] > highest) {
      highestIdx = i;
    }
  }
  int playTone = db[highestIdx];
  digitalWrite(TRIGGER, HIGH);
  digitalWrite(LED_info, HIGH);
  now = millis();
  while (millis() < now + 600) {
    tone(SPEAK, playTone, 10);
    if (millis() % 200 == 0) {
      Serial.print(" ..emitting most frequent tone = ");
      Serial.print(playTone);
    }
  }
  Serial.println();
  digitalWrite(TRIGGER, LOW);
  digitalWrite(LED_info, LOW);

  now = millis();
  while (millis() < now + 4000) {
    //do nothing
  }
}

void lightLED() {
  //set RGB_LED
  red = (redAb / abTot) * 255;
  green = (greenAb / abTot) * 255;
  blue = (blueAb / abTot) * 255;
  analogWrite(LED_R, red);
  analogWrite(LED_G, green);
  analogWrite(LED_B, blue);

  Serial.print("Intensities: red = ");
  Serial.print(red);
  Serial.print(", green = ");
  Serial.print(green);
  Serial.print(", blue = ");
  Serial.println(blue);
}

void positiveReaction(int playTone) {
  //light LED, then emit same as heard
  Serial.println("######## APPROVE #######");
  now = millis();
  while (millis() < now + 1000) {
    digitalWrite(LED_info, HIGH);
  }
  digitalWrite(LED_info, LOW);
  now = millis();
  while (millis() < now + 3000) {
    //do nothing
  }

  digitalWrite(TRIGGER, HIGH);
  digitalWrite(LED_info, HIGH);
  now = millis();
  while (millis() < now + 600) {
    tone(SPEAK, playTone, 10);
  }
  Serial.println();
  digitalWrite(TRIGGER, LOW);
  digitalWrite(LED_info, LOW);

  //Abklingzeit
  now = millis();
  while (millis() < now + 3000) {
    //do nothing
  }
}

void negativeReaction(int playTone, unsigned long wait) {
  Serial.println("######## REJECT ########");
  //vibrate:
  now = millis();
  while (millis() < now + 1000) {
    digitalWrite(VIBR, HIGH);
  }
  digitalWrite(VIBR, LOW);

  //wait a bit:
  Serial.print("waiting time: ");
  Serial.println(wait);
  now = millis();
  while (millis() < now + wait) {}

  //play tone:
  digitalWrite(TRIGGER, HIGH);
  digitalWrite(LED_info, HIGH);
  now = millis();
  while (millis() < now + 600) {
    tone(SPEAK, playTone, 10);
  }
  digitalWrite(TRIGGER, LOW);
  digitalWrite(LED_info, LOW);


  //release time
  now = millis();
  while (millis() < now + 2000) {
    //do nothing
  }
}

void checkIncomingData(int incomingData) {
  if (incomingData >= 300 && incomingData <= 2400) {

    //data (incomingData within acceptable tolerance): increase abundancy
    for (int i = 0; i < dbLength; i++) {
      if (abs(incomingData - db[i]) < freqTol) {
        ab[i]++;
        abTot++;
        if (incomingData >= 300 && incomingData < 1000) {
          redAb++;
        } else if (incomingData >= 1000 && incomingData < 1700) {
          greenAb++;
        } else if (incomingData >= 1700 && incomingData < 2401) {
          blueAb++;
        }
        dataExists = true;
        positiveReaction(incomingData);
        break;
      }
    }

    //data does not exist:
    if (!dataExists) {
      if (!dbFull) {
        Serial.println("######### db fillup ########");
        //search 0 in database:
        for (int i = 0; i < dbLength; i++) {
          if (db[i] == 0) {
            db[i] = incomingData;
            ab[i]++;
            abTot++;
            if (incomingData >= 300 && incomingData < 1000) {
              redAb++;
            } else if (incomingData >= 1000 && incomingData < 1700) {
              greenAb++;
            } else if (incomingData >= 1700 && incomingData < 2401) {
              blueAb++;
            }
            now = millis();
            while (millis() < now + 250) {
              digitalWrite(LED_info, HIGH);
            }
            digitalWrite(LED_info, LOW);
            while (millis() < now + 2750) {         }
            break;
          }
          if (i == dbLength - 1) {
            Serial.println("----------------end of database reached.----------------");
            now = millis();
            while (millis() < now + 1000) {
              for (int i = 0; i < 500; i++) {
                digitalWrite(LED_info, HIGH);
              }
              for (int i = 0; i < 500; i++) {
                digitalWrite(LED_info, LOW);
              }
            }
            digitalWrite(LED_info, LOW);
            dbFull = true;
          }
        }
      }

      //full database: lower abundancy of lowest, overwrite if zero
      if (dbFull) {
        int lowest = ab[0];
        int lowestIdx = 0;
        for (int i = 0; i < dbLength; i++) {
          if (ab[i] < lowest) {
            lowest = ab[i];
            lowestIdx = i;
          }
        }
        //lower abundancy
        ab[lowestIdx]--;
        abTot--;
        if (incomingData >= 300 && incomingData < 1000) {
          redAb--;
        } else if (incomingData >= 1000 && incomingData < 1700) {
          greenAb--;
        } else if (incomingData >= 1700 && incomingData < 2401) {
          blueAb--;
        }
        //if abundancy = 0, add data
        if (ab[lowestIdx] == 0) {
          Serial.print(db[lowestIdx]);
          Serial.print(" dropped, ");
          Serial.print(incomingData);
          Serial.println(" added");

          db[lowestIdx] = incomingData;
          ab[lowestIdx] = 1;
          abTot++;
          if (incomingData >= 300 && incomingData < 1000) {
            redAb++;
          } else if (incomingData >= 1000 && incomingData < 1700) {
            greenAb++;
          } else if (incomingData >= 1700 && incomingData < 2401) {
            blueAb++;
          }

          //pick closest entry from db and respond
          sortDatabase(db, dbLength);
          unsigned int wait = 0;
          int closest = 0;

          if (lowestIdx == 0) {
            //entry at start: wait dist
            wait = (db[lowestIdx + 1] - db[lowestIdx]) * respondFactor;
            closest = db[lowestIdx + 1];
            Serial.print("entry at 0, wait = ");
            Serial.println(wait);
          } else if (lowestIdx == dbLength - 1) {
            wait = (db[lowestIdx] - db[lowestIdx - 1]) * respondFactor;
            closest = db[dbLength - 2];
            Serial.print("entry at end, wait = ");
            Serial.println(wait);
            //entry at end: wait dist
          } else {
            //middle: calc diff, wait dist
            if (db[lowestIdx] - db[lowestIdx - 1] < db[lowestIdx + 1] - db[lowestIdx]) {
              wait = (db[lowestIdx] - db[lowestIdx - 1]) * respondFactor;
              closest = db[lowestIdx - 1];
              Serial.print(db[lowestIdx]);
              Serial.print(" - ");
              Serial.print(db[lowestIdx - 1]);
              Serial.print(" < ");
              Serial.print(db[lowestIdx + 1]);
              Serial.print(" - ");
              Serial.print(db[lowestIdx]);
              Serial.print(", difference/wait = ");
              Serial.println(wait);
            } else {
              wait = (db[lowestIdx + 1] - db[lowestIdx]) * respondFactor;
              closest = db[lowestIdx + 1];
              Serial.print(db[lowestIdx]);
              Serial.print(" - ");
              Serial.print(db[lowestIdx - 1]);
              Serial.print(" > ");
              Serial.print(db[lowestIdx + 1]);
              Serial.print(" - ");
              Serial.print(db[lowestIdx]);
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
    Serial.println();
    lightLED();
    printDatabase();
  } else {
    Serial.println("incomingData out of range");
  }
  listen = true;
}

void loop() {

  checkClipping();

  if (millis() % (waitFactor) <= 300) {
    timedStatement();
  }

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
    Serial.print("new median = ");
    Serial.println(median[4]);
    //Serial.print("new frequency = ");
    //Serial.println(frequency);
    dataExists = false;
    listen = false;
    checkIncomingData(median[4]);
    //checkIncomingData(frequency);

    //reset all values to avoid self-recording:
    maxAmp = 0;
    checkMaxAmp = 0;
    ampTimer = 0;
    for (int i = 0; i < medianLength; i++) {
      median[i] = 0;
    }
    frequency = 0;
  }

}

