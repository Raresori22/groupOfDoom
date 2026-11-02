#include <Wire.h>
#include <LiquidCrystal.h>

#define Motor_return    0
#define Motor_forward     1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10
#define LEFT_ENCA 3
#define RIGHT_ENCA 2
#define CMPS14_address 0x60


const int rs = 37, en = 36, d4 = 35, d5 = 34, d6 = 33, d7 = 32;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

volatile int counter = 0;
volatile unsigned long lastInterruptTime = 0;
volatile bool buttonPressed = false;
const unsigned long debounceDelay = 200;


int xValue, yValue;
float xValuePercentage, yValuePercentage;
int joystickButtonPin = 18;

const int headingTolerance = 4;
const unsigned long TURN_TIMEOUT = 10000;
const float HEADING_KP = 3.0;
const int turnAngle = 110;
float heading = 0;
const float headingOffset = 0.0;
String headingCommand = "";
String distanceCommand = "";

void setup() {
  lcd.begin(20,4);
  Serial.begin(9600);
  Wire.begin();
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
  lcd.setCursor(0,2);
  lcd.print("Left A: 0");
  lcd.setCursor(0,3);
  lcd.print("Right A: 0");
}

void loop() {
  readCompass();
  showHeading();
  readValues();

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
    driveRoute();
    buttonPressed = false;
  }
  // controlCar();

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

const float leftPulsesPerCm = 13.11;     // Enter your measured value
const float rightPulsesPerCm = 13.45;    // Enter your measured value

void readValues(){
  if (Serial.available() > 0){
    String message = Serial.readStringUntil('\n'); 
    Serial.print("Message received, content: ");  
    Serial.println(message);

    if (message.indexOf("LCD") > -1){
      Serial.println("Command = LCD ");
      int pos_s = message.indexOf(":");

      if (pos_s > -1){
        String stat = message.substring(pos_s + 1);
        lcd.setCursor(0,0);
        lcd.print(stat);
      } 
    }

    else if(message.indexOf("dist") > -1){
      Serial.println("Command = dist ");
      int pos_s = message.indexOf(":");
      if (pos_s > -1){
        message.trim();
        distanceCommand = message;
        interface1();
        int distance = message.substring(pos_s + 1).toInt();
        moveCar(distance);
      }
    }

    else if(message.indexOf("degree") > -1){
      Serial.println("Command = degree ");
      handleDegreeCommand(message);
    }

    else{
	    Serial.println("No greeting found, try typing Print:Hi or Print:Hello\n");
    }
  }
}

void handleDegreeCommand(String message) {
  int pos = message.indexOf(':');
  if (pos == -1) return;
  

  int degree = message.substring(pos + 1).toInt();
  if (degree < 0 || degree >= 360) {
    Serial.println("Heading must be between 0 and 359 degrees.");
    return;
  }

  message.trim();
  headingCommand = message;
  headingCommand.replace("degree", "deg");
  interface1();
  find_heading(degree);
}

void moveCar(int distanceCm) {
  if (distanceCm == 0) return;

  if (leftPulsesPerCm <= 0 || rightPulsesPerCm <= 0) {
    Serial.println("Enter both pulses-per-cm values first.");
    return;
  }

  float distance = distanceCm;
  if (distance < 0) distance = -distance;
  unsigned long leftTarget  = (unsigned long)(distance * leftPulsesPerCm + 0.5);
  unsigned long rightTarget = (unsigned long)(distance * rightPulsesPerCm + 0.5);
  if (leftTarget > 0) leftTarget--;
  if (rightTarget > 0) rightTarget--;

  int direction = (distanceCm < 0) ? Motor_return : Motor_forward;
  digitalWrite(Motor_L_dir_pin, direction);
  digitalWrite(Motor_R_dir_pin, direction);

  noInterrupts();
  leftPulses = 0;
  rightPulses = 0;
  interrupts();

  analogWrite(Motor_L_pwm_pin, 100);
  analogWrite(Motor_R_pwm_pin, 100);

  bool leftRunning = true;
  bool rightRunning = true;
  unsigned long lastLeftCount = 0, lastRightCount = 0;
  unsigned long lastProgressTime = millis();
  const unsigned long STALL_TIMEOUT = 1000;   // ms with no new pulses = stalled

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

    // Reset the stall timer whenever a running wheel makes progress
    if ((leftRunning && leftCount != lastLeftCount) ||
        (rightRunning && rightCount != lastRightCount)) {
      lastProgressTime = millis();
    }
    lastLeftCount = leftCount;
    lastRightCount = rightCount;

    // Safety stop: an encoder stopped reporting or a wheel is blocked
    if (millis() - lastProgressTime >= STALL_TIMEOUT) {
      analogWrite(Motor_L_pwm_pin, 0);
      analogWrite(Motor_R_pwm_pin, 0);
      Serial.println("Stopped: encoder stalled");
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

  lcd.setCursor(0, 2);
  lcd.print("Left A: ");
  lcd.print(finalLeft);
  lcd.print("    ");
  lcd.setCursor(0, 3);
  lcd.print("Right A: ");
  lcd.print(finalRight);
  lcd.print("    ");


}


void interface1() {
    lcd.setCursor(0, 0);
    lcd.print("x:");
    lcd.print(xValue);
    lcd.print("    ");
    lcd.setCursor(6, 0);
    lcd.print(xValuePercentage, 0);
    lcd.print("%    ");
    lcd.setCursor(10, 0);
    lcd.print(headingCommand.substring(0, 10));
    lcd.print("  ");
    lcd.setCursor(0, 1);
    lcd.print("y:");
    lcd.print(yValue);
    lcd.print("    ");
    lcd.setCursor(6, 1);
    lcd.print(yValuePercentage, 0);
    lcd.print("%   ");
    lcd.setCursor(10, 1);
    lcd.print(distanceCommand.substring(0, 10));
    lcd.print("  ");

}

void interface2() {
    lcd.setCursor(0, 0);
    lcd.print("Push Counter:");
    lcd.print(counter);
    lcd.print("   ");

    lcd.setCursor(0, 1);
    lcd.print("                "); 
}

bool readCompass() {
  Wire.beginTransmission(CMPS14_address);
  Wire.write(0x01);
  byte err = Wire.endTransmission(false);

  if (err != 0) {
    Serial.print("err: ");
    Serial.println(err);
    return false;
  }

  Wire.requestFrom((uint8_t)CMPS14_address, (uint8_t)1, (uint8_t)true);

  if (Wire.available()) {
    uint8_t bearing = Wire.read();
    heading = bearing * 360.0 / 256.0 - headingOffset;
    if (heading < 0) {
      heading = heading + 360;
    }
    return true;
  }

  return false;
}

void showHeading() {
  String direction;

  if (heading >= 23 && heading < 68) {
    direction = "NE";
  } else if (heading >= 68 && heading < 113) {
    direction = "E";
  } else if (heading >= 113 && heading < 158) {
    direction = "SE";
  } else if (heading >= 158 && heading < 203) {
    direction = "S";
  } else if (heading >= 203 && heading < 248) {
    direction = "SW";
  } else if (heading >= 248 && heading < 293) {
    direction = "W";
  } else if (heading >= 293 && heading < 338) {
    direction = "NW";
  } else {
    direction = "N";
  }

  /*Serial.print(heading);
  Serial.print(" ");
  Serial.println(direction);*/

  lcd.setCursor(14, 2);
  lcd.print("      ");
  lcd.setCursor(14, 2);
  lcd.print((int)heading);
  lcd.print(" ");
  lcd.print(direction);
}

void find_heading(int given_degree) {
  unsigned long startTime = millis();

  while (true) {
    if (readCompass() == false) {
      analogWrite(Motor_L_pwm_pin, 0);
      analogWrite(Motor_R_pwm_pin, 0);
      Serial.println("Compass read failed");
      break;
    }
    showHeading();

    float error = given_degree - heading;
    if (error > 180) {
      error = error - 360;
    }
    if (error < -180) {
      error = error + 360;
    }

    if (error >= -headingTolerance && error <= headingTolerance) {
      analogWrite(Motor_L_pwm_pin, 0);
      analogWrite(Motor_R_pwm_pin, 0);
      break;
    }

    if (millis() - startTime > TURN_TIMEOUT) {
      analogWrite(Motor_L_pwm_pin, 0);
      analogWrite(Motor_R_pwm_pin, 0);
      Serial.println("Stopped: turn timeout");
      break;
    }

    int speed;
    if (error > 0) {
      speed = error * 2;
      digitalWrite(Motor_L_dir_pin, Motor_forward);
      digitalWrite(Motor_R_dir_pin, Motor_return);
    } else {
      speed = -error * 2;
      digitalWrite(Motor_L_dir_pin, Motor_return);
      digitalWrite(Motor_R_dir_pin, Motor_forward);
    }
    speed = constrain(speed, 60, 150);

    analogWrite(Motor_L_pwm_pin, speed);
    analogWrite(Motor_R_pwm_pin, speed);

    delay(10);
  }
}

bool driveStraight(float distance, int speedPercent, int holdHeading) {
  int speed = 255 * speedPercent / 100;
  unsigned long leftCount = 0, rightCount = 0;
  unsigned long lastLeftCount = 0, lastRightCount = 0;
  unsigned long lastProgressTime = millis();
  const unsigned long STALL_TIMEOUT = 1000;

  noInterrupts();
  leftPulses = 0;
  rightPulses = 0;
  interrupts();

  digitalWrite(Motor_L_dir_pin, Motor_forward);
  digitalWrite(Motor_R_dir_pin, Motor_forward);

  while (true) {
    noInterrupts();
    leftCount = leftPulses;
    rightCount = rightPulses;
    interrupts();

    float leftCm = leftCount / leftPulsesPerCm;
    float rightCm = rightCount / rightPulsesPerCm;
    if ((leftCm + rightCm) / 2 >= distance) {
      break;
    }

    int correction = 0;
    if (readCompass() == true) {
      float error = holdHeading - heading;
      if (error > 180) {
        error = error - 360;
      }
      if (error < -180) {
        error = error + 360;
      }
      correction = error * HEADING_KP;
      correction = constrain(correction, -speed / 2, speed / 2);
    }

    analogWrite(Motor_L_pwm_pin, constrain(speed + correction, 0, 255));
    analogWrite(Motor_R_pwm_pin, constrain(speed - correction, 0, 255));

    if (leftCount != lastLeftCount || rightCount != lastRightCount) {
      lastProgressTime = millis();
    }
    lastLeftCount = leftCount;
    lastRightCount = rightCount;

    if (millis() - lastProgressTime >= STALL_TIMEOUT) {
      analogWrite(Motor_L_pwm_pin, 0);
      analogWrite(Motor_R_pwm_pin, 0);
      Serial.println("Stopped: encoder stalled");
      return false;
    }
  }

  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);

  Serial.print("Left: ");
  Serial.print(leftCount);
  Serial.print("  Right: ");
  Serial.println(rightCount);

  lcd.setCursor(0, 2);
  lcd.print("Left A: ");
  lcd.print(leftCount);
  lcd.print("  ");
  lcd.setCursor(0, 3);
  lcd.print("Right A: ");
  lcd.print(rightCount);
  lcd.print("  ");

  return true;
}

void driveRoute() {
  if (readCompass() == false) {
    Serial.println("Compass read failed");
    return;
  }

  int firstHeading = heading;
  int secondHeading = firstHeading - turnAngle;
  if (secondHeading < 0) {
    secondHeading = secondHeading + 360;
  }
  int thirdHeading = secondHeading - turnAngle;
  if (thirdHeading < 0) {
    thirdHeading = thirdHeading + 360;
  }

  Serial.println("Route start");

  if (driveStraight(20, 75, firstHeading) == false) {
    return;
  }
  delay(300);
  find_heading(secondHeading);
  delay(300);

  if (driveStraight(13, 25, secondHeading) == false) {
    return;
  }
  delay(300);
  find_heading(thirdHeading);
  delay(300);

  if (driveStraight(20, 50, thirdHeading) == false) {
    return;
  }

  Serial.println("Route done");
}
