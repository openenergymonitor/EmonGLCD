//------------------------------------------------------------------------------------------------------------------------------------------------
// emonGLCD Home Energy Monitor example
// to be used with nanode Home Energy Monitor example

// Uses power1 variable - change as required if your using different ports

// emonGLCD documentation http://openEnergyMonitor.org/emon/emonglcd

// RTC to reset Kwh counters at midnight is implemented is software. 
// Correct time is updated via NanodeRF which gets time from internet
// Temperature recorded on the emonglcd is also sent to the NanodeRF for online graphing

// GLCD library by Jean-Claude Wippler: JeeLabs.org
// 2010-05-28 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
//
// Authors: Glyn Hudson and Trystan Lea
// Part of the: openenergymonitor.org project
// Licenced under GNU GPL V3
// http://openenergymonitor.org/emon/license

// THIS SKETCH REQUIRES:

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
//	- display.ino
//	- serial.ino
//
//-------------------------------------------------------------------------------------------------------------------------------------------------
#define DEBUG

#include <OneWire.h>		    // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>      // http://download.milesburton.com/Arduino/MaximTemperature/ (3.7.2 Beta needed for Arduino 1.0)

//JeeLab libraires		       http://github.com/jcw
#include <JeeLib.h>		    // ports and RFM12 - used for RFM12B wireless
#include <RTClib.h>                 // Real time clock (RTC) - used for software RTC to reset kWh counters at midnight
#include <Wire.h>                   // Part of Arduino libraries - needed for RTClib

#include <GLCD_ST7565.h>            // Graphical LCD library 
#include <avr/pgmspace.h>           // Part of Arduino libraries - needed for GLCD lib

GLCD_ST7565 glcd;
 
#define ONE_WIRE_BUS 5              // temperature sensor connection - hard wired 
const int greenLED=6;               // Green tri-color LED - 8 on emonGLCD V1.2
const int redLED=9;                 // Red tri-color LED
const int enterswitchpin=15;		    // digital pin of onboard pushswitch 
const int LDRpin=4;    		    // analog pin of onboard lightsensor 
const int upswitchpin=16;           // digital pin of up switch - low when pressed
const int downswitchpin=19;         // digital pin of down switch - low when pressed

const int maxPower=8000;            //(8kW) Power level at which red LED is brightest - ideally this should really be calculated automatically using a clever algorithum! 

//--------------------------------------------------------------------------------------------
// RFM12B Settings
//--------------------------------------------------------------------------------------------
#define MYNODE 20            // Should be unique on network, node ID 30 reserved for base station
#define freq RF12_433MHZ     // frequency - match to same frequency as RFM12B module (change to 868Mhz or 915Mhz if appropriate)
#define group 210            // network group, must be same as emonTx and emonBase

//---------------------------------------------------
// Data structures for transfering data between units
//---------------------------------------------------
typedef struct { int power1, power2, power3, voltage; } PayloadTX;
PayloadTX emontx;

typedef struct { int temperature; } PayloadGLCD;
PayloadGLCD emonglcd;

typedef struct { int hour, mins; } PayloadBase;
PayloadBase emonbase;
//---------------------------------------------------

//--------------------------------------------------------------------------------------------
// Power variables
//--------------------------------------------------------------------------------------------
int importing, night;                                  //flag to indicate import/export
double consuming, gen, grid, wh_gen, wh_consuming;     //integer variables to store ammout of power currenty being consumed grid (in/out) +gen
unsigned long whtime;                    	       //used to calculate energy used per day (kWh/d)

//--------------------------------------------------------------------------------------------
// DS18B20 temperature setup - onboard sensor 
//--------------------------------------------------------------------------------------------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
double temp,maxtemp,mintemp;

//--------------------------------------------------------------------------------------------
// Software RTC setup
//-------------------------------------------------------------------------------------------- 
RTC_Millis RTC;
int hour;
const int time_difference=0;                        //set number of hours forward (positive) for backwards (negative) to make the emonGLCD time match local time
  
//-------------------------------------------------------------------------------------------- 
// Flow control
//-------------------------------------------------------------------------------------------- 
int view = 1;                                // Used to control which screen view is shown
unsigned long last_emontx;                   // Used to count time from last emontx update
unsigned long last_emonbase;                   // Used to count time from last emonBase update
unsigned long slow_update;                   // Used to count time for slow 10s events
unsigned long fast_update;                   // Used to count time for fast 100ms events
  
//--------------------------------------------------------------------------------------------
// Setup
//--------------------------------------------------------------------------------------------
void setup () {
    rf12_initialize(MYNODE, freq,group);
    
    glcd.begin(0x18);    //begin glcd library and set contrast 0x20 is max, 0x18 seems to look best on emonGLCD
    glcd.backLight(200); //max 255
   
    #ifdef DEBUG 
      Serial.begin(9600);
      print_glcd_setup();
    #endif
    
    pinMode(greenLED, OUTPUT); 
    pinMode(redLED, OUTPUT);  
    
    pinMode(enterswitchpin, INPUT); pinMode(upswitchpin, INPUT); pinMode(downswitchpin, INPUT); 
    // UNCOMMENT THE FOLLOWING LINE IF YOU HAVE emonGLCD V1.3 (http://openenergymonitor.blogspot.co.uk/2012/04/emonglcd-v13-switches-fix.html)
    //digitalWrite(enterswitchpin, HIGH); digitalWrite(upswitchpin, HIGH); digitalWrite(downswitchpin, HIGH); //enable Atmega328 10K internal pullup resistors
    
    sensors.begin();                         // start up the DS18B20 temp sensor onboard  
    sensors.requestTemperatures();
    temp = (sensors.getTempCByIndex(0));     // get inital temperture reading
    mintemp = temp; maxtemp = temp;          // reset min and max
    
   //RTC.begin(DateTime(__DATE__, __TIME__)); //sets the RTC to the date & time this sketch was compiled
    RTC.begin(DateTime("Dec  8 2011" , "12:00:00")); //start up software RTC - this time will be updated to correct tme from the interent via the emonBase
}
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// Loop
//--------------------------------------------------------------------------------------------
void loop () {
  
    //--------------------------------------------------------------------------------------------
    // 1. On RF recieve
    //--------------------------------------------------------------------------------------------  
    if (rf12_recvDone()){
      if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // and no rf errors
      {
        int node_id = (rf12_hdr & 0x1F);
        
        if (node_id == 10)                        // === EMONTX ====
        {
          emontx = *(PayloadTX*) rf12_data;       // get emontx payload data
          #ifdef DEBUG 
            print_emontx_payload();               // print data to serial
          #endif  
          last_emontx = millis();                 // set time of last update to now
          
          delay(100);                             // delay to make sure printing finished
          power_calculations();                   // do the power calculations
        }
        
        if (node_id == 15)                        // ==== EMONBASE ====
        {
          last_emonbase = millis();                 // set time of last update to now
          emonbase = *(PayloadBase*) rf12_data;   // get emonbase payload data
          #ifdef DEBUG 
            print_emonbase_payload();             // print data to serial
          #endif  
          RTC.adjust(DateTime(2012, 1, 1, (emonbase.hour+time_difference), emonbase.mins, 0));  // adjust emonglcd software real time clock
          
          delay(100);                             // delay to make sure printing and clock setting finished
           
          emonglcd.temperature = (int) (temp * 100);                          // set emonglcd payload
          int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}  // if ready to send + exit loop if it gets stuck as it seems too
          rf12_sendStart(0, &emonglcd, sizeof emonglcd);                      // send emonglcd data
          rf12_sendWait(0);
          #ifdef DEBUG 
            Serial.println("3 emonglcd sent");                                // print status
          #endif                               
        }
      }
    }
    
    //--------------------------------------------------------------------
    // Things to do every 10s
    //--------------------------------------------------------------------
    if ((millis()-slow_update)>10000)
    {
       slow_update = millis();
       
       backlight_control();
       
       // Get temperatue from onboard sensor
       sensors.requestTemperatures();
       temp = (sensors.getTempCByIndex(0));
       if (temp > maxtemp) maxtemp = temp;
       if (temp < mintemp) mintemp = temp;
   }

    //--------------------------------------------------------------------
    // Control toggling of screen pages
    // emonGLCD V1.3 active low, V1.4: active high
    //-------------------------------------------------------------------- 
     int S1=digitalRead(enterswitchpin); //HIGH when pressed
       int S2=digitalRead(upswitchpin);    //low when pressed
       int S3=digitalRead(downswitchpin);  //low when pressed   
    if (S1==1) draw_page_two(); // V1.4
    //if (S1==0) draw_page_two(); // V1.3

    //--------------------------------------------------------------------
    // Update the display every 200ms
    //--------------------------------------------------------------------
    if ((millis()-fast_update)>200)
    {
      fast_update = millis();
      draw_main_screen();                 //udapte LCD
      led_control();                      //adjust brightness of red LED depending on level of consumption 
    }
    
} //end loop
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------
// Calculate power and energy variables
//--------------------------------------------------------------------
void power_calculations()
{
  DateTime now = RTC.now();
  int last_hour = hour;
  hour = now.hour();
  
  if (last_hour == 23 && hour == 00) { wh_consuming = 0; }
  
  consuming = emontx.power1; 		        // for CT connected into port 1 
            
  //--------------------------------------------------
  // kWh calculation
  //--------------------------------------------------
  unsigned long lwhtime = whtime;
  whtime = millis();

  double whInc = consuming *((whtime-lwhtime)/3600000.0);
  wh_consuming=wh_consuming+whInc;
  //---------------------------------------------------------------------- 
}


