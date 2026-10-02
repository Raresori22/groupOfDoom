#define Motor_return    0
#define Motor_forward     1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10
#include <LiquidCrystal.h>
#define LEFT_ENCA 2
#define RIGHT_ENCA 3


const int rs = 37, en = 36, d4 = 35, d5 = 34, d6 = 33, d7 = 32;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

volatile int counter = 0;
volatile unsigned long lastInterruptTime = 0;
volatile bool buttonPressed = false;
const unsigned long debounceDelay = 200;


int xValue, yValue;
float xValuePercentage, yValuePercentage;
int joystickButtonPin = 18;



void setup() {
  lcd.begin(20,4);
  Serial.begin(9600);
  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);
  pinMode(joystickButtonPin, INPUT_PULLUP);
  pinMode(LEFT_ENCA, INPUT);
  pinMode(RIGHT_ENCA, INPUT);
  attachInterrupt(digitalPinToInterrupt(joystickButtonPin), buttonInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(LEFT_ENCA), leftEncoderISR, RISING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_ENCA), rightEncoderISR, RISING);
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
  //controlCar();

  delay(100);
}

void buttonInterrupt() {
  unsigned long now = millis();
  if (now - lastInterruptTime > debounceDelay) {
    buttonPressed = true;
  }
  lastInterruptTime = now;
}

volatile unsigned long leftPulses = 0;
volatile unsigned long rightPulses = 0;

void leftEncoderISR() {
  leftPulses++;
}

void rightEncoderISR() {
  rightPulses++;
}

void controlCar() {


  int x = map(xValue, 0, 1023, -255, 255);
  int y = map(yValue, 0, 1023, -255, 255);

  int leftMotor = y + x;
  int rightMotor = y - x;

  leftMotor = constrain(leftMotor, -255, 255);
  rightMotor = constrain(rightMotor, -255, 255);


  if (leftMotor >= 0) {
    digitalWrite(Motor_L_dir_pin, Motor_return);
    analogWrite(Motor_L_pwm_pin, leftMotor);
  } 
  else {
    digitalWrite(Motor_L_dir_pin, Motor_forward);
    analogWrite(Motor_L_pwm_pin, -leftMotor);
  }

  if (rightMotor >= 0) {
    digitalWrite(Motor_R_dir_pin, Motor_return);
    analogWrite(Motor_R_pwm_pin, rightMotor);
  } 
  
  else {
    digitalWrite(Motor_R_dir_pin, Motor_forward);
    analogWrite(Motor_R_pwm_pin, -rightMotor);
  }

}

const float distanceCm = 1.0;         // Distance you want to drive
const float leftPulsesPerCm = 13.11;     // Enter your measured value
const float rightPulsesPerCm = 13.45;    // Enter your measured value

void moveCar() {
  if (leftPulsesPerCm <= 0 || rightPulsesPerCm <= 0) {
    Serial.println("Enter both pulses-per-cm values first.");
    return;
  }

unsigned long leftTarget =
    (unsigned long)(distanceCm * leftPulsesPerCm + 0.5);
unsigned long rightTarget =
    (unsigned long)(distanceCm * rightPulsesPerCm + 0.5);

if (leftTarget > 0) leftTarget--;
if (rightTarget > 0) rightTarget--;

  digitalWrite(Motor_L_dir_pin, Motor_forward);
  digitalWrite(Motor_R_dir_pin, Motor_forward);

  noInterrupts();
  leftPulses = 0;
  rightPulses = 0;
  interrupts();

  analogWrite(Motor_L_pwm_pin, 100);
  analogWrite(Motor_R_pwm_pin, 100);

  bool leftRunning = true;
  bool rightRunning = true;
  unsigned long startTime = millis();

  while (leftRunning || rightRunning) {
    unsigned long leftCount, rightCount;

    noInterrupts();
    leftCount = leftPulses;
    rightCount = rightPulses;
    interrupts();

    if (leftRunning && leftCount >= leftTarget) {
      analogWrite(Motor_L_pwm_pin, 0);
      leftRunning = false;
    }

    if (rightRunning && rightCount >= rightTarget) {
      analogWrite(Motor_R_pwm_pin, 0);
      rightRunning = false;
    }

    // Stop both motors if an encoder stops reporting pulses.
    if (millis() - startTime >= 10000) {
      analogWrite(Motor_L_pwm_pin, 0);
      analogWrite(Motor_R_pwm_pin, 0);
      break;
    }
  }

  noInterrupts();
  unsigned long finalLeft = leftPulses;
  unsigned long finalRight = rightPulses;
  interrupts();

  Serial.print("Left: ");
  Serial.print(finalLeft);
  Serial.print(" / ");
  Serial.println(leftTarget);

  Serial.print("Right: ");
  Serial.print(finalRight);
  Serial.print(" / ");
  Serial.println(rightTarget);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Left A: ");
  lcd.print(finalLeft);
  lcd.setCursor(0, 1);
  lcd.print("Right A: ");
  lcd.print(finalRight);
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
