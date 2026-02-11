int LED5 = 5;
int BUTTON4 = 4;

bool ledState = false;
int lastButton = LOW;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED5,OUTPUT);
  pinMode(BUTTON4,INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int currentButton = digitalRead(BUTTON4);

  if(currentButton == HIGH && lastButton == LOW ) {

    ledState = !ledState;
    if(ledState)
      digitalWrite(LED5,HIGH);
    else
      digitalWrite(LED5,LOW);
    
  delay(50);
  }

  lastButton = currentButton;
}
