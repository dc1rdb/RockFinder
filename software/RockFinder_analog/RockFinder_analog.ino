/*

Simple detector for gamma radioactive substances,
utilizing a Mini SiPM Driver (SiD) Board
https://github.com/OpenGammaProject/Mini-SiD
and a Wemos D1 Mini ESP8266. This sketch
is basically a voltage controlled oscillator (VCO)
from analog input A0 and outputs an audio frequency
tone to a passive buzzer.

V2.0 - November 19, 2023
NEW: adjustable threshold via rotary encoder

Dependencies:
RotaryEncoder by Matthias Hertel

*/

#include <ESP8266WiFi.h>
#include <RotaryEncoder.h>

const uint16_t MIN = 100;       //minimum output tone frequency (Hz)
const uint16_t MAX = 5000;      //maximum output tone frequency (Hz)
const uint16_t INI = 70;        //initial threshold value

#define buzzerPin D1            //passive buzzer between this pin and GND
#define PIN_IN1 D5              //rotary encoder DT
#define PIN_IN2 D6              //rotary encoder CLK

#define ROTARYSTEPS 5           //step width for threshold adjustment
#define ROTARYMIN 5             //minimum threshold value
#define ROTARYMAX 120           //maximum threshold value

RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);

void setup() {
//  Serial.begin(115200);         //debug only
  encoder.setPosition(INI / ROTARYSTEPS);
  pinMode(buzzerPin, OUTPUT);
  WiFi.mode(WIFI_OFF);          //turn off WIFI to reduce ESP8266 power consumption
  delay(1000);
}

void loop() {
//read current tone threshold setting from rotary encoder
  encoder.tick();

  uint16_t THR = encoder.getPosition() * ROTARYSTEPS;

  if (THR < ROTARYMIN) {
    encoder.setPosition(ROTARYMIN / ROTARYSTEPS);
    THR = ROTARYMIN;
  } 
  
  else if (THR > ROTARYMAX) {
    encoder.setPosition(ROTARYMAX / ROTARYSTEPS);
    THR = ROTARYMAX;
  } 

//  Serial.print(THR);      //debug only
//  Serial.println();       //debug only

//output tone to buzzer
  uint16_t frequencyinput = analogRead(A0);
  uint16_t outputfrequency = map(frequencyinput, THR, 1023, MIN, MAX);
  tone(buzzerPin, outputfrequency);
  delay(10);
}
