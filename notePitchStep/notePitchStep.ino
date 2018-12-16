int note = 100;
int i = 1;

void setup() {
  Serial.begin(9600);
  pinMode(7, OUTPUT); //tone
  pinMode(8, OUTPUT); //trigger
}

void loop() {
  Serial.println(note);
  //digitalWrite(8, HIGH);
  tone(8, note, 600);
  delay(2000);
  //digitalWrite(8, LOW);
  delay(3000);

  i++;
  //note = note*i;
  note += 100;
  if (note >= 3000)
  {
    note = 100;
  }

}
