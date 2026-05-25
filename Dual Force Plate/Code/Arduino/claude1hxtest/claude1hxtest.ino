// ============================================================
//  HX711 Load Cell Test Sketch for ESP32
//  Works with both Serial Monitor and Serial Plotter
// ============================================================

#include "HX711.h"          // Include the HX711 library

// --- Pin Definitions ---
#define DOUT_PIN 27         // HX711 DT/DOUT → ESP32 GPIO 27
#define SCK_PIN  26         // HX711 SCK/CLK → ESP32 GPIO 26

// --- HX711 Object ---
HX711 scale;                // Create an HX711 object called "scale"

// --- Settings ---
const int AVG_SAMPLES = 10; // Number of readings to average

void setup() {
  Serial.begin(115200);     // Start Serial at 115200 baud
  delay(500);

  Serial.println("==============================");
  Serial.println("  HX711 Load Cell Test");
  Serial.println("==============================");

  scale.begin(DOUT_PIN, SCK_PIN);  // Initialize HX711
  delay(1000);                     // Give HX711 time to wake up

  Serial.println("Waiting for HX711...");
  while (!scale.is_ready()) {      // Keep trying until it responds
    delay(100);
  }
  Serial.println("HX711 detected!");

  Serial.println("Taring... remove any weight and wait.");
  delay(2000);                     // 2 seconds to remove weight
  scale.tare();                    // Zero the scale
  Serial.println("Tare complete. Scale is now at zero.");
  Serial.println("------------------------------");
}

void loop() {
  if (scale.is_ready()) {

    long rawReading = scale.read();                    // Single raw reading
    long avgReading = scale.read_average(AVG_SAMPLES); // Averaged reading

    // --- Serial Monitor: human-readable ---
    Serial.print("Raw: ");
    Serial.print(rawReading);
    Serial.print("  |  Avg: ");
    Serial.println(avgReading);

    // --- Serial Plotter: labeled CSV format ---
    // Open Tools → Serial Plotter to see live graph
    // (Close Serial Monitor first)
    Serial.print("Raw:");
    Serial.print(rawReading);
    Serial.print(",");
    Serial.print("Avg:");
    Serial.println(avgReading);

  } else {
    Serial.println("HX711 not ready — check connection.");
  }

  delay(500);   // Half second between readings (faster = smoother graph)
}