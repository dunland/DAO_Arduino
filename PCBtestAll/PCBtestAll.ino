/* based on blink sketch */
int serialTestNum = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);
  //pinMode(6, OUTPUT); RGB_LED on if pinMode = OUTPUT!
  pinMode(8, OUTPUT);
  //pinMode(9, OUTPUT);
  //pinMode(10, OUTPUT); 
}

// the loop function runs over and over again forever
void loop() {

  Serial.print("Serial test #"+serialTestNum);
  //LED
  digitalWrite(5, HIGH);
  delay(1000);
  digitalWrite(5, LOW);
  delay(1000);

  //LED B
  pinMode(6, OUTPUT);
  delay(1000);
  pinMode(6, INPUT);
  delay(1000);

  //LED R
  pinMode(9, OUTPUT);
  delay(1000);
  pinMode(9, INPUT);
  delay(1000);

  //LED G
  pinMode(10, OUTPUT);
  delay(1000);
  pinMode(10, INPUT);
  delay(1000);

  //speaker
  tone(8, 500, 500);
  delay(1000);

  //motor
  digitalWrite(3, HIGH);
  delay(100);
  digitalWrite(3, LOW);
  delay(1900);

  serialTestNum++;
}

