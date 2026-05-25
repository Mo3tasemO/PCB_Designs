#include "HX711.h"

// Pin Definitions
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN  = 5;

HX711 scale;

// --- ADJUST THESE ---
// Start with 1.0, put a known weight on, then: 
// new_calibration_factor = (Reading / Known_Weight)
float calibration_factor = -7050.0; 

// Filtering Constants
float filtered_weight = 0.0;
const float alpha = 0.15; // Lower = smoother but slower
const float deadzone = 0.005; 

// Timing
unsigned long last_read_time = 0;
const int read_interval = 50; // Read every 50ms (20Hz)

void setup() {
  Serial.begin(115200);
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  
  Serial.println("Initializing scale... please do not place weight.");
  scale.tare(); // Reset the scale to 0
  Serial.println("System Ready. Send 't' to tare.");
}

void loop() {
  // 1. Non-blocking Serial Command Check
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't' || c == 'T') {
      scale.tare();
      filtered_weight = 0.0;
      Serial.println(">> Tared");
    }
  }

  // 2. Non-blocking Sensor Reading
  if (millis() - last_read_time >= read_interval) {
    last_read_time = millis();

    if (scale.is_ready()) {
      // get_units(1) is faster than get_units(5) for real-time filtering
      float raw_weight = scale.get_units(1);

      // Apply Exponential Moving Average (EMA) Filter
      filtered_weight = (alpha * raw_weight) + ((1.0 - alpha) * filtered_weight);

      // Apply Deadzone to prevent "ghost" drifting near zero
      float final_weight = (abs(filtered_weight) < deadzone) ? 0.0 : filtered_weight;
      
      float force_newtons = final_weight * 9.80665;

      // Output formatted for Serial Plotter
      Serial.print("Weight_kg:");
      Serial.print(final_weight, 3);
      Serial.print(",");
      Serial.print("Force_N:");
      Serial.println(force_newtons, 3);
    } else {
      // Optional: Serial.println("HX711 not found.");
    }
  }
}