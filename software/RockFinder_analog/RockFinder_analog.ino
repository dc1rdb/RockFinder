
/*

Simple detector for gamma radioactive substances,
utilizing a Mini SiPM Driver (SiD) Board
https://github.com/OpenGammaProject/Mini-SiD
and a Wemos D1 Mini ESP8266. This sketch
is basically a voltage controlled oscillator (VCO)
from analog input A0 and outputs an audio frequency
tone to a passive buzzer.

V1.1 - November 18, 2023

Possible future improvements:
- threshold adjustment via rotary encoder
- automatic threshold depending on background radiation

*/

#include <ESP8266WiFi.h>

const uint8_t buzzerPin = 5;    //Wemos Mini pin D1 connects to passive buzzer
const uint16_t THR = 70;        //threshold value to suppress background SiPM noise
const uint16_t MIN = 100;       //minimum output tone frequency (Hz)
const uint16_t MAX = 5000;      //maximum output tone frequency (Hz)

void setup() {
  pinMode(buzzerPin, OUTPUT);
  WiFi.mode(WIFI_OFF);          //turn off WIFI to reduce ESP8266 power consumption
  delay(1000);
}

void loop() {

  uint16_t frequencyinput = analogRead(A0);
  uint16_t outputfrequency = map(frequencyinput, THR, 1023, MIN, MAX);
  tone(buzzerPin, outputfrequency);
  delay(10);
}
