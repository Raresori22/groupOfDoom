#include <LiquidCrystal.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
const int LED4 = 4;
const int Dig6=6;

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(Dig6,INPUT);
  pinMode(A2,INPUT);
  Serial.begin(9600);
}

void loop() {
  lcd.setCursor(0, 0);
  int A2_state = analogRead(A2), pin_state = digitalRead(Dig6), timeOn=1000, timeOff = 1000;
    if(A2_state<200)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("    WARNING    ");
    lcd.setCursor(0, 1);
    lcd.print(" Value too low! ");
    delay(500);
  } else if(A2_state>800){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("    WARNING    ");
    lcd.setCursor(0, 1);
    lcd.print(" Value too high! ");
    delay(500);
  } else {
  lcd.print("Dig.:" + String(pin_state));
  lcd.print("Ana.:" + String(A2_state));
  Serial.println(A2_state);
  calculate(timeOn,timeOff);
  delay(500);
  lcd.clear();
  }
  
}


void calculate(float timeOn,float timeOff)
{
  lcd.setCursor(0, 1);
  lcd.print("dut:" + String((timeOn/(timeOn+timeOff))*100) + "%");
  lcd.print("Fr:" + String(1/((timeOff+timeOn)/1000)) + "Hz");
}
