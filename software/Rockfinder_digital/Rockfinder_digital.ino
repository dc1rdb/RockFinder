/*

Simple detector for gamma radioactive substances,
utilizing a Mini SiPM Driver (SiD) Board
https://github.com/OpenGammaProject/Mini-SiD
and a Wemos D1 Mini ESP8266.
Digital version, no analog integration circuit required
Converts Mini-SiD TTL output pulses to continuous moving tone

V1.0 - June 26, 2026
- initial working version, still contains serial output for optimizing.
  Parameters tailored to a 40mm dia x 40mm NaI scintillator

*/

#include <ESP8266WiFi.h>
#include <Arduino.h>

// ============================================================================
// 1. HARDWARE & CONSTANT SETUP
// ============================================================================
// Pin Configuration
const int PULSE_PIN = 13;      // GPIO13 (D7 on Wemos D1 mini)
const int BUZZER_PIN = 5;       // GPIO5  (D1 on Wemos D1 mini)

// Rotary Encoder Pins
const int encoderCLK = 14;     // GPIO14 (D5 on Wemos D1 mini)
const int encoderDT = 12;      // GPIO12 (D6 on Wemos D1 mini)

// Timing Rules
const unsigned long UPDATE_INTERVAL = 20; // Sample every 100ms
unsigned long lastUpdateTime = 0;

// ============================================================================
// 2. SIGNAL FILTER & CONTROL CONFIGURATION
// ============================================================================
// Running Median (Filters statistical scintillation noise)
const int MEDIAN_WINDOW = 5;       
float cpsSamples[MEDIAN_WINDOW] = {0.0};
int sampleIndex = 0;

// Buzzer Frequency Boundaries (Hz)
const int MIN_FREQ = 30;                 
const int MAX_FREQ = 5000;

// Audio Glide Engine
const float PITCH_GLIDE = 0.005; // Seamless micro-step smoothing factor
float targetPitch = MIN_FREQ;       
float activePitch = MIN_FREQ;

// Dynamic Sensitivity Tuning (Adjusts the upper CPS limit for tone mapping)
const int MIN_CPS = 50;             // Background CPS
volatile int maxCPSCeiling = 2000;  // Default Sensitivity (Mapping: MIN_CPS to 2000 CPS)
const int MIN_CEILING = 1000;        // Highly sensitive / narrow range
const int MAX_CEILING = 15000;      // Less sensitive / wide range
const int ENCODER_STEP = 1000;       // Adjustment increment step

// Boundary Alert Timing Variables
volatile unsigned long alertEndTime = 0; // Tracks non-blocking alert duration
volatile bool isAlertActive = false;     // Audio engine status flag

// Interrupt Storage
volatile unsigned long pulseCount = 0;
volatile int lastClkState;

// ============================================================================
// 3. INTERRUPT SERVICE ROUTINES (ISRs)
// ============================================================================
// Pulse counter ISR
void ICACHE_RAM_ATTR countPulse() {
  pulseCount++;
}


// Rotary Encoder ISR - Triggered on RISING edge only
void ICACHE_RAM_ATTR readEncoder() {
  int originalCeiling = maxCPSCeiling;

  // Determine direction based on DT state
  if (digitalRead(encoderDT) == LOW) {
    maxCPSCeiling -= ENCODER_STEP; // Lowering the ceiling INCREASES audible sensitivity
  } else {
    maxCPSCeiling += ENCODER_STEP; // Raising the ceiling DECREASES audible sensitivity
  }
  
  // Check bounds and trigger confirmation if user attempts to go past limits
  if (maxCPSCeiling <= MIN_CEILING) {
    maxCPSCeiling = MIN_CEILING;
    if (originalCeiling == MIN_CEILING) { // Trigger only if already at limit
      isAlertActive = true;
      alertEndTime = millis() + 40;       // Beep for 40ms
    }
  }
  else if (maxCPSCeiling >= MAX_CEILING) {
    maxCPSCeiling = MAX_CEILING;
    if (originalCeiling == MAX_CEILING) { // Trigger only if already at limit
      isAlertActive = true;
      alertEndTime = millis() + 40;       // Beep for 40ms
    }
  }
}

// ============================================================================
// 4. HELPER FUNCTIONS
// ============================================================================
float getMedian(float newVal) {
  cpsSamples[sampleIndex] = newVal;
  sampleIndex = (sampleIndex + 1) % MEDIAN_WINDOW;

  float sortedSamples[MEDIAN_WINDOW];
  for (int i = 0; i < MEDIAN_WINDOW; i++) {
    sortedSamples[i] = cpsSamples[i];
  }

  // Fast Insertion Sort for small windows (N <= 10)
  for (int i = 1; i < MEDIAN_WINDOW; i++) {
    float key = sortedSamples[i];
    int j = i - 1;
    while (j >= 0 && sortedSamples[j] > key) {
      sortedSamples[j + 1] = sortedSamples[j];
      j--;
    }
    sortedSamples[j + 1] = key;
  }
  
  return sortedSamples[MEDIAN_WINDOW / 2];
}

// ============================================================================
// 5. SYSTEM INITIALIZATION
// ============================================================================
void setup() {
  Serial.begin(115200);
  
  // Power reduction configuration
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();   
  
  pinMode(PULSE_PIN, INPUT); 
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Setup Rotary Encoder pins with internal pullups
  pinMode(encoderCLK, INPUT_PULLUP);
  pinMode(encoderDT, INPUT_PULLUP);
  
  // Read initial state of encoder
  lastClkState = digitalRead(encoderCLK);
  
  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), countPulse, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderCLK), readEncoder, RISING);
}

// ============================================================================
// 6. MAIN EXECUTION LOOP
// ============================================================================
void loop() {
  unsigned long currentTime = millis();
  
  // --------------------------------------------------------------------------
  // BLOCK A: DATA SAMPLING (Executes once every update interval)
  // --------------------------------------------------------------------------
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    
    // Atomically snapshot and reset interrupt counter
    noInterrupts();
    unsigned long pulsesSampled = pulseCount;
    pulseCount = 0;
    int localCeiling = maxCPSCeiling; // Safely copy volatile parameter
    interrupts();
    
    // Calculate instantaneous CPS
    float instantCPS = (pulsesSampled * 1000.0) / UPDATE_INTERVAL;
    
    // Process signal filtering
    float filteredCPS = getMedian(instantCPS);
    
    // Dynamic frequency mapping using the custom encoder ceiling
    targetPitch = map(filteredCPS, MIN_CPS, localCeiling, MIN_FREQ, MAX_FREQ);
    
    // Hard boundary safety rails
    if (targetPitch < MIN_FREQ) targetPitch = MIN_FREQ;
    if (targetPitch > MAX_FREQ) targetPitch = MAX_FREQ;

    // Telemetry output showing the live ceiling adjustments
    Serial.print("Raw CPS: "); 
    Serial.print(instantCPS, 1);
    Serial.print(" | Median CPS: "); 
    Serial.print(filteredCPS, 1);
    Serial.print(" | Tone Range Top: ");
    Serial.print(localCeiling);
    Serial.println(" CPS");

    lastUpdateTime = currentTime;
  }

  // --------------------------------------------------------------------------
  // BLOCK B: CONTINUOUS AUDIO ENGINE (Executes every loop cycle)
  // --------------------------------------------------------------------------
  if (isAlertActive) {
    if (currentTime < alertEndTime) {
      // Force a distinct high-pitch chirp while the alert window is open
      tone(BUZZER_PIN, 2500); 
    } else {
      // Timer expired: Clear the alert flag to resume normal tracking glide
      isAlertActive = false; 
    }
  } else {
    // Normal Glide Operation
    activePitch = activePitch + ((targetPitch - activePitch) * PITCH_GLIDE);
    tone(BUZZER_PIN, (int)activePitch);
  }
}
