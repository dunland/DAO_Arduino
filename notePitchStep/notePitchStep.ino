int note = 300;

void setup() {
  Serial.begin(9600);
  pinMode(7, OUTPUT); //tone
  pinMode(8, OUTPUT); //trigger
}

void loop() {
  Serial.println(note);
  digitalWrite(8, HIGH);
  tone(7, note, 1000);
  delay(1000);
  digitalWrite(8, LOW);
  delay(3000);

  note += 100;
}
