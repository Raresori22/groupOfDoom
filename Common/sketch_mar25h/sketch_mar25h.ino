#include <Ethernet.h>

#define MAC_6    0x73
static uint8_t mymac[6] = { 0x44, 0x76, 0x58, 0x10, 0x00, MAC_6 };

void fetchIP();

void setup() {
  Serial.begin(9600);
  fetchIP();
  Serial.print("Hello");
}

void loop() { Serial.print("Hello Loop");}

void fetchIP() {
  byte connection = 1;
  connection = Ethernet.begin(mymac);
  
  Serial.print(F("\nW5100 Revision "));
  if (connection == 0) {
    Serial.println(F("Failed to access Ethernet controller"));
  }
  
  Serial.println(F("Setting up DHCP"));
  Serial.print("Connected with IP: ");
  Serial.println(Ethernet.localIP());
  delay(1500);
}
