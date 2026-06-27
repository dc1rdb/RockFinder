/*

Simple detector for gamma radioactive substances,
utilizing a Mini SiPM Driver (SiD) Board
https://github.com
and a Wemos D1 Mini ESP8266.
Digital version, no analog integration circuit reqired
Converts Mini-SiD TTL output pulses to continuous moving tone

V1.1 - Debounced Encoder Version
- Tailored to a 40mm dia x 40mm NaI scintillator
- Fixed encoder jumping via an ISR microsecond time-lock

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
const unsigned long UPDATE_INTERVAL = 20; // Sample every 20ms
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

// Interrupt Storage
volatile unsigned long pulseCount = 0;
volatile int lastClkState;

// Software Debounce Settings for Encoder
const unsigned long DEBOUNCE_US = 4000; // 4 milliseconds block window (in microseconds)
volatile unsigned long lastEncoderTriggerTime = 0;

// ============================================================================
// 3. INTERRUPT SERVICE ROUTINES (ISRs)
// ============================================================================
// Pulse counter ISR
void ICACHE_RAM_ATTR countPulse() {
  pulseCount++;
}

// Rotary Encoder ISR - Triggered on CHANGE edge for absolute state verification
void ICACHE_RAM_ATTR readEncoder() {
  unsigned long currentTimeUs = micros();
  
  // Software Debounce: If the change occurs within the window, ignore it as noise
  if (currentTimeUs - lastEncoderTriggerTime < DEBOUNCE_US) {
    return;
  }
  
  int currentClkState = digitalRead(encoderCLK);
  
  // Only process on a clean transition edge to prevent double execution
  if (currentClkState != lastClkState) {
    lastClkState = currentClkState;
    
    // We sample when CLK goes HIGH (Rising edge logic)
    if (currentClkState == HIGH) {
      // Determine direction based on DT state
      if (digitalRead(encoderDT) == LOW) {
        maxCPSCeiling -= ENCODER_STEP; // Lowering the ceiling INCREASES audible sensitivity
      } else {
        maxCPSCeiling += ENCODER_STEP; // Raising the ceiling DECREASES audible sensitivity
      }
      
      // Enforce boundary rails
      if (maxCPSCeiling < MIN_CEILING) {
        maxCPSCeiling = MIN_CEILING;
      }
      else if (maxCPSCeiling > MAX_CEILING) {
        maxCPSCeiling = MAX_CEILING;
      }
      
      // Update the timestamp only on an accepted change event
      lastEncoderTriggerTime = currentTimeUs;
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
  
  // Attach interrupts (Changed encoder to CHANGE edge for superior tracking)
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), countPulse, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderCLK), readEncoder, CHANGE);
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
  // Normal Glide Operation
  activePitch = activePitch + ((targetPitch - activePitch) * PITCH_GLIDE);
  tone(BUZZER_PIN, (int)activePitch);
}
