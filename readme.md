# open-source wireless graphical display - arduino code compatible

**Discontinued**

***

A general purpose wireless graphical LCD display unit. Powered by an Atmega328 with an Arduino bootloader.

The emonGLCD is designed to drive a 128x64 GLCD display based on the ST7565 driver. The board has a built in HopeRF RFM12B wireless module to receive data from emonpi or a emonTx energy monitoring node or an emonBase (Nanode-RF web-connected basestation) which in turn could be pulled aggregated data from a web-sever to be displayed.

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
* Software RTC updated from the internet via emonpi.




## Using with emonpi

The latest emonglcd firmware supports reading data that is transmitted via emonpi.  

First you need to add the following to your emonhub.conf file for each emonGlcd that you have:

```
[[20]]
    nodename = emonglcd
    firmware =V1_1
    hardware = emonglcd
    [[[rx]]]
     names = temperature,
     datacodes = h
     scales = 0.01,1
     units = c
  [[[tx]]]
     names =nodeid,hour,minute,second,utilityW,solarW,utilityKwh,solarKwh
     datacodes =b,b,b,h,h,H,H
     units = h,min,sec,W,W,kwh,kwh
```


Simply edit firmware/SolarPV.ino and ensure line 63 has the nodeId of your emonpi (defaults to 5) and upload the firmware to your emonglcd.
A compiled version exists in the 'compiled' directory. 

For updating, you can either use the supplied node-red flow ( emonglcd_updater_nodered.json ) or a companion perl script running on your emonpi.

Either method allows you to chose which emonpi node/feed values are to be used for Solar Watts/kwh & Utility Watts/Kwh.

Both methods will update the emonpi each time any of these node variables change in emonhub.
They work by subscribing to these topics via MQTT and using MQTT to make RF transmit requests to emonhub (which are then sent to each emonglcd).

The companion script (emonglcd-send.pl) has been provided in this repo that reads any emonhub node/feed values and transmits them to mulitple emonglcd(s). 
Simply place clone this repo into /home/pi, edit /home/pi/EmonGLCD/emonglcd-send.cfg with your emonGLCD nodeid(s) and the variables you wish to use for solarW, utilityW, solarKwh and utilityKwh.
A systemd run file has also been provided to run this at startup, simply place systemd/emonglcd.service into /lib/systemd/system and run 'systemctl enable emonglcd'

Note:(you may need to install  Net::MQTT::Simple and Config::Simple  from cpan ) :

```
->cpan Net::MQTT::Simple
->cpan Config::Simple
```
 

## Using without emonpi (sniff from emonTx)
To use without emonpi and have the emonglcd sniff the data from a emonTx, simply change line 64 to include your nodeId of the emonTx, and undef EMONPI in line 63:

#undef EMONPI
#define EMONTX  27

## Build Guide

### [Pictorial Build Guide](buildguide/readme.md)

## License

The emonGLCD is open hardware with the designs released under the Creative Commons Attribution-ShareAlike 3.0 Unported License. The schematics and board files can be viewed and downloaded from the openenergymonitor github hardware repository here:
