# RockFinder
Scintillator based detector for prospecting radioactive minerals.

This project is an add-on for Nuclear Phoenix' Mini SiPM Driver (SiD) Board https://github.com/OpenGammaProject/Mini-SiD
Please refer to that page for setting up the scintillator and SiPM.

Compact design: Total size 125 x 100 x 55 mm. Weight 275 g.

Low-voltage device: No HV needed like with a photomultiplier tube.

Low power consumption: <50 mA.


When looking for radioactive rocks in the wild, I always found it annoying to keep my eyes on the display of whatever gamma detector I was using. Concentrating on small changes in click rate is also less than ideal.
On the other hand, the human brain / ear can easily and subconciously detect very small variations in tone pitch.
Via a few additional passive components, the Trigger/TTL output of the Mini SiD is hooked up to the analog input of a ESP 8266 programmed as a voltage controlled oscillator. 

A passive buzzer connected to a GPIO pin of the ESP8266 outputs a tone signal with a frequency that is proportional to the counts being detected by the SiPM.

![RockFinderInterface](https://github.com/dc1rdb/RockFinder/assets/9314230/71a8b4e7-332c-48c4-94ef-ce229b19496c)


The ESP8266 is a Lolin D1 Mini Pro with included battery connector and charging circuit. A small LiPo battery sits between the Mini SiD and the veroboard and the Mini SiD is easily being powered via the 3V3 pin of the ESP. The LiPo battery is being charged via the USB interface of the ESP8266. I've added a magnetic USB plug for better handling. If an internal battery is not required, any ESP8266 clone be used.

![IMG_20231112_143026](https://github.com/dc1rdb/RockFinder/assets/9314230/6422b9cf-9892-42e8-9e93-dd494c099827)

The enclosure is a standard DN40 (or DN50) tube with end caps, available at home improvement stores. The handle is 3D printed from:
Ludlum 2401 Handle by S0N1C https://www.thingiverse.com/thing:2561532

![IMG_20231112_142928](https://github.com/dc1rdb/RockFinder/assets/9314230/362bfb38-0d7f-455c-a06b-735b87b1088a)

Arduino code in https://github.com/dc1rdb/RockFinder/tree/main/software/RockFinder_analog

Note: there is a digital variant in https://github.com/dc1rdb/RockFinder/tree/main/software/Rockfinder_digital which does not require the above mentioned passive components but it is not working well yet. Feel free to improve it but don't complain :)
