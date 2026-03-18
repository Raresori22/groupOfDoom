#include <LiquidCrystal.h>
#include <TimerOne.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 7, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int buttonPin = 2;
volatile int counter = 0;

//volatile float timeEnd = millis();
//volatile float timeStart;
volatile float duration = 10.0;
volatile float pulse = 0.0;
volatile float frequency = 0.0;
void setup() {
  // put your setup code here, to run once:
  lcd.begin(20, 4);
  Serial.begin(9600);
  Timer1.initialize(10000000);
  Timer1.attachInterrupt(interrupt_routine);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterrupt, FALLING);
}

void loop() {
  // put your main code here, to run repeatedly:
    lcd.setCursor(0, 0);
    lcd.print("Pulses: ");
    lcd.print(pulse);
    lcd.setCursor(0, 1);
    lcd.print("Duration: ");
    lcd.print(duration);
    lcd.setCursor(0, 2);
    lcd.print("Frequency: ");
    lcd.print(frequency);

}

void buttonInterrupt() {
  /*timeStart = timeEnd;
  timeEnd = millis();
  if(counter != 0) {
  duration = (timeEnd - timeStart)/1000;
  }
  */
  counter++;
}

void interrupt_routine() {
  lcd.clear();
  pulse = counter;
  frequency = pulse / duration;
  counter = 0;
}