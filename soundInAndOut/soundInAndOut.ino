int note = 1000;
unsigned long now;

void setup() {
  Serial.begin(9600);
  pinMode(7, OUTPUT); //tone
  pinMode(8, OUTPUT); //trigger
}

void loop() {
  Serial.println(analogRead(0) / 4);
  //Serial.println(note);
  digitalWrite(8, HIGH);
  tone(7, note, 500);
  now = millis();
  while (millis() < now + 2000) {
    Serial.println(analogRead(0) / 4);
  }
  digitalWrite(8, LOW);

  note += 100;
}
