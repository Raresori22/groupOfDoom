#include <LiquidCrystal.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
const int LED4 = 4;
const int Dig6=6;
const int Dig7=7;
int count = 10;
unsigned long timeStart,timeEnd,time;
int voltage = 4;
void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(Dig6,INPUT);
  pinMode(A2,INPUT);
  pinMode(Dig7,OUTPUT);
  Serial.begin(9600); // 1336/1344 for 115200 and 1320 for 4800/9600
  
}

void loop() {
  int A2_state = analogRead(A2), pin_state = digitalRead(Dig6);
  float A1_state = analogRead(A1);
  float average2 = average(count);
  lcd.setCursor(0, 0);
  float voltScaled = average2/204.6;
  voltScaled = (voltScaled * 50) + 50;
  lcd.print("VS: " + String(voltScaled));
  if(voltage > average2){
    digitalWrite(Dig7, LOW);
  }
  else{
    digitalWrite(Dig7, HIGH);
  }
  lcd.print("V " + String(A1_state/204.6));
  delay(500);
  lcd.clear();
}


float average(int count) {
  int sum = 0;
  timeStart = micros();
  for(int i=0;i<count;i++)
  {
     sum += analogRead(A1);
  }
  Serial.println("Hieno juttu, kiitos paljon!"); // this adds ~200 MS
  timeEnd = micros();
  time = timeEnd-timeStart;
  float averageS = sum/count;
  lcd.setCursor(0, 1);
  lcd.print("Avg:" + String(int(averageS)));

  lcd.print("S: " + String(time));
  return averageS/204.6;
}