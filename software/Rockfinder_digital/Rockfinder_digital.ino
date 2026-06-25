/*

Simple detector for gamma radioactive substances,
utilizing a Mini SiPM Driver (SiD) Board
https://github.com/OpenGammaProject/Mini-SiD
and a Wemos D1 Mini ESP8266.
Digital version, no analog integration circuit reqired
Converts Mini-SiD TTL output pulses to continuous moving tone

V0.1 - June 25, 2026
- initial working version, sensitivity still sucks, parameter tweaking required

*/

#include <ESP8266WiFi.h>

const int interruptPin = 12;   // Wemos D6 linked to Mini-SiD TTL INT output
const int buzzerPin = 5;       // Wemos D1 linked to passive buzzer

// Frequency spectrum configuration for audio gliding
const float BASE_FREQ = 50.0;   // Low pitch for baseline background counts
const float MAX_FREQ = 5000.0;   // Piercing pitch for hot uranium spots

// Inertia & responsiveness settings
const float RAD_SMOOTHING = 0.1; // Filters out single-pulse jitter (0.01 to 0.1)
const float PITCH_GLIDE = 0.15;   // Controls tone transition speed. Lower = longer slides.

volatile unsigned long rawPulses = 0;
float currentRadLevel = 0.0;
float activePitch = BASE_FREQ;

unsigned long lastFilterTick = 0;

void ICACHE_RAM_ATTR handleInterrupt();

void setup() {
  //optimize power consumption
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();   
  
  pinMode(interruptPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);
  
  delay(1);
  
  lastFilterTick = millis();
}

void handleInterrupt() {
  rawPulses++; 
}

void loop() {
  unsigned long now = millis();
  
  // Dynamic processing block executed every 10ms
  if (now - lastFilterTick >= 10) {
    lastFilterTick = now;

    // 1. Thread-safe collection of pulse count
    noInterrupts();
    unsigned long intervalPulses = rawPulses;
    rawPulses = 0;
    interrupts();

    // 2. Smooth raw Geiger spikes into a continuous mathematical curve
    // Converts pulse bursts into a sustained rolling intensity level
    currentRadLevel = (currentRadLevel * 0.8) + ((float)intervalPulses * RAD_SMOOTHING);

    // 3. Map intensity to a target pitch logarithmically
    // Natural ore gives variable spikes; exponential scaling keeps the audio clear
    float targetPitch = BASE_FREQ + (currentRadLevel * (MAX_FREQ - BASE_FREQ) / 8.0);
    targetPitch = constrain(targetPitch, BASE_FREQ, MAX_FREQ);

    // 4. Gliding mechanism: Push the active audio pitch smoothly toward target
    activePitch = activePitch + ((targetPitch - activePitch) * PITCH_GLIDE);

    // 5. Stream frequency updates continuously to the hardware timer
    tone(buzzerPin, (int)activePitch);
  }
}
