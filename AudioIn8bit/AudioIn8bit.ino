float sig;

void setup() {
  Serial.begin(9600);
}

void loop() {
  sig = analogRead(0) / 4;
  Serial.println(sig);
}
