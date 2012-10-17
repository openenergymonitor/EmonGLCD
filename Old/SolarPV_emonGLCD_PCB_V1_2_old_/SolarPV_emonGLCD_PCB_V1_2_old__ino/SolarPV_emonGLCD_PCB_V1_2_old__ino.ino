//------------------------------------------------------------------------------------------------------------------------------------------------
// emonGLCD Solar PV monitor example - compatiable with Arduino 1.0 

// emonGLCD documentation http://openEnergyMonitor.org/emon/emonglcd
// solar PV monitor build documentation: http://openenergymonitor.org/emon/applications/solarpv

// For use with emonTx setup with 2CT with CT 1 monitoring consumption/grid and CT 2 monitoring PV generation .
// The CT's should be clipped on with the orientation so grid reading is postive when importing and negative when exporting. Generation reading should always be positive. 

// Correct time is updated via NanodeRF which gets time from internet, this is used to reset Kwh/d counters at midnight. 

// Temperature recorded on the emonglcd is also sent to the NanodeRF for online graphing

// this sketch is currently setup for type 1 solar PV monitoring where CT's monitor generation and consumption separately
// The sketch assumes emonx.power1 is consuming/grid power and emontx.power2 is solarPV generation
// to use this sketch for type 2 solar PV monitoring where CT's monitor consumption and grid import/export using an AC-AC adapter to detect current flow direction 
//    -change line 220-221- see comments in on specific lines. See Solar PV documentation for explination 

// GLCD library by Jean-Claude Wippler: JeeLabs.org
// 2010-05-28 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
//
// History page by vworp https://github.com/vworp
//
// Authors: Glyn Hudson and Trystan Lea
// Part of the: openenergymonitor.org project
// Licenced under GNU GPL V3
// http://openenergymonitor.org/emon/license

// Libraries in the standard arduino libraries folder:
//
//	- OneWire library	http://www.pjrc.com/teensy/td_libs_OneWire.html
//	- DallasTemperature	http://download.milesburton.com/Arduino/MaximTemperature
//                           or https://github.com/milesburton/Arduino-Temperature-Control-Library
//	- JeeLib		https://github.com/jcw/jeelib
//	- RTClib		https://github.com/jcw/rtclib
//	- GLCD_ST7565		https://github.com/jcw/glcdlib
//
// Other files in project directory (should appear in the arduino tabs above)
//	- icons.ino
//	- templates.ino
//
//-------------------------------------------------------------------------------------------------------------------------------------------------

#include <JeeLib.h>
#include <GLCD_ST7565.h>
#include <avr/pgmspace.h>
GLCD_ST7565 glcd;

#include <OneWire.h>		    // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>      // http://download.milesburton.com/Arduino/MaximTemperature/ (3.7.2 Beta needed for Arduino 1.0)
#include <RTClib.h>                 // Real time clock (RTC) - used for software RTC to reset kWh counters at midnight
#include <Wire.h>                   // Part of Arduino libraries - needed for RTClib
RTC_Millis RTC;

//--------------------------------------------------------------------------------------------
// RFM12B Settings
//--------------------------------------------------------------------------------------------
#define MYNODE 20            // Should be unique on network, node ID 30 reserved for base station
#define freq RF12_433MHZ     // frequency - match to same frequency as RFM12B module (change to 868Mhz or 915Mhz if appropriate)
#define group 210            // network group, must be same as emonTx and emonBase

//---------------------------------------------------
// Data structures for transfering data between units
//---------------------------------------------------
typedef struct { int power1, power2, power3, Vrms; } PayloadTX;         // neat way of packaging data for RF comms
PayloadTX emontx;

typedef struct { int temperature; } PayloadGLCD;
PayloadGLCD emonglcd;

//---------------------------------------------------
// emonGLCD SETUP
//---------------------------------------------------
#define emonGLCDV1.3                // un-comment if using older V1.3 emonGLCD PCB - enables required internal pull up resistors. Not needed for V1.4 onwards 
const int greenLED=8;               // Green tri-color LED (Dig 8 on emonGLCD V1.2)
const int redLED=9;                 // Red tri-color LED
const int LDRpin=4;    		    // analog pin of onboard lightsensor 
const int switch1=15;               // Push switch digital pins (active low for V1.3, active high for V1.4)
const int switch2=16;
const int switch3=19;

const int maxgen=3000;              // peak output of soalr PV system in W - used to calculate when to change cloud icon to a sun

//---------------------------------------------------
// emonGLCD variables 
//---------------------------------------------------
int hour = 0, minute = 0;
double usekwh = 0, genkwh = 0;
double use_history[7], gen_history[7];
int cval_use, cval_gen;
byte page = 1;


//---------------------------------------------------
// Temperature Sensor Setup
//---------------------------------------------------
#define ONE_WIRE_BUS 5              // temperature sensor connection - hard wired 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
double temp,maxtemp,mintemp;


//-------------------------------------------------------------------------------------------- 
// Flow control
//-------------------------------------------------------------------------------------------- 
unsigned long last_emontx;                   // Used to count time from last emontx update
unsigned long last_emonbase;                   // Used to count time from last emontx update
boolean last_switch_state, switch_state; 
unsigned long fast_update, slow_update;

//--------------------------------------------------------------------------------------------
// Setup
//--------------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(9600);
  rf12_initialize(MYNODE, freq,group);
  glcd.begin(0x18);
  glcd.backLight(200);
  
  sensors.begin();                         // start up the DS18B20 temp sensor onboard  
  sensors.requestTemperatures();
  temp = (sensors.getTempCByIndex(0));     // get inital temperture reading
  mintemp = temp; maxtemp = temp;          // reset min and max

  pinMode(greenLED, OUTPUT); 
  pinMode(redLED, OUTPUT); 
  
  #ifdef emonGLCDV1.3                      //enable internal pull up resistors for push switches on emonGLCD V1.3 (old) 
  pinMode(switch1, INPUT); pinMode(switch2, INPUT); pinMode(switch2, INPUT);
  digitalWrite(switch1, HIGH); digitalWrite(switch2, HIGH); digitalWrite(switch3, HIGH); 
  #endif
}

//--------------------------------------------------------------------------------------------
// Loop
//--------------------------------------------------------------------------------------------
void loop()
{
  
  if (rf12_recvDone())
  {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // and no rf errors
    {
      int node_id = (rf12_hdr & 0x1F);
      if (node_id == 10) {emontx = *(PayloadTX*) rf12_data; last_emontx = millis();}
      
      if (node_id == 15)
      {
        RTC.adjust(DateTime(2012, 1, 1, rf12_data[1], rf12_data[2], rf12_data[3]));
        last_emonbase = millis();
      } 
    }
  }

  //--------------------------------------------------------------------------------------------
  // Display update every 200ms
  //--------------------------------------------------------------------------------------------
  if ((millis()-fast_update)>200)
  {
    fast_update = millis();
    
    DateTime now = RTC.now();
    int last_hour = hour;
    hour = now.hour();
    minute = now.minute();

    usekwh += (emontx.power1 * 0.2) / 3600000;
    genkwh += (emontx.power2 * 0.2) / 3600000;
    
    if (last_hour == 23 && hour == 00) 
    { 
      int i; for (i=6; i>0; i--) gen_history[i] = gen_history[i-1]; 
      genkwh = 0;
      for(i=6; i>0; i--) use_history[i] = use_history[i-1];
      usekwh = 0;
      
    }
    gen_history[0] = genkwh;
    use_history[0] = usekwh;
    
    cval_use = cval_use + (emontx.power1 - cval_use)*0.50;
    cval_gen = cval_gen + (emontx.power2 - cval_gen)*0.50;
    
    last_switch_state = switch_state;
    switch_state = digitalRead(switch1);  
    if (!last_switch_state && switch_state) { page += 1; if (page>4) page = 1; }

    if (page==1)
    {
                          //use, usekwh, gen,    maxgen, genkwh, temp, mintemp, maxtemp, hour, minute, last_emontx, last_emonbase)
      draw_solar_page(cval_use, usekwh, cval_gen, maxgen, genkwh, temp, mintemp, maxtemp, hour,minute, last_emontx, last_emonbase);
      glcd.refresh();
    }
    else if (page==2)
    {
      draw_power_page( "POWER" ,cval_use, "USE", usekwh);
      draw_temperature_time_footer(temp, mintemp, maxtemp, hour,minute);
      glcd.refresh();
    }
    else if (page==3)
    {
      draw_power_page( "SOLAR" ,cval_gen, "GEN", genkwh);
      draw_temperature_time_footer(temp, mintemp, maxtemp, hour,minute);
      glcd.refresh();
    }
    else if (page==4)
    {
      draw_history_page(gen_history, use_history);
    }
    
    if ((hour > 22) ||  (hour < 5)) glcd.backLight(0); else glcd.backLight(200);  
   
    if (cval_gen>0) {
      if (cval_gen > cval_use) {            //show green LED when gen>consumption cval are the smooth curve values  
	digitalWrite(redLED, LOW);         
	digitalWrite(greenLED, HIGH);    
        
      } else {                              //red if consumption>gen
        digitalWrite(greenLED, LOW); 
	digitalWrite(redLED, HIGH);   
      }
    } else {                                //Led's off at night and when solar PV is not generating
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, LOW);
    }  
} 

  
  if ((millis()-slow_update)>10000)
  {
    slow_update = millis();

    sensors.requestTemperatures();
    temp = (sensors.getTempCByIndex(0));
    if (temp > maxtemp) maxtemp = temp;
    if (temp < mintemp) mintemp = temp;
   
    emonglcd.temperature = (int) (temp * 100);                          // set emonglcd payload
    int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}  // if ready to send + exit loop if it gets stuck as it seems too
    rf12_sendStart(0, &emonglcd, sizeof emonglcd);                      // send emonglcd data
    rf12_sendWait(0);    
  }
}
