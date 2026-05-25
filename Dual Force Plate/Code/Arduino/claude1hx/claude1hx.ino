// Minimal HX711 detection test
#include "HX711.h"

HX711 scale;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting...");
  
  scale.begin(27, 26);   // DOUT=27, SCK=26
  
  for (int i = 0; i < 10; i++) {   // Try 10 times
    Serial.print("Attempt ");
    Serial.print(i + 1);
    Serial.print(": ");
    
    if (scale.is_ready()) {
      Serial.println("HX711 FOUND!");
      break;
    } else {
      Serial.println("Not found...");
    }
    delay(500);
  }
}

void loop() {}