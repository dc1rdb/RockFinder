/*

Simple detector for gamma radioactive substances,
utilizing a Mini SiPM Driver (SiD) Board
https://github.com
and a Wemos D1 Mini ESP8266.
Digital version, no analog integration circuit reqired
Converts Mini-SiD TTL output pulses to continuous moving tone

V1.2 - Adaptive exponential moving average (EMA) filter
     - Optional HC-05 BT module to transmit CPS data to CurieFinder
       (https://github.com/ATonda/CurieFinder_app)

*/

#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

// ============================================================================
// 1. HARDWARE & CONSTANT SETUP
// ============================================================================
const int PULSE_PIN = 13;      // GPIO13 (D7 on Wemos D1 mini)
const int BUZZER_PIN = 5;       // GPIO5  (D1 on Wemos D1 mini)

// Rotary Encoder Pins
const int encoderCLK = 14;     // GPIO14 (D5 on Wemos D1 mini)
const int encoderDT = 12;      // GPIO12 (D6 on Wemos D1 mini)

// Bluetooth SoftwareSerial Pins
const int BT_RX_PIN = 4;       // GPIO4  (D2 on Wemos D1 mini) -> Connect to HC-05 TX
const int BT_TX_PIN = 2;       // GPIO2  (D4 on Wemos D1 mini) -> Connect to HC-05 RX

// Timing Rules
const unsigned long UPDATE_INTERVAL = 50; // Sample every x ms
unsigned long lastUpdateTime = 0;

// Bluetooth Timing Configuration
const unsigned long BT_INTERVAL = 200;    // Send data every 200 ms
unsigned long lastBTTime = 0;             // Tracks last Bluetooth transmission

// Global storage for the latest averaged value to share across loops
float globalFilteredCPS = 0.0;

SoftwareSerial BTSerial(BT_RX_PIN, BT_TX_PIN);

// ============================================================================
// 2. SIGNAL FILTER & CONTROL CONFIGURATION
// ============================================================================
// Adaptive EMA Configuration
float emaFilteredCPS = 0.0;        // Keeps track of the current filter state
const float ALPHA_MIN = 0.08;      // Heavy smoothing coefficient for stable baselines
const float ALPHA_MAX = 0.85;      // Ultra-fast response coefficient for rapid spikes
const float SENSITIVITY_SCALE = 0.02; // Controls how aggressively alpha reacts to deviation

// Buzzer Frequency Boundaries (Hz)
const int MIN_FREQ = 30;                 
const int MAX_FREQ = 5000;

// Audio Glide Engine Constants
const float PITCH_GLIDE = 0.005; 
float targetPitch = MIN_FREQ;       
float activePitch = MIN_FREQ;

// Dynamic Sensitivity Tuning
const int MIN_CPS = 50;             //background cps, higher value for larger scintillator
volatile int maxCPSCeiling = 1000;  //highest sensitivity at power-up
const int MIN_CEILING = 1000;        
const int MAX_CEILING = 15000;      
const int ENCODER_STEP = 1000;       

// Interrupt Storage
volatile unsigned long pulseCount = 0;
volatile int lastClkState;

// Software Debounce Settings for Encoder
const unsigned long DEBOUNCE_US = 4000; 
volatile unsigned long lastEncoderTriggerTime = 0;

// ============================================================================
// 3. INTERRUPT SERVICE ROUTINES (ISRs)
// ============================================================================
void ICACHE_RAM_ATTR countPulse() {
  pulseCount++;
}

void ICACHE_RAM_ATTR readEncoder() {
  unsigned long currentTimeUs = micros();
  if (currentTimeUs - lastEncoderTriggerTime < DEBOUNCE_US) return;
  
  int currentClkState = digitalRead(encoderCLK);
  if (currentClkState != lastClkState) {
    lastClkState = currentClkState;
    if (currentClkState == HIGH) {
      if (digitalRead(encoderDT) == LOW) {
        maxCPSCeiling -= ENCODER_STEP;
      } else {
        maxCPSCeiling += ENCODER_STEP;
      }
      if (maxCPSCeiling < MIN_CEILING) maxCPSCeiling = MIN_CEILING;
      else if (maxCPSCeiling > MAX_CEILING) maxCPSCeiling = MAX_CEILING;
      lastEncoderTriggerTime = currentTimeUs;
    }
  }
}

// ============================================================================
// 4. HELPER FUNCTIONS
// ============================================================================
// Adaptive Exponential Moving Average Filter (Reverted)
float getAdaptiveEMA(float newVal) {
  // Calculate how far the new reading is from our current filtered average
  float diff = abs(newVal - emaFilteredCPS);
  
  // Scale the alpha dynamically based on the difference size
  float alpha = ALPHA_MIN + (diff * SENSITIVITY_SCALE);
  
  // Constrain alpha between defined lower and upper boundary limits
  if (alpha > ALPHA_MAX) alpha = ALPHA_MAX;
  if (alpha < ALPHA_MIN) alpha = ALPHA_MIN;
  
  // Apply the standard EMA formula: EMA = (New * Alpha) + (Old * (1 - Alpha))
  emaFilteredCPS = (newVal * alpha) + (emaFilteredCPS * (1.0 - alpha));
  
  return emaFilteredCPS;
}

// ============================================================================
// 5. SYSTEM INITIALIZATION
// ============================================================================
void setup() {
  Serial.begin(115200);
  BTSerial.begin(9600);
  
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();   
  
  pinMode(PULSE_PIN, INPUT); 
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(encoderCLK, INPUT_PULLUP);
  pinMode(encoderDT, INPUT_PULLUP);
  
  lastClkState = digitalRead(encoderCLK);
  
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
    
    noInterrupts();
    unsigned long pulsesSampled = pulseCount;
    pulseCount = 0;
    int localCeiling = maxCPSCeiling; 
    interrupts();
    
    float instantCPS = (pulsesSampled * 1000.0) / UPDATE_INTERVAL;
    
    // Process signal filtering via Standard Adaptive EMA
    globalFilteredCPS = getAdaptiveEMA(instantCPS);
    
    targetPitch = map(globalFilteredCPS, MIN_CPS, localCeiling, MIN_FREQ, MAX_FREQ);
    
    if (targetPitch < MIN_FREQ) targetPitch = MIN_FREQ;
    if (targetPitch > MAX_FREQ) targetPitch = MAX_FREQ;

    Serial.print("Raw CPS: "); 
    Serial.print(instantCPS, 1);
    Serial.print(" | Adaptive EMA CPS: "); 
    Serial.print(globalFilteredCPS, 1);
    Serial.print(" | Tone Range Top: ");
    Serial.print(localCeiling);
    Serial.println(" CPS");

    lastUpdateTime = currentTime;
  }

  // --------------------------------------------------------------------------
  // BLOCK B: BLUETOOTH DATA TRANSMISSION (Executes every 200ms)
  // --------------------------------------------------------------------------
  if (currentTime - lastBTTime >= BT_INTERVAL) {
    BTSerial.print("CPS=");
    BTSerial.println(globalFilteredCPS, 1); 
    lastBTTime = currentTime;
  }

  // --------------------------------------------------------------------------
  // BLOCK C: CONTINUOUS AUDIO ENGINE
  // --------------------------------------------------------------------------
  activePitch = activePitch + ((targetPitch - activePitch) * PITCH_GLIDE);
  tone(BUZZER_PIN, (int)activePitch);
}
