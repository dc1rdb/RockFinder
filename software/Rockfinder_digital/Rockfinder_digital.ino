/*

Simple detector for gamma radioactive substances,
utilizing a Mini SiPM Driver (SiD) Board
https://github.com/OpenGammaProject/Mini-SiD
and a Wemos D1 Mini ESP8266.
Converts Mini-SiD TTL output pulses to continuous moving tone
Optional rotary encoder for sensitivity adjustment
Optional HC-05 BT module: connect RX to GPIO2  (D4 on Wemos D1 mini)
to transmit CPS data to CurieFinder https://github.com/ATonda/CurieFinder_app

V1.3 - BT module on hardware Serial1 for improved performance
     - Fixed rotary encoder bouncing & skipping steps
     - Filter optimizations

*/

#include <ESP8266WiFi.h>
#include <Arduino.h>

// ============================================================================
// 1. HARDWARE & CONSTANT SETUP
// ============================================================================
const int PULSE_PIN = 13;      // GPIO13 (D7 on Wemos D1 mini)
const int BUZZER_PIN = 5;       // GPIO5  (D1 on Wemos D1 mini)

// Rotary Encoder Pins
const int encoderCLK = 14;     // GPIO14 (D5 on Wemos D1 mini)
const int encoderDT = 12;      // GPIO12 (D6 on Wemos D1 mini)

// HC-05 BT module RX on GPIO2  (D4 on Wemos D1 mini)

// Timing Rules
const unsigned long UPDATE_INTERVAL = 50;    //how often cps value is being calculated
unsigned long lastUpdateTime = 0;

// Bluetooth Timing Configuration
const unsigned long BT_INTERVAL = 200;       //how often data is being sent via Bluetooth
unsigned long lastBTTime = 0;             

// Global storage for the latest averaged value to share across loops
float globalFilteredCPS = 0.0;

// ============================================================================
// 2. SIGNAL FILTER & CONTROL CONFIGURATION
// ============================================================================
// Adaptive EMA Configuration
float emaFilteredCPS = 0.0;        
const float ALPHA_MIN = 0.08f;          //smoothing coefficient for stable baselines
const float ALPHA_MAX = 0.90f;          //response coefficient for rapid spikes
const float SENSITIVITY_SCALE = 0.04f;  //controls how aggressively alpha reacts to deviation

// Buzzer Frequency Boundaries (Hz)
const int MIN_FREQ = 30;                 
const int MAX_FREQ = 5000;

// Audio Glide Engine Constants
const float PITCH_GLIDE = 0.01f;        //"flywheel" for tone pitch changes
float targetPitch = MIN_FREQ;           //buzzer frequency at background radiation
float activePitch = MIN_FREQ;           //max buzzer frequency 
int lastEmittedPitch = 0;            

// Dynamic Sensitivity Tuning
const int MIN_CPS = 20;              //background cps, adjust for scintillator size/performance
volatile int maxCPSCeiling = 1000;   //adjust this for desired fixed sensitivity if no rotary encoder used
const int MIN_CEILING = 1000;        
const int MAX_CEILING = 15000;      
const int ENCODER_STEP = 1000;       

// Interrupt Storage
volatile unsigned long pulseCount = 0;

// Optimized State Machine Array for Quadrature Encoding (Prevents Ghost Steps)
// Maps 4-bit indices [OldCLK, OldDT, NewCLK, NewDT] to valid step actions (-1, 0, 1)
const int8_t KNOB_STATES[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
volatile uint8_t encoderHistory = 0;

// Compile-time pre-calculation
const float CPS_MULTIPLIER = 1000.0f / (float)UPDATE_INTERVAL;

// ============================================================================
// 3. INTERRUPT SERVICE ROUTINES (ISRs)
// ============================================================================
void ICACHE_RAM_ATTR countPulse() {
  pulseCount++;
}

// State Machine Decoder running on CHANGE for BOTH pins
void ICACHE_RAM_ATTR readEncoder() {
  // Shift old pins left, read new pins into lowest 2 bits
  encoderHistory = (encoderHistory << 2) & 0x0F;
  encoderHistory |= (digitalRead(encoderCLK) << 1) | digitalRead(encoderDT);
  
  // Lookup movement based on historical sequence
  int8_t movement = KNOB_STATES[encoderHistory];
  
  // Static sub-step tracker to filter out 4 states per physical click
  static int8_t subStepCounter = 0;
  
  if (movement != 0) {
    // REVERSED CORRECTION: Flipped the '+' and '-' signs here to fix direction polarity
    if (movement > 0) {
      subStepCounter--;
    } else {
      subStepCounter++;
    }
    
    // 4-STEP FILTER: Only apply change when 4 quadrature sub-steps have completed
    if (subStepCounter >= 4) {
      subStepCounter = 0; // Reset sub-steps
      maxCPSCeiling += ENCODER_STEP;
      if (maxCPSCeiling > MAX_CEILING) maxCPSCeiling = MAX_CEILING;
    } 
    else if (subStepCounter <= -4) {
      subStepCounter = 0; // Reset sub-steps
      maxCPSCeiling -= ENCODER_STEP;
      if (maxCPSCeiling < MIN_CEILING) maxCPSCeiling = MIN_CEILING;
    }
  }
}

// ============================================================================
// 4. HELPER FUNCTIONS
// ============================================================================
float getAdaptiveEMA(float newVal) {
  float diff = abs(newVal - emaFilteredCPS);
  float alpha = ALPHA_MIN + (diff * SENSITIVITY_SCALE);
  alpha = constrain(alpha, ALPHA_MIN, ALPHA_MAX);
  emaFilteredCPS = (newVal * alpha) + (emaFilteredCPS * (1.0f - alpha));
  return emaFilteredCPS;
}

// ============================================================================
// 5. SYSTEM INITIALIZATION
// ============================================================================
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600); 
  
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();   
  
  pinMode(PULSE_PIN, INPUT); 
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(encoderCLK, INPUT_PULLUP);
  pinMode(encoderDT, INPUT_PULLUP);
  
  // Seed initial encoder state array index
  encoderHistory = (digitalRead(encoderCLK) << 1) | digitalRead(encoderDT);
  
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), countPulse, RISING);
  
  // Both pins MUST trigger the ISR on CHANGE for full quadrature decoding
  attachInterrupt(digitalPinToInterrupt(encoderCLK), readEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderDT), readEncoder, CHANGE);
}

// ============================================================================
// 6. MAIN EXECUTION LOOP
// ============================================================================
void loop() {
  unsigned long currentTime = millis();
  
  // --------------------------------------------------------------------------
  // BLOCK A: DATA SAMPLING
  // --------------------------------------------------------------------------
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = currentTime; 
    
    noInterrupts();
    unsigned long pulsesSampled = pulseCount;
    pulseCount = 0;
    int localCeiling = maxCPSCeiling; 
    interrupts();
    
    float instantCPS = pulsesSampled * CPS_MULTIPLIER;
    globalFilteredCPS = getAdaptiveEMA(instantCPS);
    
    targetPitch = map((int)globalFilteredCPS, MIN_CPS, localCeiling, MIN_FREQ, MAX_FREQ);
    targetPitch = constrain(targetPitch, MIN_FREQ, MAX_FREQ);

    Serial.print("Raw CPS: "); 
    Serial.print(instantCPS, 1);
    Serial.print(" | Adaptive EMA CPS: "); 
    Serial.print(globalFilteredCPS, 1);
    Serial.print(" | Tone Range Top: ");
    Serial.print(localCeiling);
    Serial.println(" CPS");
  }

  // --------------------------------------------------------------------------
  // BLOCK B: BLUETOOTH DATA TRANSMISSION
  // --------------------------------------------------------------------------
  if (currentTime - lastBTTime >= BT_INTERVAL) {
    lastBTTime = currentTime;
    Serial1.print("CPS=");
    Serial1.println(globalFilteredCPS, 1); 
  }

  // --------------------------------------------------------------------------
  // BLOCK C: CONTINUOUS AUDIO ENGINE 
  // --------------------------------------------------------------------------
  activePitch = activePitch + ((targetPitch - activePitch) * PITCH_GLIDE);
  int currentIntPitch = (int)activePitch;
  
  if (currentIntPitch != lastEmittedPitch) {
    tone(BUZZER_PIN, currentIntPitch);
    lastEmittedPitch = currentIntPitch;
  }
}
