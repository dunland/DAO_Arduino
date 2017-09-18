int note = 1000;

void setup() {
  Serial.begin(9600);
  pinMode(7, OUTPUT); //tone
  pinMode(8, OUTPUT); //trigger
}

void loop() {
  Serial.println(note);
  digitalWrite(8, HIGH);
  tone(7, note, 500);
  delay(2000);
  digitalWrite(8, LOW);

  note += 100;
}
