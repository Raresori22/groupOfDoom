#define Motor_return    0
#define Motor_forward     1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10
#include <LiquidCrystal.h>
#define LEFT_ENCA 3
#define RIGHT_ENCA 2
#include <Wire.h>
#define CMPS14_address 0x60

const int turnSpeed = 100;
const int headingTolerance = 4;


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
  Wire.begin();
  Wire.setWireTimeout(25000, true);
  delay(500);
  Serial.println("Scanning I2C addresses...");
  bool deviceFound = false;
  for (int address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(address, HEX);
      deviceFound = true;
    }
  }
  if (!deviceFound) {
    Serial.println("No I2C devices found");
  }
  Serial.println("Scan finished");
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
  if (buttonPressed) {
    buttonPressed = false;
    driveRoute();
    buttonPressed = false;
  }
  delay(50);
}

int readCompass() {
  int heading = -1;

  Wire.beginTransmission(CMPS14_address);
  Wire.write(1);

  int error = Wire.endTransmission();

  if (error == 0) {
    Wire.requestFrom(CMPS14_address, 1);

    if (Wire.available()) {
      int bearing = Wire.read();
      heading = bearing * 360L / 256;
      heading = (heading - 194 + 360) % 360; // Measured north reference before applying the offset
      const char* direction;

      if (heading < 23) direction = "N";
      else if (heading < 68) direction = "NE";
      else if (heading < 113) direction = "E";
      else if (heading < 158) direction = "SE";
      else if (heading < 203) direction = "S";
      else if (heading < 248) direction = "SW";
      else if (heading < 293) direction = "W";
      else if (heading < 338) direction = "NW";
      else direction = "N";

      lcd.setCursor(0, 0);
      lcd.print("Bearing: ");
      lcd.print(heading);
      lcd.print(" ");
      lcd.print(direction);
      lcd.print("    ");
      Serial.println(heading);
    } else {
      Serial.println("No data received");
    }
  } else {
    Serial.print("Compass communication error: ");
    Serial.println(error);
  }


  return heading;
}

void stopCar() {
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);
}

bool find_heading(int given_degree) {
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {
    int heading = readCompass();
    stopCar();
    if (heading < 0) return false;

    int difference = (given_degree - heading + 540) % 360 - 180;
    if (abs(difference) <= headingTolerance) return true;

    if (difference > 0) {
      digitalWrite(Motor_L_dir_pin, Motor_forward);
      digitalWrite(Motor_R_dir_pin, Motor_return);
    } else {
      digitalWrite(Motor_L_dir_pin, Motor_return);
      digitalWrite(Motor_R_dir_pin, Motor_forward);
    }
    analogWrite(Motor_L_pwm_pin, turnSpeed);
    analogWrite(Motor_R_pwm_pin, turnSpeed);
    delay(50);
  }
  stopCar();
  Serial.println("Turn timed out");
  return false;
}

void driveRoute() {
  int startHeading = readCompass();
  if (startHeading < 0) {
    stopCar();
    return;
  }

  if (!moveCar(20, 128)) return;
  if (!find_heading((startHeading + 360 - 110) % 360)) return;
  if (!moveCar(13, 64)) return;
  if (!find_heading((startHeading + 360 - 220) % 360)) return;
  if (!moveCar(20, 191)) return;
  Serial.println("Route finished");
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

const float leftPulsesPerCm = 13.48;    
const float rightPulsesPerCm = 13.85;    

bool moveCar(float distanceCm, int speed) {
  if (leftPulsesPerCm <= 0 || rightPulsesPerCm <= 0) {
    stopCar();
    Serial.println("Enter both pulses-per-cm values first.");
    return false;
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

  analogWrite(Motor_L_pwm_pin, speed * 189L / 200);
  analogWrite(Motor_R_pwm_pin, speed);

  bool leftRunning = true;
  bool rightRunning = true;
  unsigned long startTime = millis();

  while (leftRunning || rightRunning) {
  if (millis() - startTime >= 10000) {
    stopCar();
    Serial.println("Drive timed out");
    return false;
  }
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
  return true;
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
