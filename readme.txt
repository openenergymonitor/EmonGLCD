open-source wireless graphical display - arduino code compatialbe 
******************************************************************

emonGLCD Documentation: http://openenergymonitor.org/emon/emonglcd
Schematic and PCB board design: http://solderpad.com/openenergymon/emonglcd/

Required Arduino Libraries
--------------------------------------------------------------------------------------------
Builds on JeeLabs hardware and uses JeeLabs libraries: 
Download the JeeLabs Ports and RF12 library here (insert into Arduino librarys folder):
http://jeelabs.net/projects/cafe/wiki/Ports
http://jeelabs.net/projects/cafe/wiki/RF12
http://jeelabs.net/projects/cafe/wiki/GLCDlib
http://jeelabs.net/projects/cafe/wiki/RTClib 	-needed for Solar PV example

On-board DS18B20 digital temperature sensor needs:
OneWire library: http://www.pjrc.com/teensy/td_libs_OneWire.html
Dallas Temperature library: milesburton.com/Dallas_Temperature_Control_Library
---------------------------------------------------------------------------------------------

Note: for improved display contrast edit line 134 of GLCD_ST7565.cpp in GLCDlib to be: st7565_Set_Brightness(0x18);


emonGLCD sketch examples (currently use Arduino 23):

example 02: Standard single CT energy monitor display with horizontal bar

solarPV type 1 and 2: Solar PV monitoring display, both consumption and generation  

