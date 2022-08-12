//------------------------------------------------------------------------------------------------------------------------------------------------
// emonGLCD Solar PV monitor example - compatiable with Arduino 1.0

// emonGLCD documentation http://openEnergyMonitor.org/emon/emonglcd
// solar PV monitor build documentation: http://openenergymonitor.org/emon/applications/solarpv

// For use with emonTx setup with 2CT with CT 1 monitoring consumption/grid and CT 2 monitoring PV generation .
// The CT's should be clipped on with the orientation so grid reading is postive when importing and negative when exporting. Generation reading should always be positive.

// Correct time is updated via NanodeRF which gets time from internet, this is used to reset Kwh/d counters at midnight.

// Temperature recorded on the emonglcd is also sent to the NanodeRF for online graphing

// this sketch is currently setup for type 1 solar PV monitoring where CT's monitor generation and consumption separately
// The sketch assumes emonx.utility is consuming/grid power and emontx.solarW is solarPV generation
// to use this sketch for type 2 solar PV monitoring where CT's monitor consumption and grid import/export using an AC-AC adapter to detect current flow direction
// change type to '2' on emonGLCD setup section below.

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
//	- RTClib		https://github.com/adafruit/RTClib
//	- GLCD_ST7565		https://github.com/openenergymonitor/glcdlib
//
// Other files in project directory (should appear in the arduino tabs above)
//	- icons.ino
//	- templates.ino
//
//-------------------------------------------------------------------------------------------------------------------------------------------------


//#define RF69_COMPAT 1 // set to 1 to use RFM69CW
//#include <JeeLib.h>   // make sure V12 (latest) is used if using RFM69CW
// RFM interface
#include "spi.h"                                                       // Requires "RFM69 Native" JeeLib Driver
#include "rf69.h"
RF69<SpiDev10> rf;


#include <GLCD_ST7565.h>
#include <avr/pgmspace.h>
GLCD_ST7565 glcd;

#include <OneWire.h>		    // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>      // http://download.milesburton.com/Arduino/MaximTemperature/ (3.7.2 Beta needed for Arduino 1.0)
#include <RTClib.h>                 // Real time clock (RTC) - used for software RTC to reset kWh counters at midnight
#include <Wire.h>                   // Part of Arduino libraries - needed for RTClib
RTC_Millis RTC;

//--------------------------------------------------------------------------------------------
// RF Settings
//--------------------------------------------------------------------------------------------
#define MYNODE 20            // Should be unique on network, node ID 30 reserved for base station
//#define RF_freq RF12_433MHZ     // frequency - match to same frequency as RFM12B module (change to 868Mhz or 915Mhz if appropriate)
#define group 210            // network group, must be same as emonTx and emonBase
#define EMONPI 5            //id of EMONPI base (where transmissions come from)
#undef EMONTX   26          //if sniffing EMONTX data, set this to id of EMONTX, else undef and then expect data from EMONPI 


byte nativeMsg[66];                                                    // 'Native' format message buffer



#undef ENABLE_HIST        //enable / disable historic readings page (#define if desired)

#ifdef ENABLE_HIST
#define NUMPAGES 4
#else
#define NUMPAGES 3
#endif



//---------------------------------------------------
// Data structures for transfering data between units
//---------------------------------------------------
#ifdef EMONTX
typedef struct {
  //power1 = utilityW, power2 = solarW , other struct members we ignore in the code
  byte hdr ;
  byte src ;
  int utilityW, solarW, power3, Vrms;
} PayloadTX;         // neat way of packaging data for RF comms
#else
typedef struct {
  byte hdr ;
  byte src ;
  byte type ;     //should always be 0A - EmonPi Power Packet
  byte  hour, min, sec, day, month, year ;
  int utilityW, solarW, utilityKwh, solarKwh;
} PayloadTX;         // neat way of packaging data for RF comms
#endif

PayloadTX emontx;


typedef struct {
  int temperature;
} PayloadGLCD;
PayloadGLCD emonglcd;


typedef struct
{
  byte hdr ;
  byte src ;
  byte stnID = 0 ; //  WH1050 station id
  byte lowBatt = 0 ; // battery status
  float tempC = 0 ; // temp in Deg C
  byte relH = 0 ; // rel humidity
  float windAvg = 0 ; // wind avg m/s
  float windGust = 0 ; // wind gust m/s
  float rain = 0 ;    // rain in mm as reported by sensor, cumlative
  float hpa = 0 ;     //Pressure  (from BMP280 sensor)
  float intempC = 0 ; //inside temp (from BMP280 sensor)
  long  lastHeard = 0 ;  //millis from when we last heard from this station
} wh1050Payload ;


wh1050Payload wx ;

//---------------------------------------------------
// emonGLCD SETUP
//---------------------------------------------------
//#define emonGLCDV1.3               // un-comment if using older V1.3 emonGLCD PCB - enables required internal pull up resistors. Not needed for V1.4 onwards
const int SolarPV_type = 1;          // Select solar PV wiring type - Type 1 is when use and gen can be monitored seperatly. Type 2 is when gen and use can only be monitored together, see solar PV application documentation for more info
const int maxgen = 6900;            // peak output of soalr PV system in W - used to calculate when to change cloud icon to a sun
byte PV_gen_offset = 5;       // When generation drops below this level generation will be set to zero - used to force generation level to zero at night

const int greenLED = 6;             // Green tri-color LED
const int redLED = 9;               // Red tri-color LED
const int LDRpin = 4;    		  // analog pin of onboard lightsensor
const int switch1 = 15;             // Push switch digital pins (active low for V1.3, active high for V1.4)
const int switch2 = 16;
const int switch3 = 19;

//---------------------------------------------------
// emonGLCD variables
//---------------------------------------------------
int hour = 12, minute = 0;
double usekwh = 0, genkwh = 0;
int cval_use, cval_gen;
byte page = 1;

#ifdef ENABLE_HIST
double use_history[7], gen_history[7];
#endif


//---------------------------------------------------
// Temperature Sensor Setup
//---------------------------------------------------
#define ONE_WIRE_BUS 5              // temperature sensor connection - hard wired
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
double intemp, outtemp, maxtemp, mintemp;


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
  delay(500); 				   //wait for power to settle before firing up the RF
  rf.init(MYNODE, 210, 434);
  delay(100);				   //wait for RF to settle befor turning on display
  glcd.begin(0x19);
  glcd.backLight(200);

  Serial.begin (57600);
  Serial.println("hello");
  sensors.begin();                         // start up the DS18B20 temp sensor onboard
  sensors.requestTemperatures();
  intemp = (sensors.getTempCByIndex(0));     // get inital temperture reading
  mintemp = intemp; maxtemp = intemp;          // reset min and max

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);

#ifdef emonGLCDV1.3                      //enable internal pull up resistors for push switches on emonGLCD V1.3 (old)
  pinMode(switch1, INPUT); pinMode(switch2, INPUT); pinMode(switch2, INPUT);
  digitalWrite(switch1, HIGH); digitalWrite(switch2, HIGH); digitalWrite(switch3, HIGH);
#endif

  //Set Date to a reasonable default (midnight, Jan1, 2020)
  RTC.adjust(DateTime(2020, 1, 1, 0, 0, 0));
}

//--------------------------------------------------------------------------------------------
// Loop
//--------------------------------------------------------------------------------------------
void loop()
{

  int len = rf.receive(&nativeMsg, sizeof(nativeMsg));

  if (len > 0 ) {
    Serial.print("Received something len:"); Serial.println(len);
    char rfdata[len] ;

    byte sender = nativeMsg[1];

#ifdef EMONTX
    if (sender == EMONTX && len == sizeOf(emonTx) ) {
      memcpy (rfdata, nativeMsg, len) ;
      emontx = *(PayloadTX*) rfdata;
      last_emontx = millis();
    }

    if (sender == 15) {     // I think this is EMONBASE sending the time
      RTC.adjust(DateTime(2018, 1, 1, nativeMsg[3], nativeMsg[4], nativeMsg[5]));
      last_emonbase = millis();
    }

#else
    //Sanity with an EMONPI :)

    if (sender == EMONPI && nativeMsg[2] == 0x0A) {
      //this packet is for us
      memcpy (rfdata, nativeMsg, len) ;
      emontx = *(PayloadTX*) rfdata;
      last_emontx = millis();
      RTC.adjust(DateTime(emontx.year, emontx.month, emontx.day, emontx.hour, emontx.min, emontx.sec));
      last_emonbase = millis();

    }


#endif

    //sniff my weather station packet to get external temperature displayed
    if ( len == sizeof(wx))  {
      memcpy (rfdata, nativeMsg, len) ;
      wx = *(wh1050Payload*) rfdata;
      outtemp = wx.tempC ;
    }

  } // if len>0




  //--------------------------------------------------------------------------------------------
  // Display update every 200ms
  //--------------------------------------------------------------------------------------------
  if ((millis() - fast_update) > 200)
  {
    fast_update = millis();

    DateTime now = RTC.now();
    int last_hour = hour;
    hour = now.hour();
    minute = now.minute();

#ifdef EMONTX
    //if data came from EMONTX, calculate the kwh & manipulate (why?) the current values
    if (SolarPV_type == 1) {
      usekwh += (emontx.utilityW * 0.2) / 3600000;
      genkwh += (emontx.solarW * 0.2) / 3600000;
      cval_use = cval_use + (emontx.utilityW - cval_use) * 0.50;
      cval_gen = cval_gen + (emontx.solarW - cval_gen) * 0.50;
    }

    if (SolarPV_type == 2) {
      usekwh += ((emontx.utilityW + emontx.solarW) * 0.2) / 3600000;
      genkwh += (emontx.solarW * 0.2) / 3600000;
      cval_use = cval_use + ((emontx.utilityW + emontx.solarW) - cval_use) * 0.50;
      cval_gen = cval_gen + (emontx.solarW - cval_gen) * 0.50;
    }
#else
    //if data came from EMONPI, use what it told us for the kwh & current values
    usekwh = emontx.utilityKwh / 100.0 ;
    genkwh = emontx.solarKwh / 100.0 ;
    cval_use = emontx.utilityW ;
    cval_gen = emontx.solarW ;
#endif




    if (last_hour == 23 && hour == 00) {
#ifdef ENABLE_HIST
      int i; for (i = 6; i > 0; i--) gen_history[i] = gen_history[i - 1];
      for (i = 6; i > 0; i--) use_history[i] = use_history[i - 1];
#endif
      genkwh = 0;
      usekwh = 0;
    }

#ifdef ENABLE_HIST
    gen_history[0] = genkwh;
    use_history[0] = usekwh;
#endif


    if (cval_gen < PV_gen_offset) cval_gen = 0;              //set generation to zero when generation level drops below a certian level (at night) eg. 20W

    last_switch_state = switch_state;
    switch_state = digitalRead(switch1);
    if (!last_switch_state && switch_state) {
      page += 1;

      if (page > NUMPAGES) page = 1;
    }

    last_switch_state = switch_state;
    switch_state = digitalRead(switch2);
    if (!last_switch_state && switch_state) {
      if ( PV_gen_offset != 0 ) {
        PV_gen_offset = 0 ;
      } else {
        PV_gen_offset = 5 ;
      }
    }


    if (page == 1)
    {
      //use, usekwh, gen,    maxgen, genkwh, temp, mintemp, maxtemp, hour, minute, last_emontx, last_emonbase)
      draw_solar_page(cval_use, usekwh, cval_gen, maxgen, genkwh, intemp, outtemp, now, last_emontx, last_emonbase);
      glcd.refresh();
    }
    else if (page == 2)
    {
      draw_power_page( "Utility Power" , (cval_use), "USED", usekwh);
      draw_temperature_time_footer(intemp, mintemp, maxtemp, now);
      glcd.refresh();
    }
    else if (page == 3)
    {
      draw_power_page( "Solar Power" , cval_gen, "GEN", genkwh);
      draw_temperature_time_footer(intemp, mintemp, maxtemp, now);
      glcd.refresh();
    }
#ifdef ENABLE_HIST
    else if (page == 4)
    {
      draw_history_page(gen_history, use_history);
    }
#endif


    int LDR = analogRead(LDRpin);                     // Read the LDR Value so we can work out the light level in the room.
    int LDRbacklight = map(LDR, 0, 1023, 1, 250);    // Map the data from the LDR from 0-1023 (Max seen 1000) to var GLCDbrightness min/max
    LDRbacklight = constrain(LDRbacklight, 0, 255);   // Constrain the value to make sure its a PWM value 0-255
    if ((hour > 23) ||  (hour < 6)) glcd.backLight(0); else
      glcd.backLight(LDRbacklight);

    int PWRleds = 0 ;

    if (PV_gen_offset == 0) {
      PWRleds = map(cval_use, 0, maxgen, 0, 255);  // No PV been generated, map the LED brightness to used (aka utility) power only, with maxgen being the maximum

    } else {
      PWRleds = map(cval_use - cval_gen, 0, maxgen, 0, 255);  // Map importing value from (LED brightness - cval3 is the smoothed grid value - see display above

    }
    if (PWRleds < 0) PWRleds = PWRleds * -1;                    // keep it positive
    PWRleds = constrain(PWRleds, 0, 255);                       // Constrain the value to make sure its a PWM value 0-255

    if (cval_gen >= PV_gen_offset) {
      if (cval_gen > cval_use) {            //show green LED when gen>consumption cval are the smooth curve values
        analogWrite(redLED, 0);
        analogWrite(greenLED, PWRleds);

      } else {                              //red if consumption>gen
        analogWrite(greenLED, 0);
        analogWrite(redLED, PWRleds);

      }
    } else {                                //Led's off when it's dark in the room (imagine watching a movie)
      analogWrite(redLED, 0);
      analogWrite(greenLED, 0);
    }
  }

  if ((millis() - slow_update) > 35000)
  {
    slow_update = millis();

    sensors.requestTemperatures();
    double rawtemp = (sensors.getTempCByIndex(0));
    if ((rawtemp > -30) && (rawtemp < 70)) {
      intemp = rawtemp;            //is temperature withing reasonable limits?
      emonglcd.temperature = (int) (intemp * 100);                          // set emonglcd payload
      rf.send(0, &emonglcd, sizeof emonglcd);

      /* rf12_sendNow(0, &emonglcd, sizeof emonglcd);                     //send temperature data via RFM12B using new rf12_sendNow wrapper -glynhudson
        rf12_sendWait(2);*/
    }
    if (intemp > maxtemp) maxtemp = intemp;
    if (intemp < mintemp) mintemp = intemp;


  }
}
