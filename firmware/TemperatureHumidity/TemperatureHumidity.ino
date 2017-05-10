/*

  EmonGLCD Temperature/Humidity Display (using DHT22)

  Contributors: Glyn Hudson, Trystan Lea
  Icons implementation by DrsDre
  GLCD library by Jean-Claude Wippler: JeeLabs.org
  This temperature/humidity customisation by Ben Aylott.

  Part of the: openenergymonitor.org project
  Licenced under GNU GPL V3

*/
#define RF69_COMPAT 1 // set to 1 to use RFM69CW 
#include <JeeLib.h>
#include <GLCD_ST7565.h>
#include <avr/pgmspace.h>
#include <DHT.h>
#include <RTClib.h>                 // Real time clock (RTC) - used for software RTC
#include <Wire.h>                   // Part of Arduino libraries - needed for RTClib

#define DHTPIN 5                    //DHT sensor is on Dig Pin 5
#define DHTTYPE DHT22

RTC_Millis RTC;

GLCD_ST7565 glcd;
DHT dht(DHTPIN, DHTTYPE);

unsigned long fast_update,slow_update;
const int LDRpin=4;            // analog (ADC) pin of onboard lightsensor


//--------------------------------------------------------------------------------------------
// RFM12B Settings
//--------------------------------------------------------------------------------------------
#define MYNODE 29            // Should be unique on network, node ID 30 reserved for base station
#define BASENODE 5
#define RF_freq RF12_868MHZ     // frequency - match to same frequency as RFM12B module (change to 868Mhz or 915Mhz if appropriate)
#define group 210

//---------------------------------------------------
// Data structures for TX
//---------------------------------------------------

typedef struct {
  int temperature;
  int humidity;
  int ldr;
} PayloadGLCDTH;

PayloadGLCDTH emonglcd;

//--------------------------------------------------------------------------------------------
// Flow control
//--------------------------------------------------------------------------------------------
unsigned long last_emontx;                   // Used to count time from last emontx update (not used)
unsigned long last_emonbase;                   // Used to count time from last emonbase update


// Variables
double temp,maxtemp,mintemp;
double hum,minhum,maxhum;
int LDR;

int hour = 12, minute = 0;

void setup()
{
  delay(500);            //wait for power to settle before firing up the RF
  rf12_initialize(MYNODE, RF_freq,group);
  delay(100);          //wait for RF to settle befor turning on display
  glcd.begin(0x20);
  glcd.backLight(200);
  dht.begin();

  hum = dht.readHumidity();
  minhum=hum;
  maxhum=hum;

  temp = dht.readTemperature();
  mintemp=temp;
  maxtemp=temp;

}

void loop()
{

  if (rf12_recvDone())
  {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // and no rf errors
    {
      int node_id = (rf12_hdr & 0x1F);

      if (node_id == BASENODE)
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

    LDR = analogRead(LDRpin);                     // Read the LDR Value so we can work out the light level in the room.
    int LDRbacklight = map(LDR, 0, 1023, 50, 250);    // Map the data from the LDR from 0-1023 (Max seen 1000) to var GLCDbrightness min/max
    LDRbacklight = constrain(LDRbacklight, 0, 255);   // Constrain the value to make sure its a PWM value 0-255
    //if ((hour > 22) ||  (hour < 5)) glcd.backLight(0); else     // Turn off backlight during night hours...
    glcd.backLight(255-LDRbacklight);            // NOTE this inversion to increase backlight when light levels low...

    hum = dht.readHumidity();                    // Read humidity from DHT
    temp = dht.readTemperature();                // Read temperature from DHT

    // Don't use buttons...
//    int S1 = digitalRead(15);
//    int S2 = digitalRead(16);
//    int S3 = digitalRead(19);

      // Draw GLCD
      // temp,mintemp,maxtemp,hum,minhum,maxhum,hour,minute,last_emonbase,node id
      draw_th_page(temp,mintemp,maxtemp,hum,minhum,maxhum,hour,minute,last_emonbase,MYNODE);
      glcd.refresh();
  }

  if ((millis()-slow_update)>10000)
  {
    slow_update = millis();

    hum = dht.readHumidity();
    temp = dht.readTemperature();

    // See if min/max temp needs to change
    if (temp > maxtemp) maxtemp = temp;
    if (temp < mintemp) mintemp = temp;

    // See if min/max humidity needs to change
    if (hum > maxhum) maxhum = hum;
    if (hum < minhum) minhum = hum;

    glcd.refresh();

    // Set TX payload to base station
    emonglcd.temperature = (int) (temp * 100);
    emonglcd.humidity = (int) (hum * 100);
    emonglcd.ldr=(int)LDR;
    // Send TX payload to base station
    rf12_sendNow(0, &emonglcd, sizeof emonglcd);
    rf12_sendWait(2);
  }
}
