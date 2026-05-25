// ============================================================
//  HX711 x4 Load Cell Test Sketch for ESP32
//  WITH BIOMECHANICS EVENT DETECTION (Jump Detection)
//  Works with Serial Monitor AND Serial Plotter
// ============================================================

#include <Arduino.h>
#include <HX711.h>

// --- Event Detection State Machine ---
enum JumpPhase {
  STANDING,        // Quiet baseline
  LOADING,         // Force rising (preparing to jump)
  FLIGHT,          // No contact (in the air)
  LANDING,         // Force spike from landing
};

struct JumpMetrics {
  JumpPhase phase;
  float contactStartForce;
  float peakForce;
  unsigned long contactStartTime;
  unsigned long flightStartTime;
  float contactTime;  // milliseconds
  float flightTime;   // milliseconds
  bool jumpDetected;
};

// --- Pin Definitions ---
#define DOUT_1  19
#define SCK_1   18

#define DOUT_2  23
#define SCK_2   22

#define DOUT_3  27
#define SCK_3   26

#define DOUT_4  33
#define SCK_4   32

// --- 4 HX711 Objects ---
HX711 scale1;
HX711 scale2;
HX711 scale3;
HX711 scale4;

// --- Settings ---
const int AVG_SAMPLES = 3;  // Light averaging - balance between instant & clean
const float SMOOTH_FACTOR = 0.6;  // Moderate smoothing (60% new, 40% old)

// --- Event Detection Thresholds ---
const float FORCE_THRESHOLD = 0.3;  // kg - force above baseline = contact
const float FLIGHT_THRESHOLD = 0.1;  // kg - force below this = flight phase
const unsigned long MIN_FLIGHT_TIME = 150;  // milliseconds - minimum air time to count as jump
long offset1 = 0, offset2 = 0, offset3 = 0, offset4 = 0;

// --- Smoothed output values ---
float smoothed_w_avg1 = 0, smoothed_w_avg2 = 0, smoothed_w_avg3 = 0, smoothed_w_avg4 = 0;
float smoothed_right = 0, smoothed_left = 0, smoothed_total = 0;

// --- Jump Metrics & State ---
JumpMetrics jumpMetrics;
float baselineTotal = 0;  // Baseline force for threshold calculations

// --- Calibration Factors (kilograms per ADC unit) ---
// TO CALIBRATE: Place known weight (e.g., 1 kg), note ADC reading, 
// then: calibration_factor = known_weight_kg / adc_reading
// Example: 1 kg gives ADC=95000 → factor = 1/95000 = 0.00001053
const float CAL_FACTOR_RU = 0.00001053;  // Right Up calibration
const float CAL_FACTOR_RD = 0.00001053;  // Right Down calibration
const float CAL_FACTOR_LU = 0.00001053;  // Left Up calibration
const float CAL_FACTOR_LD = 0.00001053;  // Left Down calibration

// --- Biomechanics Event Detection Function ---
void updateJumpPhase(float totalForce) {
  // Simple state machine for jump phase detection
  switch (jumpMetrics.phase) {
    case STANDING:
      // Transition to LOADING when force rises significantly
      if (totalForce > FORCE_THRESHOLD) {
        jumpMetrics.phase = LOADING;
        jumpMetrics.contactStartTime = millis();
        jumpMetrics.contactStartForce = totalForce;
        jumpMetrics.peakForce = totalForce;
      }
      baselineTotal = totalForce;  // Update baseline
      break;

    case LOADING:
      // Track peak force during loading/propulsion
      if (totalForce > jumpMetrics.peakForce) {
        jumpMetrics.peakForce = totalForce;
      }
      // Transition to FLIGHT when force drops below threshold
      if (totalForce < FLIGHT_THRESHOLD) {
        jumpMetrics.phase = FLIGHT;
        jumpMetrics.contactTime = (millis() - jumpMetrics.contactStartTime);
        jumpMetrics.flightStartTime = millis();
        jumpMetrics.jumpDetected = true;
      }
      break;

    case FLIGHT:
      // In the air - wait for landing (force spike)
      if (totalForce > FORCE_THRESHOLD) {
        jumpMetrics.phase = LANDING;
        jumpMetrics.flightTime = (millis() - jumpMetrics.flightStartTime);
      }
      break;

    case LANDING:
      // Brief landing phase - transition back to STANDING
      if (totalForce < FLIGHT_THRESHOLD) {
        jumpMetrics.phase = STANDING;
        jumpMetrics.jumpDetected = false;
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("============================================");
  Serial.println("   4x HX711 Load Cell - Auto Calibrating");
  Serial.println("============================================");
  Serial.println("Initializing scales...");

  scale1.begin(DOUT_1, SCK_1);
  scale2.begin(DOUT_2, SCK_2);
  scale3.begin(DOUT_3, SCK_3);
  scale4.begin(DOUT_4, SCK_4);
  delay(500);

  Serial.println("Checking all load cells...");
  int attempts = 0;
  bool allReady = false;
  while (!allReady && attempts < 30) {
    bool r1 = scale1.is_ready();
    bool r2 = scale2.is_ready();
    bool r3 = scale3.is_ready();
    bool r4 = scale4.is_ready();
    allReady = (r1 && r2 && r3 && r4);
    if (!allReady) {
      delay(100);
      attempts++;
    }
  }

  if (allReady) {
    Serial.println("✓ All 4 load cells detected and ready!");
  } else {
    Serial.println("✗ Warning: Not all load cells responding.");
  }

  Serial.println("Auto-taring all scales...");
  scale1.tare();
  scale2.tare();
  scale3.tare();
  scale4.tare();
  delay(500);

  Serial.println("Calibrating zero offset for all scales...");
  offset1 = scale1.read_average(20);
  offset2 = scale2.read_average(20);
  offset3 = scale3.read_average(20);
  offset4 = scale4.read_average(20);
  Serial.println("Calibration complete!");

  Serial.println("Ready! Starting measurements...");
  Serial.println("============================================");
  Serial.println("Output: Weight in kilograms (kg)");
  Serial.println("Commands: Send 'T' to re-tare (reset baseline)");
  Serial.println("Events: Jump detection enabled");
  Serial.println("RU_Avg,RD_Avg,LU_Avg,LD_Avg,Right_Limb,Left_Limb,Total_Force");
  
  // Initialize jump metrics
  jumpMetrics.phase = STANDING;
  jumpMetrics.contactStartForce = 0;
  jumpMetrics.peakForce = 0;
  jumpMetrics.contactStartTime = 0;
  jumpMetrics.flightStartTime = 0;
  jumpMetrics.contactTime = 0;
  jumpMetrics.flightTime = 0;
  jumpMetrics.jumpDetected = false;
}

void loop() {
  // --- Check for Serial commands ---
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'T' || cmd == 't') {
      Serial.println("Re-taring all scales...");
      offset1 = scale1.read_average(20);
      offset2 = scale2.read_average(20);
      offset3 = scale3.read_average(20);
      offset4 = scale4.read_average(20);
      Serial.println("✓ Tare complete! Baseline reset.");
    }
  }

  bool ready1 = scale1.is_ready();
  bool ready2 = scale2.is_ready();
  bool ready3 = scale3.is_ready();
  bool ready4 = scale4.is_ready();

  // --- Read all 4 cells (use 0 if not ready) and subtract calibration offset ---
  long raw1 = ready1 ? (offset1 - scale1.read()) : 0;
  long avg1 = ready1 ? (offset1 - scale1.read_average(AVG_SAMPLES)) : 0;

  long raw2 = ready2 ? (offset2 - scale2.read()) : 0;
  long avg2 = ready2 ? (offset2 - scale2.read_average(AVG_SAMPLES)) : 0;

  long raw3 = ready3 ? (offset3 - scale3.read()) : 0;
  long avg3 = ready3 ? (offset3 - scale3.read_average(AVG_SAMPLES)) : 0;

  long raw4 = ready4 ? (offset4 - scale4.read()) : 0;
  long avg4 = ready4 ? (offset4 - scale4.read_average(AVG_SAMPLES)) : 0;

  // --- Calculate limb totals and overall total ---
  float w_raw1 = raw1 * CAL_FACTOR_RU;
  float w_avg1 = avg1 * CAL_FACTOR_RU;

  float w_raw2 = raw2 * CAL_FACTOR_RD;
  float w_avg2 = avg2 * CAL_FACTOR_RD;

  float w_raw3 = raw3 * CAL_FACTOR_LU;
  float w_avg3 = avg3 * CAL_FACTOR_LU;

  float w_raw4 = raw4 * CAL_FACTOR_LD;
  float w_avg4 = avg4 * CAL_FACTOR_LD;

  // --- Limb forces (using averages for smoothness) ---
  float rightLimbForce = w_avg1 + w_avg2;  // Right Up + Right Down
  float leftLimbForce = w_avg3 + w_avg4;   // Left Up + Left Down
  float totalForce = rightLimbForce + leftLimbForce;

  // --- Apply exponential smoothing for ultra-clean output ---
  smoothed_w_avg1 = (SMOOTH_FACTOR * w_avg1) + ((1.0 - SMOOTH_FACTOR) * smoothed_w_avg1);
  smoothed_w_avg2 = (SMOOTH_FACTOR * w_avg2) + ((1.0 - SMOOTH_FACTOR) * smoothed_w_avg2);
  smoothed_w_avg3 = (SMOOTH_FACTOR * w_avg3) + ((1.0 - SMOOTH_FACTOR) * smoothed_w_avg3);
  smoothed_w_avg4 = (SMOOTH_FACTOR * w_avg4) + ((1.0 - SMOOTH_FACTOR) * smoothed_w_avg4);
  smoothed_right = (SMOOTH_FACTOR * rightLimbForce) + ((1.0 - SMOOTH_FACTOR) * smoothed_right);
  smoothed_left = (SMOOTH_FACTOR * leftLimbForce) + ((1.0 - SMOOTH_FACTOR) * smoothed_left);
  smoothed_total = (SMOOTH_FACTOR * totalForce) + ((1.0 - SMOOTH_FACTOR) * smoothed_total);

  // --- UPDATE JUMP PHASE STATE ---
  updateJumpPhase(smoothed_total);

  // -------------------------------------------------------
  // SERIAL PLOTTER — Optimized output (3 lines for clarity)
  // Total Force, Right Limb, Left Limb (all in kg)
  // -------------------------------------------------------
  int phaseCode = (int)jumpMetrics.phase;  // 0=STANDING, 1=LOADING, 2=FLIGHT, 3=LANDING
  Serial.print(smoothed_total); Serial.print(",");
  Serial.print(smoothed_right); Serial.print(",");
  Serial.print(smoothed_left); Serial.print(",");
  Serial.println(phaseCode);

  // --- SERIAL MONITOR — labeled output (open Serial Monitor to see values) ---
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 100) {  // Print every 100ms to Serial Monitor
    Serial.print("RU:");Serial.print(smoothed_w_avg1,3); Serial.print(" | ");
    Serial.print("RD:");Serial.print(smoothed_w_avg2,3); Serial.print(" | ");
    Serial.print("LU:");Serial.print(smoothed_w_avg3,3); Serial.print(" | ");
    Serial.print("LD:");Serial.print(smoothed_w_avg4,3); Serial.print(" | ");
    Serial.print("Total:");Serial.print(smoothed_total,3); Serial.print(" | ");
    
    // Print phase and metrics
    switch (jumpMetrics.phase) {
      case STANDING: Serial.print("PHASE:STANDING"); break;
      case LOADING:  Serial.print("PHASE:LOADING"); break;
      case FLIGHT:   Serial.print("PHASE:FLIGHT"); break;
      case LANDING:  Serial.print("PHASE:LANDING"); break;
    }
    
    // Print jump metrics when jump is detected
    if (jumpMetrics.jumpDetected) {
      Serial.print(" | JUMP! Contact:");Serial.print(jumpMetrics.contactTime,0);
      Serial.print("ms Flight:");Serial.print(jumpMetrics.flightTime,0);
      Serial.print("ms Peak:");Serial.print(jumpMetrics.peakForce,2);Serial.print("kg");
    }
    Serial.println();
    lastPrint = millis();
  }

  delay(5);   // Balance responsiveness & clarity
}