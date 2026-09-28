#include <LiquidCrystal.h>

// ---------- Motor pins ----------
#define Motor_return     0
#define Motor_forward    1
#define Motor_L_dir_pin  7
#define Motor_R_dir_pin  8
#define Motor_L_pwm_pin  9
#define Motor_R_pwm_pin  10

// Encoder calibration notes (for when the encoder is fixed)
// TABLE 21.5 (1304) = 60.65, 21.2 (1295) = 61.08, 21.8 (1282) = 58.8    AVG pulses per cm = 60.18
// FLOOR 20.8 (2219) = 106.68, 19.9 (2176) = 109.35, 19.8 (2026) = 102.32 AVG pulses per cm = 106.11

// ---------- LCD ----------
const int rs = 37, en = 36, d4 = 35, d5 = 34, d6 = 33, d7 = 32;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// ---------- Joystick pins ----------
const int joystickXPin = A8;
const int joystickYPin = A9;
const int joystickButtonPin = 18;

// ---------- Driving settings (tune these) ----------
const int  DEADZONE   = 25;    // stick movement ignored around center (out of 255)
const int  MIN_PWM    = 60;    // lowest PWM where the wheels actually turn on the floor
const int  ACCEL_STEP = 10;    // max speed increase per update (~0.5 s from stop to full)
const int  DECEL_STEP = 25;    // max speed decrease per update (~0.2 s from full to stop)
const bool INVERT_X   = false; // set true if pushing right turns the car left
const bool INVERT_Y   = false; // set true if pushing forward drives backward
const bool LEFT_MOTOR_REVERSED  = false; // set true if the left wheel spins the wrong way
const bool RIGHT_MOTOR_REVERSED = false; // set true if the right wheel spins the wrong way

// ---------- Fixed test drive (button) ----------
const int DRIVE_PWM = 100;
const int R_TRIM    = 0;       // + or - on the right motor until the car drives straight
const unsigned long DRIVE_TIME = 4000;

// ---------- Timing ----------
const unsigned long CONTROL_INTERVAL = 20;   // ms between joystick/motor updates
const unsigned long LCD_INTERVAL     = 200;  // ms between LCD refreshes
unsigned long lastControlTime = 0;
unsigned long lastLcdTime = 0;

// ---------- Button ----------
int counter = 0;                        // only changed in loop(), so no volatile needed
unsigned long lastInterruptTime = 0;    // only used inside the ISR
volatile bool buttonPressed = false;
const unsigned long debounceDelay = 200;

// ---------- Joystick / motor state ----------
int xValue, yValue;
float xValuePercentage, yValuePercentage;
int xCenter = 512, yCenter = 512;       // measured at startup
int currentL = 0, currentR = 0;         // ramped speeds (-255..255)
int leftPwm = 0, rightPwm = 0;          // what's actually sent to the motors


void setup() {
  lcd.begin(20, 4);
  Serial.begin(9600);

  pinMode(Motor_L_dir_pin, OUTPUT);
  pinMode(Motor_R_dir_pin, OUTPUT);
  pinMode(Motor_L_pwm_pin, OUTPUT);
  pinMode(Motor_R_pwm_pin, OUTPUT);
  analogWrite(Motor_L_pwm_pin, 0);
  analogWrite(Motor_R_pwm_pin, 0);

  pinMode(joystickButtonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(joystickButtonPin), buttonInterrupt, FALLING);

  calibrateJoystick();   // don't touch the stick while powering on
}

void loop() {
  unsigned long now = millis();

  if (buttonPressed) {
    buttonPressed = false;
    counter++;
    lcd.clear();         // switching screens: start from a blank display
    updateLcd();
    moveCar();
    return;              // next loop re-reads the joystick fresh
  }

  if (now - lastControlTime >= CONTROL_INTERVAL) {
    lastControlTime = now;
    readJoystick();
    controlCar();
  }

  if (now - lastLcdTime >= LCD_INTERVAL) {
    lastLcdTime = now;
    updateLcd();
  }
}

void buttonInterrupt() {
  unsigned long now = millis();
  if (now - lastInterruptTime > debounceDelay) {
    buttonPressed = true;
  }
  lastInterruptTime = now;
}

// ---------- Joystick ----------

// Averages the resting position so the center doesn't have to be exactly 512
void calibrateJoystick() {
  long xSum = 0, ySum = 0;
  for (int i = 0; i < 20; i++) {
    xSum += analogRead(joystickXPin);
    ySum += analogRead(joystickYPin);
    delay(5);
  }
  // Fall back to a sane range if the stick was held or is disconnected
  xCenter = constrain(xSum / 20, 400, 624);
  yCenter = constrain(ySum / 20, 400, 624);
}

void readJoystick() {
  xValue = analogRead(joystickXPin);
  yValue = analogRead(joystickYPin);
  xValuePercentage = (xValue / 1023.0) * 100;
  yValuePercentage = (yValue / 1023.0) * 100;
}

// Converts a raw reading (0..1023) to a speed (-255..255) with a deadzone
int axisToSpeed(int raw, int center, bool invert) {
  int value;
  if (raw >= center) value = map(raw, center, 1023, 0, 255);
  else               value = map(raw, 0, center, -255, 0);

  if (abs(value) <= DEADZONE) return 0;

  // Rescale so speed starts from 0 right at the edge of the deadzone
  if (value > 0) value = map(value, DEADZONE, 255, 0, 255);
  else           value = map(value, -255, -DEADZONE, -255, 0);

  return invert ? -value : value;
}

// ---------- Driving ----------

void controlCar() {
  int x = axisToSpeed(xValue, xCenter, INVERT_X);
  int y = axisToSpeed(yValue, yCenter, INVERT_Y);

  // Arcade mixing: y = forward/back, x = turn
  int leftTarget  = y + x;
  int rightTarget = y - x;

  // Scale both sides down together so turning still works at full speed
  int biggest = max(abs(leftTarget), abs(rightTarget));
  if (biggest > 255) {
    leftTarget  = (long)leftTarget  * 255 / biggest;
    rightTarget = (long)rightTarget * 255 / biggest;
  }

  // Move gradually toward the target instead of jumping to it
  currentL = rampToward(currentL, leftTarget);
  currentR = rampToward(currentR, rightTarget);

  leftPwm  = applyMinPwm(currentL);
  rightPwm = applyMinPwm(currentR);
  writeMotor(Motor_L_dir_pin, Motor_L_pwm_pin, leftPwm,  LEFT_MOTOR_REVERSED);
  writeMotor(Motor_R_dir_pin, Motor_R_pwm_pin, rightPwm, RIGHT_MOTOR_REVERSED);
}

// Speeds up gently, slows down quickly, and pauses at 0 before reversing
int rampToward(int current, int target) {
  bool slowingDown = (current > 0 && target < current) || (current < 0 && target > current);

  if (slowingDown) {
    int next = (current > 0) ? max(current - DECEL_STEP, target)
                             : min(current + DECEL_STEP, target);
    if ((current > 0 && next < 0) || (current < 0 && next > 0)) next = 0;
    return next;
  }

  if (target > current) return min(current + ACCEL_STEP, target);
  return max(current - ACCEL_STEP, target);
}

// Skips the low PWM range where the motors only buzz
int applyMinPwm(int speed) {
  if (speed == 0) return 0;
  int pwm = map(abs(speed), 1, 255, MIN_PWM, 255);
  return (speed > 0) ? pwm : -pwm;
}

// Sends a signed PWM value (-255..255) to one motor
void writeMotor(int dirPin, int pwmPin, int pwm, bool reversed) {
  bool forward = (pwm >= 0);
  if (reversed) forward = !forward;
  digitalWrite(dirPin, forward ? Motor_forward : Motor_return);
  analogWrite(pwmPin, constrain(abs(pwm), 0, 255));
}

// Fixed 4-second forward drive (time-based until the encoder is fixed)
void moveCar() {
  writeMotor(Motor_L_dir_pin, Motor_L_pwm_pin, DRIVE_PWM,          LEFT_MOTOR_REVERSED);
  writeMotor(Motor_R_dir_pin, Motor_R_pwm_pin, DRIVE_PWM + R_TRIM, RIGHT_MOTOR_REVERSED);

  delay(DRIVE_TIME);

  writeMotor(Motor_L_dir_pin, Motor_L_pwm_pin, 0, LEFT_MOTOR_REVERSED);
  writeMotor(Motor_R_dir_pin, Motor_R_pwm_pin, 0, RIGHT_MOTOR_REVERSED);

  currentL = currentR = 0;   // joystick ramp restarts from standstill
  leftPwm = rightPwm = 0;
  buttonPressed = false;     // ignore presses made during the drive
}

// ---------- LCD ----------

void updateLcd() {
  if (counter % 2 == 0) interface1();
  else                  interface2();
}

void interface1() {
  lcd.setCursor(0, 0);
  lcd.print("x:");
  lcd.print(xValue);
  lcd.print("    ");
  lcd.setCursor(9, 0);
  lcd.print(xValuePercentage, 0);
  lcd.print("%   ");

  lcd.setCursor(0, 1);
  lcd.print("y:");
  lcd.print(yValue);
  lcd.print("    ");
  lcd.setCursor(9, 1);
  lcd.print(yValuePercentage, 0);
  lcd.print("%   ");

  // Motor outputs, handy for tuning DEADZONE and MIN_PWM
  lcd.setCursor(0, 2);
  lcd.print("L:");
  lcd.print(leftPwm);
  lcd.print("    ");
  lcd.setCursor(9, 2);
  lcd.print("R:");
  lcd.print(rightPwm);
  lcd.print("    ");
}

void interface2() {
  lcd.setCursor(0, 0);
  lcd.print("Push Counter:");
  lcd.print(counter);
  lcd.print("   ");
}
