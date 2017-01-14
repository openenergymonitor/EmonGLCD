# emonGLCD V1


A general purpose wireless graphical LCD display unit. Powered by an Atmega328 with an Arduino bootloader.

The emonGLCD is designed to drive a 128x64 GLCD display based on the ST7565 driver. The board has a built in HopeRF RFM12B wireless module to receive data from the emonTx energy monitoring node or an emonBase (Nanode-RF web-connected basestation) which in turn could be pulled aggregated data from a web-sever to be displayed.

The emonGLCD has an on-board temperate sensor, light sensor and two tri-color ambient indicator LED's.

Based on JeeLabs.org design.


[http://shop.openenergymonitor.com/displays/](http://shop.openenergymonitor.com/displays/)


![photo.jpg](photo.jpg)

### Port map

|         |                   |         |
|---|---|---|
|Function|emonGLCD V1.2 (old)|emonGLCD V1.3+|
|Push button(s)|Dig15 / ADC|Dig 15 (enter), dig 16 (up) and dig 19 (down) - requires software pull-up, active low (V1.3 only). Blog post|
|Temperature sensor|Dig 5|Dig 5|
|LDR light sensor|ADC0 – see V1.2 bugs page|ADC4|
|Tri-color LED Red|Dig 9 – PWM|Dig 9 – PWM|
|Tri-colour LCD Green|Dig 8|Dig 6 – PWM|



### Schematic

![schematic.png](schematic.png)



### Board

![board.png](board.png)


## Enclosure

- [emonGLCD Fascia CAD design is up on Thingiverse](http://www.thingiverse.com/thing:22295)
- [User contributed 3D printed case](https://openenergymonitor.org/emon/node/12290)

## Errata

**The v1.4 PCB contains a track error, rendering the spare Digital IO pin on the JeePort unusable. It can be fixed by cutting a PCB trace. See  this for details.**

***

### Hardware revision history

#### V1.2:

* First release

#### V1.3:

* LDR moved to ADC4,
* LED from Dig 8 to Dig 6 to be on PWM output
* Added two extra push buttons for menu navigation (Dig 16 and Dig19) - don't work. Update: they do work!
* Connected backlight directly to IRQ PWM
* Added two additional menu selection buttons to make three in total

#### V1.4:

* Fixed switches connections
* Connected LCD to 3.3V instead of 5V
* External power connection now routed through voltage reg - accepts input from 5-12V

#### V1.5:

* Corrected orientation of C13 and C14, this fixed issue of LCD contrast fading
* 12/03/13 - Fix Dig 8 and Dig 6 short and silkscreen error on JeePort3: https://openenergymonitor.org/emon/node/2063

### Open Hardware

Hardware designs (schematics and CAD) files are licensed under the [Creative Commons Attribution-ShareAlike 3.0 Unported License](http://creativecommons.org/licenses/by-sa/3.0/) and follow the terms of the [OSHW (Open-source hardware) Statement of Principles 1.0.](http://freedomdefined.org/OSHW)
