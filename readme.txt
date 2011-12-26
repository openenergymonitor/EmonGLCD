*****************************************************************
open-source wireless graphical display - arduino code compatible
******************************************************************

emonGLCD Documentation: http://openenergymonitor.org/emon/emonglcd
Schematic and PCB board design: http://solderpad.com/openenergymon/emonglcd/

Required Arduino Libraries
--------------------------------------------------------------------------------------------
Builds on JeeLabs hardware and uses JeeLabs libraries: 
Download the JeeLabs Ports and RF12 library here (insert into Arduino librarys folder):
https://github.com/jcw/jeelib	-RFM12 library 
https://github.com/jcw/rtclib   -used for time keeping to reset kWh at midnight
https://github.com/jcw/glcdlib	-GLCD library 

On-board DS18B20 digital temperature sensor uses:
Dallas Temperature library: http://download.milesburton.com/Arduino/MaximTemperature/ (version 372 works with Arduino 1.0) 
OneWire library: http://www.pjrc.com/teensy/td_libs_OneWire.html

---------------------------------------------------------------------------------------------

emonGLCD sketch examples - updated to Arduino 1.0 (Dec 2011)

example 02: Standard single CT energy monitor display with horizontal bar

solarPV type 1 and 2: Solar PV monitoring display, both consumption and generation display (see solar PV documentation link above) 

