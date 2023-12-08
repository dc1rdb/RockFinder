# RockFinder
Scintillator based detector for prospecting radioactive minerals.
![IMG_20231112_142928](https://github.com/dc1rdb/RockFinder/blob/main/docs/IMG_20231112_142928.jpg)

## Background
When searching for radioactive rocks in the wild, I always found it annoying to keep my eyes on the display of whatever gamma detector I was using. Concentrating on small changes in click rate is also less than ideal.
On the other hand, the human brain / ear can easily and subconciously detect very small variations in tone pitch. For example, this principle is widely used in satellite dish alignment tools.

## Project Overview
This project is an add-on for **Nuclear Phoenix' Mini SiPM Driver (SiD) Board:** https://github.com/OpenGammaProject/Mini-SiD

Please refer to his page for setting up the scintillator and SiPM.

This add-on converts the TTL output pulses from the Mini SiD to a tone signal with variable frequency. The tone frequency increases with the count rate detected by the SiPM. As a result, one can quickly pinpoint the radioactive source / hotspot.

## Specs

* Compact design: Total size 125 x 100 x 55 mm. Weight 275 g
* Low-voltage device: No HV needed like with a photomultiplier tube
* Low power consumption: <50 mA
* Adjustable threshold / sensitivity to cope with different levels of background radiation (optional)

I am currently using a 1" dia x 1" CsI(Tl) encapsulated scintillator from OST Photonics, coupled to a MICROFC-60035-SMT-TR SiPM. The side wall of the scintillator is covered with lead foil to improve directivity.

## Description

Via a few additional passive components, the Trigger/TTL output of the Mini SiD is connected to the analog input of a ESP 8266 programmed as a voltage controlled oscillator. 

![RockFinderInterface](https://github.com/dc1rdb/RockFinder/blob/main/docs/RockFinderInterface.png)

A passive buzzer connected to a GPIO pin of the ESP8266 outputs a tone signal.

Optionally, sensitivity can be adjusted with a rotary encoder.

The ESP8266 is a Lolin D1 Mini Pro with included battery connector and charging circuit. A small single cell 350 mAh LiPo battery sits between the Mini SiD and the veroboard and the Mini SiD is easily being powered via the 3V3 pin of the ESP. This allows for around 6 hours of operation. The LiPo battery is being charged via the USB interface of the ESP8266. A magnetic USB plug is added for better handling. If an internal battery is not required, any ESP8266 clone can be used.

![IMG_20231112_143026](https://github.com/dc1rdb/RockFinder/blob/main/docs/IMG_20231112_143026.jpg)

## Hardware
The enclosure is a standard DN40 (or DN50) tube with end caps, available at home improvement stores. The handle is 3D printed from:
Ludlum 2401 Handle by S0N1C https://www.thingiverse.com/thing:2561532

An endcap to house the SiPM on the window side of the scintillator can be 3D printed with TPU from this file: https://github.com/dc1rdb/RockFinder/blob/main/hardware/SiPM_endcap1inch.stl

![SiPM_endcap](https://github.com/dc1rdb/RockFinder/blob/main/docs/SiPM_endcap.jpg)

## Code
Arduino code in https://github.com/dc1rdb/RockFinder/tree/main/software/RockFinder_analog

## Demo
Video of the prototype in action: https://github.com/dc1rdb/RockFinder/blob/main/docs/RockfinderDemo.mp4

## Updates and Project Log
Update Dec 8th, 2023

I`ve just finished a unit that incorporates a rotary encoder for threshold adjustment and utilizes a 40 mm dia x 40 mm NaI(Tl) scintillator coupled to a 2x2 SiPM array (MICROFC-60035-SMT-TR). With around four times the scintillator volume compared to my first unit, the sensitivity is significantly improved. A few pics of the new unit can be found in https://github.com/dc1rdb/RockFinder/tree/main/docs

Thanks for reading!
