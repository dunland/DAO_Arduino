int note = 100;

void setup() {
  pinMode(7, OUTPUT); //tone
  pinMode(8, OUTPUT); //trigger
}

void loop() {
  digitalWrite(8, HIGH);
  tone(7, note, 500);
  delay(2000);
  digitalWrite(8, LOW);

  note += 10;
}
