#define Motor_return    0
#define Motor_forward     1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10
#include <LiquidCrystal.h>
#define ENCA 2
#define ENCA 3

// TABLE 21.5 (1304)  = 60.65, 21,2(1295) = 61.08, 21.8 (1282) = 58.8 AVG pulses per cm = 60.18

// FLOOR 20.8 (2219) =  106.68 , 19.9 (2176) = 109.35, 19.8(2026) = 102.32 AVG pulses per cm = 106.11

const int rs = 37, en = 36, d4 = 35, d5 = 34, d6 = 33, d7 = 32;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

volatile int counter = 0;
volatile unsigned long lastInterruptTime = 0;
volatile bool buttonPressed = false;
const unsigned long debounceDelay = 200;


int xValue, yValue;
float xValuePercentage, yValuePercentage;
int joystickButtonPin = 18;
volatile unsigned long encoderPulses = 0;



void setup() {
  lcd.begin(20,4);
  Serial.begin(9600);
  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);
  pinMode(joystickButtonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(joystickButtonPin), buttonInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENCA),encoderISR,RISING);
}

void loop() {
  xValue = analogRead(A8);
  yValue = analogRead(A9);
  xValuePercentage = (xValue/1023.0) * 100;
  yValuePercentage = (yValue/1023.0) * 100;
  // int(255 * xValuePercentage)
  if(counter % 2 == 0) {
    interface1();
  } else {
   interface2();
  }
  if(buttonPressed) {
    buttonPressed = false;
    counter++;
    moveCar();
  }
  controlCar();

  delay(100);
}

void buttonInterrupt() {
  unsigned long now = millis();
  if (now - lastInterruptTime > debounceDelay) {
    buttonPressed = true;
  }
  lastInterruptTime = now;
}

void encoderISR() {
  encoderPulses++;
}

void controlCar() {


  int x = map(xValue, 0, 1023, -255, 255);
  int y = map(yValue, 0, 1023, -255, 255);

  int leftMotor = y + x;
  int rightMotor = y - x;

  leftMotor = constrain(leftMotor, -255, 255);
  rightMotor = constrain(rightMotor, -255, 255);


  if (leftMotor >= 0) {
    digitalWrite(Motor_L_dir_pin, Motor_forward);
    analogWrite(Motor_L_pwm_pin, leftMotor);
  } 
  else {
    digitalWrite(Motor_L_dir_pin, Motor_return);
    analogWrite(Motor_L_pwm_pin, -leftMotor);
  }

  if (rightMotor >= 0) {
    digitalWrite(Motor_R_dir_pin, Motor_forward);
    analogWrite(Motor_R_pwm_pin, rightMotor);
  } 
  else {
    digitalWrite(Motor_R_dir_pin, Motor_return);
    analogWrite(Motor_R_pwm_pin, -rightMotor);
  }

}

void moveCar() {

  encoderPulses = 0;

  digitalWrite(Motor_L_dir_pin, Motor_forward);
  digitalWrite(Motor_R_dir_pin, Motor_forward);

  analogWrite(Motor_L_pwm_pin, 100);
  analogWrite(Motor_R_pwm_pin, 100);

  delay(4000);

  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);


  Serial.print("Encoder pulses: ");
  Serial.println(encoderPulses);
}

void interface1() {
    lcd.setCursor(0, 0);
    lcd.print("x:");
    lcd.print(xValue);
    lcd.print("    ");
    lcd.setCursor(9, 0);
    lcd.print(xValuePercentage, 0);
    lcd.print("%    ");
    lcd.setCursor(0, 1);
    lcd.print("y:");
    lcd.print(yValue);
    lcd.print("    ");
    lcd.setCursor(9, 1);
    lcd.print(yValuePercentage, 0);
    lcd.print("%   ");
}

void interface2() {
    lcd.setCursor(0, 0);
    lcd.print("Push Counter:");
    lcd.print(counter);
    lcd.print("   ");

    lcd.setCursor(0, 1);
    lcd.print("                "); 
}
