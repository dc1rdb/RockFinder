/*

Simple detector for gamma radioactive substances,
utilizing a Mini SiPM Driver (SiD) Board
https://github.com/OpenGammaProject/Mini-SiD
and a Wemos D1 Mini ESP8266. This sketch
integrates digital pulses from the (SiD) Board,
converts them to a proportional audio frequency
and outputs the tone on a passive buzzer

*/

#include <ESP8266WiFi.h>

const int interruptPin = 13;   //Wemos D7
const int buzzerPin = 5;       //Wemos D1
const int MIN = 100;           //minimum output tone frequency (Hz)
const int MAX = 5000;          //maximum output tone frequency (Hz)
volatile int sipmpulse = 1;

void ICACHE_RAM_ATTR handleInterrupt();

void setup() {
 
  Serial.begin(115200);
  pinMode(interruptPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);
  WiFi.mode( WIFI_OFF );       //turn off WIFI to reduce power consumption
  delay(1000);
 
}
 
void handleInterrupt() {
  if (sipmpulse < 512) {
        sipmpulse++;
      }
}

 
void loop() {
 
      Serial.println(sipmpulse);
      int outputfrequency = map(sipmpulse, 2, 624, MIN, MAX);
      tone(buzzerPin, outputfrequency);
      delay(20);
      if (sipmpulse > 0) {
        sipmpulse--;
      }
}
