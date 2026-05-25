#include "HX711.h"


const int LOADCELL_DOUT_PIN = 21;
const int LOADCELL_SCK_PIN = 22;

HX711 scale;

float calibration_factor = 1000.0; 

void setup() {
  Serial.begin(9600); 
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  
  scale.set_scale(calibration_factor);
  scale.tare(); 
  
  delay(1000); 
}

void loop() {
  if (scale.is_ready()) {
  
    float weight_kg = scale.get_units(1); 
  
    float force_newtons = weight_kg * 9.81;
    
    Serial.println(force_newtons);
  } 
  else {
    
    Serial.println(0); 
  }
  
  delay(120); 
}