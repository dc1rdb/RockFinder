# RockFinder
Scintillator based detector for radioactive minerals

This is an add-on for Nuclear Phoenix' Mini SiPM Driver (SiD) Board https://github.com/OpenGammaProject/Mini-SiD

When looking for radioactive rocks in the wild, I always found it annoying to keep my eyes on the display of whatever gamma detector I was using. Concentrating on small changes in click rate is also less than ideal.
On the other hand, the human brain / ear can easily and subconciously detect very small variations in tone pitch.
With a few additional passive components, the Trigger/TTL output of the Mini SiD is hooked up to an ESP 8266 programmed as a voltage controlled oscillator.

![RockFinderInterface](https://github.com/dc1rdb/RockFinder/assets/9314230/71a8b4e7-332c-48c4-94ef-ce229b19496c)


The ESP8266 is a Lolin D1 mini pro with included battery connector and charging circuit. A small LiPo battery sits between the Mini SiD and the veroboard and the Mini SiD is easily being powered thru the ESP. Battery charging via USB

![IMG_20231112_143026](https://github.com/dc1rdb/RockFinder/assets/9314230/6422b9cf-9892-42e8-9e93-dd494c099827)

The enclosure is a standard DN40 tube, available at home improvement stores. The handle is 3D printed from:
Ludlum 2401 Handle by S0N1C https://www.thingiverse.com/thing:2561532

![IMG_20231112_142928](https://github.com/dc1rdb/RockFinder/assets/9314230/362bfb38-0d7f-455c-a06b-735b87b1088a)
