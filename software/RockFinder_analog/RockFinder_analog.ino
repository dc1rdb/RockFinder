/*

Simple detector for gamma radioactive substances,
utilizing a Mini SiPM Driver (SiD) Board
https://github.com/OpenGammaProject/Mini-SiD
and a Wemos D1 Mini ESP8266. This sketch
is basically a voltage controlled oscillator (VCO)
from analog input A0 and outputs an audio frequency
tone to a passive buzzer.

V2.2 - January 6, 2024
NEW: adjustable threshold via rotary encoder
(no code changes required if no encoder is connected)

Dependencies:
RotaryEncoder by Matthias Hertel

*/

#include <ESP8266WiFi.h>
#include <RotaryEncoder.h>

#define MIN 100                 //minimum output tone frequency (Hz)
#define MAX 5000                //maximum output tone frequency (Hz)
#define buzzerPin D1            //passive buzzer between this pin and GND

#define PIN_IN1 D5              //rotary encoder DT
#define PIN_IN2 D6              //rotary encoder CLK

#define LATCHMODE FOUR3         //LatchMode FOUR3 or TWO03, depending on encoder type
#define ROTARYSTEPS 5           //step width for threshold adjustment
#define ROTARYMIN 5             //minimum threshold value
#define ROTARYMAX 300           //maximum threshold value
#define INI 90                  //initial threshold value (or fixed threshold if no encoder is connected)

RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::LATCHMODE);

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

  //Serial.print(THR);      //debug only
  //Serial.println();       //debug only

//output tone to buzzer
  uint16_t frequencyinput = analogRead(A0);
  uint16_t outputfrequency = map(frequencyinput, THR, 1023, MIN, MAX);
  tone(buzzerPin, outputfrequency);
  delay(10);
}
