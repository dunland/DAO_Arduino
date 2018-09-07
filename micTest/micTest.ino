boolean blink = true;
int count = 0;
unsigned long waitUntil;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(5, OUTPUT);
  //pinMode(6, INPUT);
  //pinMode(9, INPUT);
}

void loop() {

  if (count%4000 == 0)
  {
    if (blink)
    {
      digitalWrite(5, HIGH);
      blink = false;
    }
    else
    {
      digitalWrite(5,LOW);
      blink = true;
    }
  }
  //Serial.print(analogRead(0));

  if (analogRead(0) >= 491)
  {
   //digitalWrite(10, HIGH);
   pinMode(6, OUTPUT);
   waitUntil = millis()+500;
  }

  if (millis()>waitUntil)
  {
    //digitalWrite(10, LOW);
    pinMode(6,INPUT);
  }
  count++;

}
