// Include the HX711 library (Make sure to install it via the Library Manager)
#include "HX711.h"

// Define the Arduino UNO pins connected to the HX711 module
const int HX711_DOUT_PIN = 27; // DT (Data) pin connected to Digital Pin 3
const int HX711_SCK_PIN = 26;  // SCK (Clock) pin connected to Digital Pin 2

// Create an instance of the HX711 class named 'scale'
HX711 scale;

void setup() {
  // Initialize the Serial Monitor communication at a baud rate of 9600
  Serial.begin(9600);
  
  // Print a startup message to the Serial Monitor
  Serial.println("Initializing the HX711 scale...");

  // Initialize the HX711 module with the defined data and clock pins
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);

  // Check if the HX711 module is physically connected and responding
  if (scale.is_ready()) {
    Serial.println("HX711 detected successfully!");
  } else {
    Serial.println("HX711 not found. Please check your wiring and restart.");
    // Freeze the program here if the module isn't detected
    while (1); 
  }

  // Perform 'Tare' to zero out the scale based on the current initial weight
  Serial.println("Tare in progress... Do not place any weight on the load cell.");
  scale.tare(); // This resets the current reading to 0 raw offset
  Serial.println("Tare complete. Starting readings...");
}

void loop() {
  // Check again if the scale is ready to send data before reading
  if (scale.is_ready()) {
    
    // Read a single instantaneous raw value relative to the tared zero point
    long raw_single = scale.get_value(1);
    
    // Read an average raw value from 10 consecutive readings for stability
    long raw_average = scale.get_value(10);

    // Print the single instantaneous raw reading to the Serial Monitor
    Serial.print("Raw Single Reading: ");
    Serial.print(raw_single);
    
    // Print a separator tab for clear spacing between values
    Serial.print("\t|\t");
    
    // Print the averaged raw reading to the Serial Monitor
    Serial.print("Raw Average (10 samples): ");
    Serial.println(raw_average);
    
  } else {
    // Error message if the HX711 suddenly becomes disconnected during operation
    Serial.println("HX711 not ready.");
  }

  // Delay for 500 milliseconds (half a second) before taking the next reading
  delay(500);
}