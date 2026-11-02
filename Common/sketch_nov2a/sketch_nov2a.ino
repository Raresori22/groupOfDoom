#include <LiquidCrystal.h>
const int rs = 37, en = 36, d4 = 35, d5 = 34, d6 = 33, d7 = 32;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup(){
  lcd.begin(20,4);
  Serial.begin(9600);
  Serial.println("Write something to the serial monitor.");
}

void loop(){
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
        
      }
    }

    else if(message.indexOf("degree") > -1){
      Serial.println("Command = degree ");
      int pos_s = message.indexOf(":");
      if (pos_s > -1){
        
      }
    }

    else{
	    Serial.println("No greeting found, try typing Print:Hi or Print:Hello\n");
    }
  }
}