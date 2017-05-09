# open-source wireless graphical display - arduino code compatible

**Discontinued**

***

A general purpose wireless graphical LCD display unit. Powered by an Atmega328 with an Arduino bootloader.

The emonGLCD is designed to drive a 128x64 GLCD display based on the ST7565 driver. The board has a built in HopeRF RFM12B wireless module to receive data from the emonTx energy monitoring node or an emonBase (Nanode-RF web-connected basestation) which in turn could be pulled aggregated data from a web-sever to be displayed.

The emonGLCD has an on-board temperate sensor, light sensor and two tri-color ambient indicator LED's.

## Features

* ATmega328 with Arduino Uno bootloader
* 128x64 ST7565 LCD display
* White PWM LED backlight
* On-board HopeRF RFM12B wireless transceiver module
* Power supply: 5VDC via mini-USB conector
* DS18B20 on-board temperature sensor
* On-board LDR light-level sensor
* Based on JeeLabs.org design. Uses Adafruit/JeeLabs GLCD library
* Software RTC updated from the internet via emonbase.


## Build Guide

### [Pictorial Build Guide](buildguide/readme.md)

## License

The emonGLCD is open hardware with the designs released under the Creative Commons Attribution-ShareAlike 3.0 Unported License. The schematics and board files can be viewed and downloaded from the openenergymonitor github hardware repository here:
