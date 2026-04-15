#include <LiquidCrystal.h>
#include <TimerOne.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SPI.h>

#define MAC_6   0x92

unsigned int Port = 1883;  // MQTT server port
byte server[] = { 10, 6, 0, 23 };
EthernetClient ethClient; // Client object from Ethernet library
PubSubClient client(server, Port, ethClient); // Create PubSubClient object

#define outTopic   "ICT4_out_2020" // Topic to which the message is sent

static uint8_t mymac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };// 08-8F-C3-C7-F9-14

char* deviceId = "2020a2202"; // username
char* clientId = "a2202"; // client ID 
char* deviceSecret = "tamk";
char buffer[100];

void fetchIP();

IPAddress storedIP;
byte connection = 1;

const int rs = 9, en = 2, d4 = 5, d5 = 4, d6 = 7, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
volatile bool updateFlag = false;
int buttonPin = 3;
volatile long counter = 0;
double weatherData;
volatile double duration = 1.0;
volatile double pulse = 0.0;
volatile double frequency = 0.0;
int lightButton = 14;
int humidityButton = 17;
int bothButton = 19;
int  light = 0, humidity = 0;


void setup() {
  lcd.begin(20, 4);
  Serial.begin(9600);

  if (Ethernet.linkStatus() != LinkOFF) {
    Ethernet.begin(mymac);
  }
  delay(500);
  connect_MQTT_server();

  pinMode(A4, INPUT);
  pinMode(lightButton, INPUT);
  pinMode(humidityButton, INPUT);
  pinMode(bothButton, INPUT);

  Timer1.initialize(1000000);
  Timer1.attachInterrupt(interrupt_routine);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterrupt, FALLING);
}

void loop() {
  
    if (Ethernet.linkStatus() != LinkOFF && Ethernet.localIP() == IPAddress(0,0,0,0)) {
      Ethernet.begin(mymac);
    }
    if(!client.connected()) {
      connect_MQTT_server();
    }
    client.loop();

    int A4_state=analogRead(A4);

    if(digitalRead(lightButton) == HIGH)
    {
      light = 1;
      humidity = 0;
    } else if(digitalRead(humidityButton) == HIGH){
      humidity = 1;
      light = 0;
    } else if(digitalRead(bothButton) == HIGH) {
      humidity = 1;
      light = 1;
    }
    if(updateFlag)
    {
      updateFlag = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Light: ");
      lcd.print(A4_state);
      lcd.setCursor(19, 0);
      if(light == 1)
      lcd.print("1");
      lcd.setCursor(0, 1);
      if (Ethernet.linkStatus() == LinkOFF) {
      lcd.print("fail ethernet");
      } else {
        lcd.print("IP: ");
        lcd.print(Ethernet.localIP());
      }
      lcd.setCursor(0, 2);
      lcd.print("Frequency: ");
      lcd.print(frequency);
      lcd.setCursor(0, 3);
      weatherData = (8568 - frequency) / 16.7;
      if(weatherData < 0 || weatherData > 100)
      {
      lcd.print("Invalid Humidity");
      } else {
        lcd.print("Humidity: ");
        lcd.print(weatherData);
      }
      lcd.setCursor(19, 3);
      if(humidity == 1)
      lcd.print("1");
    }
  if(weatherData < 0 || weatherData > 100)
  {
    send_MQTT_message(A4_state, "Invalid");
  } else send_MQTT_message(A4_state, weatherData);
    delay(5000);
}

void buttonInterrupt() {
  counter++;
}

void interrupt_routine() {
  updateFlag = true;
  pulse = counter;
  frequency = pulse / duration;
  counter = 0;
}

void send_MQTT_message(int data, String invalidData) {

  if (light == 1 && humidity == 1) {

    sprintf(buffer, "GoD light_Data & humidity_Data: %d | %s", data, invalidData.c_str());

    if (client.publish(outTopic, buffer)) {
      Serial.println("Data sent:");
      Serial.println(buffer);
    } else {
      Serial.println("Send failed!");
    }

  } 
  else if (light == 1) {

    sprintf(buffer, "GoD light_Data: %d", data);

    if (client.publish(outTopic, buffer)) {
      Serial.println("Data sent:");
      Serial.println(buffer);
    } else {
      Serial.println("Send failed!");
    }

  } 
  else if (humidity == 1) {

    sprintf(buffer, "GoD humidity_Data: %s", invalidData.c_str());

    if (client.publish(outTopic, buffer)) {
      Serial.println("Data sent:");
      Serial.println(buffer);
    } else {
      Serial.println("Send failed!");
    }

  }

}

void send_MQTT_message(int data, double humidData) {

  char humStr[10];  
  dtostrf(humidData, 4, 2, humStr);

  if (light == 1 && humidity == 1) {

    sprintf(buffer, "GoD light_Data & humidity_Data: %d | %s", data, humStr);

    if (client.publish(outTopic, buffer)) {
      Serial.println("Data sent:");
      Serial.println(buffer);
    } else {
      Serial.println("Send failed!");
    }

  } 
  else if (light == 1) {

    sprintf(buffer, "GoD light_Data: %d", data);

    if (client.publish(outTopic, buffer)) {
      Serial.println("Data sent:");
      Serial.println(buffer);
    } else {
      Serial.println("Send failed!");
    }

  } 
  else if (humidity == 1) {

    sprintf(buffer, "GoD humidity_Data: %s", humStr);

    if (client.publish(outTopic, buffer)) {
      Serial.println("Data sent:");
      Serial.println(buffer);
    } else {
      Serial.println("Send failed!");
    }

  }
}

void connect_MQTT_server() { 
    if (client.connect(clientId, deviceId, deviceSecret)) { // Attempt to connect to MQTT broker
        Serial.println("Connected OK"); // Connection successful
    } else {
        Serial.println("Connection failed."); // Connection failed
    }    
}
