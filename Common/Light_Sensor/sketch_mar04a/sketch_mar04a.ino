#include <LiquidCrystal.h>


const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int min = 9999;
int max = -9999;
int button = 6;
int lastButton = LOW;
unsigned long pressStartTime = 0;



void setup() {
  // put your setup code here, to run once:
  lcd.begin(16, 2);
  pinMode(A4, INPUT);
  pinMode(6, INPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  int A4sum=0;
  for(int i = 0; i < 10; i++){
    int A4_state=analogRead(A4);
    A4sum += A4_state;
    if(A4_state<min) min=A4_state;
    if(A4_state>max) max=A4_state;
    delay(50);
  }
 
  int D6_state = digitalRead(button);

  if(D6_state == HIGH && lastButton == LOW){
    pressStartTime = millis();
   
  }
  lastButton = D6_state;
  
  if((millis() - pressStartTime >= 5000) && (D6_state == HIGH)){
    min = 9999;
    max = -9999;
    pressStartTime = 0;
  }
  int average = A4sum / 10;
  lcd.clear();
  lcd.print("avg:" + String(average));
  lcd.setCursor(0,1);
  lcd.print("min:" + String(min));
  lcd.print("max:" + String(max));
}
