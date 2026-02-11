const int LED5 = 5;
const int LED4 = 4;

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED5, OUTPUT);
  pinMode(PIN4,INPUT);
  pinMode(A2,INPUT);
  Serial.begin(9600);
}

// the loop function runs over and over again forever
void loop() {
  int A2_state = analogRead(A2), pin_state = digitalRead(LED4), timeOn=1000, timeOff = 1000;
  digitalWrite(LED5, HIGH);
  delay(timeOn);                      
  pin_state = digitalRead(LED4);   
  digitalWrite(LED5, LOW);
  pin_state = digitalRead(LED4);
  Serial.println(pin_state);
  Serial.print("Trimmer Value: ");
  Serial.println(A2_state);
  delay(timeOff); 
  calculate(timeOn,timeOff);
}

void calculate(float timeOn,float timeOff)
{
  Serial.println("Duty cycle: " + String((timeOn/(timeOn+timeOff))*100) + "%");
  Serial.print("Frequency: ");
  Serial.print(1/((timeOff+timeOn)/1000));
  Serial.println("Hz");
}


// 5. Maximum frequency is 1kHz. A delay function halts the execution of code until a certain condition is met Ex: timer running out, user input, conditional statement. It can stop your code from running or not stop from running when intended.
// 6. Serial.println prints a comment then moves to the next line and Serial.print stays on the same line
// 9. It's considered HIGH 
// 11. 0 - 1023