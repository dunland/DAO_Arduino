int note = 100;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  tone(8,note);
  delay(2000);
  note += 100;
  if (note >= 3000)
  {
    note = 100;
  }
  
}
