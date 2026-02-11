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
  pinMode(PIN4,INPUT);
  pinMode(A2,INPUT);
  Serial.begin(9600);
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  int A2_state = analogRead(A2), pin_state = digitalRead(LED4), timeOn=1000, timeOff = 1000;
  digitalWrite(LED_BUILTIN, HIGH);
  delay(timeOn);                      
  pin_state = digitalRead(LED_BUILTIN);   
  digitalWrite(LED_BUILTIN, LOW);
  pin_state = digitalRead(LED_BUILTIN);
  Serial.println(pin_state);
  lcd.setCursor(0, 0);
  delay(50);
  lcd.print("TV: ");
  lcd.print(A2_state);
  delay(timeOff); 
 // calculate(timeOn,timeOff);
}


void calculate(float timeOn,float timeOff)
{
  lcd.setCursor(0, 0);
  lcd.print("DC: " + String((timeOn/(timeOn+timeOff))*100) + "%");
  lcd.setCursor(0, 1);
  lcd.print("Fr: ");
  lcd.print(1/((timeOff+timeOn)/1000));
  lcd.print("Hz");
}
