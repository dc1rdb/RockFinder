/*

Simple detector for gamma radioactive substances,
utilizing a Mini SiPM Driver (SiD) Board
https://github.com/OpenGammaProject/Mini-SiD
and a Wemos D1 Mini ESP8266. This sketch
is basically a voltage controlled oscillator (VCO)
from analog input A0 and outputs an audio frequency
tone to a passive buzzer.

V3.0 - July 30, 2024
- Rotary encoder readout is now an interrupt service routine
- real variable gain rather than just threshold adjustment

Dependencies:
RotaryEncoder by Matthias Hertel

*/

#include <ESP8266WiFi.h>
#include <RotaryEncoder.h>

#define MIN 50                  //minimum output tone frequency (Hz)
#define MAX 5000                //maximum output tone frequency (Hz)
#define buzzerPin D1            //passive buzzer between this pin and GND

#define PIN_IN1 D6              //rotary encoder DT
#define PIN_IN2 D5              //rotary encoder CLK

#define LATCHMODE FOUR3         //LatchMode FOUR3 or TWO03, depending on encoder type
#define ROTARYSTEPS 1           //step width for gain adjustment
#define ROTARYMIN 1             //minimum gain value
#define ROTARYMAX 10            //maximum gain value
#define INI 10                  //initial gain value (or fixed gain if no encoder is connected)
#define THR 100                 //threshold to suppress SIPM noise

RotaryEncoder *encoder = nullptr;

IRAM_ATTR void checkPosition()
{
  encoder->tick();              // just call tick() to check the state.
}

void setup() {
  // Serial.begin(115200);         //debug only
  WiFi.mode(WIFI_OFF);          //turn off WIFI to reduce ESP8266 power consumption
  delay(1000);
  
  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::LATCHMODE);
  encoder->setPosition(INI / ROTARYSTEPS);
  
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);

  pinMode(buzzerPin, OUTPUT);
  
}

void loop() {
  
  encoder->tick();              // just call tick() to check the state.

   int GAIN = encoder->getPosition() * ROTARYSTEPS;

  if (GAIN < ROTARYMIN) {
    encoder->setPosition(ROTARYMIN / ROTARYSTEPS);
    GAIN = ROTARYMIN;
  } 
  
  else if (GAIN > ROTARYMAX) {
    encoder->setPosition(ROTARYMAX / ROTARYSTEPS);
    GAIN = ROTARYMAX;
  } 

    // Serial.print(GAIN);        //debug only
    // Serial.println();          //debug only

  //output tone to buzzer
  uint16_t frequencyinput = analogRead(A0) * GAIN;
  uint16_t outputfrequency = map(frequencyinput, (THR * GAIN / 2), 10230, MIN, MAX);
  tone(buzzerPin, outputfrequency);
  
  delay(10);
}
