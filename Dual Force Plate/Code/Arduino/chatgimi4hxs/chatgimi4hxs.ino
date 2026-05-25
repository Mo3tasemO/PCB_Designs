#include "HX711.h"

// --- CONFIGURATION MODE ---
// Set to 'true' for Serial Plotter (graphs)
// Set to 'false' for Serial Monitor (text reports)
bool plotMode = false; 

// --- PIN DEFINITIONS ---
const int UL_DOUT = 19; const int UL_CLK = 18;
const int LL_DOUT = 23; const int LL_CLK = 22;
const int UR_DOUT = 27; const int UR_CLK = 26;
const int LR_DOUT = 33; const int LR_CLK = 32;

// --- CALIBRATION FACTORS ---
// Adjust these after performing a calibration test for each sensor
float calUL = 420.0; 
float calLL = 420.0; 
float calUR = 420.0; 
float calLR = 420.0;

// Initialize 4 independent HX711 objects
HX711 upperLeft, lowerLeft, upperRight, lowerRight;

void setup() {
  Serial.begin(115200);
  
  // Initialize each sensor with its unique pins
  upperLeft.begin(UL_DOUT, UL_CLK);
  lowerLeft.begin(LL_DOUT, LL_CLK);
  upperRight.begin(UR_DOUT, UR_CLK);
  lowerRight.begin(LR_DOUT, LR_CLK);

  // Apply calibration factors
  upperLeft.set_scale(calUL);
  lowerLeft.set_scale(calLL);
  upperRight.set_scale(calUR);
  lowerRight.set_scale(calLR);

  if (!plotMode) {
    Serial.println("--- 4-Channel Scale System ---");
    Serial.println("Taring all sensors... Do not place weight.");
  }

  // Reset sensors to 0.0
  upperLeft.tare();
  lowerLeft.tare();
  upperRight.tare();
  lowerRight.tare();

  if (!plotMode) Serial.println("System Ready.");
}

void loop() {
  // Read individual values
  float valUL = getSensorValue(upperLeft, "Upper Left");
  float valLL = getSensorValue(lowerLeft, "Lower Left");
  float valUR = getSensorValue(upperRight, "Upper Right");
  float valLR = getSensorValue(lowerRight, "Lower Right");

  // Perform Calculations
  float leftTotal   = valUL + valLL;
  float rightTotal  = valUR + valLR;
  float topTotal    = valUL + valUR;
  float bottomTotal = valLL + valLR;
  float grandTotal  = valUL + valLL + valUR + valLR;

  if (plotMode) {
    // FORMAT FOR SERIAL PLOTTER
    // Format: Label:Value separated by spaces or tabs
    Serial.print("UL:"); Serial.print(valUL); Serial.print(" ");
    Serial.print("LL:"); Serial.print(valLL); Serial.print(" ");
    Serial.print("UR:"); Serial.print(valUR); Serial.print(" ");
    Serial.print("LR:"); Serial.println(valLR); 
  } else {
    // FORMAT FOR SERIAL MONITOR
    printTextReport(valUL, valLL, valUR, valLR, leftTotal, rightTotal, topTotal, bottomTotal, grandTotal);
  }

  // Small delay for stability
  delay(plotMode ? 20 : 1000); 
}

/**
 * Helper to read sensor safely with error checking
 */
float getSensorValue(HX711 &sensor, String label) {
  if (sensor.is_ready()) {
    return sensor.get_units(3); // Average of 3 readings
  } else {
    if (!plotMode) {
      Serial.print(label); Serial.println(" not connected");
    }
    return 0.0;
  }
}

/**
 * Helper to print structured text to Serial Monitor
 */
void printTextReport(float ul, float ll, float ur, float lr, float lTot, float rTot, float tTot, float bTot, float gTot) {
  Serial.println("\n==============================");
  Serial.printf("Upper Left:  %8.2f | Upper Right: %8.2f\n", ul, ur);
  Serial.printf("Lower Left:  %8.2f | Lower Right: %8.2f\n", ll, lr);
  Serial.println("------------------------------");
  Serial.printf("Left Total:  %8.2f | Right Total: %8.2f\n", lTot, rTot);
  Serial.printf("Top Total:   %8.2f | Bottom Total: %8.2f\n", tTot, bTot);
  Serial.println("------------------------------");
  Serial.printf("GRAND TOTAL: %8.2f\n", gTot);
}