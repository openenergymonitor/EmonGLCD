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

Single CT_AutoTime_TempLog: for dispaying data from single CT emonTx, and posting on-board temperature online to emoncms with emonBase/NanodeRF. Also gets time from interent via NanodeRF. Use with Nanode RF_singleCT_RTCrelay_GLCDtemp example

solarPV_AutoTme_TempLog - for displaying data from 2CT solar PV monitoring emonTx (type 1 and 2 - see comments in sketch). Posts room temperature online to emoncms via emonBase/NanodeRF. Also gets current time for interent via NanodeRF. Use with NanodeRf_singleCT_RTcrealy_GLCDtemp example but add in commented out lines to send multiple power values online to emoncms. 

emonGLCD Tester - simple sketch to test the function of the temperature sensor, light sensor and tri-colour LED's. Example can be used with or without the LCD and without the RFM12B. If LCD is connected results will be displayed on LCD. 

