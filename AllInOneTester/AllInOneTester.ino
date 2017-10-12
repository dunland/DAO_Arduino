int rgb = 6;

void setup() {
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);

}

void loop() {
  if (rgb == 9) {
    rgb = 10;
  } else if (rgb == 10) {
    rgb = 6;
  } else if (rgb == 6) {
    rgb = 9;
  }
  digitalWrite(5, HIGH);
  digitalWrite(3, HIGH);
  delay(1000);
  digitalWrite(3, LOW);
  digitalWrite(5, LOW);
  delay(1000);

  digitalWrite(5, HIGH);
  digitalWrite(rgb, HIGH);
  delay(1000);
  digitalWrite(5, LOW);
  digitalWrite(rgb, LOW);
  delay(1000);

  digitalWrite(5, HIGH);
  digitalWrite(8, HIGH);
  tone(7, 1300, 1000);
  delay(1000);
  digitalWrite(5, LOW);
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);
  delay(1000);
}
