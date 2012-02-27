//------------------------------------------------------------------------------------------------------------------------------------------------
// emonGLCD Test - tests LED's, temperature and light sensors 
// emonGLCD documentation http://openEnergyMonitor.org/emon/emonglcd
// push switches don't work on emonGLCD V1.3 


// GLCD library by Jean-Claude Wippler: JeeLabs.org
// 2010-05-28 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
//
// Authors: Glyn Hudson and Trystan Lea
// Part of the: openenergymonitor.org project
// Licenced under GNU GPL V3
// http://openenergymonitor.org/emon/license

//-------------------------------------------------------------------------------------------------------------------------------------------------

#include <OneWire.h>		    // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>      // http://download.milesburton.com/Arduino/MaximTemperature/ (3.7.2 Beta needed for Arduino 1.0)

//JeeLab libraires		       http://github.com/jcw
#include <JeeLib.h>		    // ports and RFM12 - used for RFM12B wireless

#include <GLCD_ST7565.h>            // Graphical LCD library 
#include <avr/pgmspace.h>           // Part of Arduino libraries - needed for GLCD lib
#include "utility/font_clR6x8.h"    //font 

GLCD_ST7565 glcd;

#define ONE_WIRE_BUS 5              // on board temperature sensor connection 
const int redLED=9;               // Green tri-color LED
const int greenLED=6;                 // Red tri-color LED - dig9 on old emonGLCD V1.2
const int LDRpin=4;    		    // ADC pin of onboard lightsensor - ADC 0 on old emonGLCD V1.2

long int last; 
int RED,GREEN;
double temp;

//Heartbeat LED
//elCalvoMike 12-6-2008
int i = 0;
int pmw = 255;  //set PWM max - this can differ for other board pins
int rate = 25;  //this is the beats per minute (60000 ms)
               //because there are two beats to simulate the 'lub-dub' of the heart,
               // a 60 beat heart rate is only a value of 30 in the rate variable
               //the delay is the key to this programs realism - divide the rate into a minute, then weight it and divide by the pmw
               //you can modify the weight by changing the fractions (i.e .1, .2, .6) but to keep the timing correct, they should total 1
               //.1+.2+.1+.6 = 1

//--------------------------------------------------------------------------------------------
// DS18B20 temperature setup - onboard sensor 
//--------------------------------------------------------------------------------------------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//--------------------------------------------------------------------------------------------
// Setup
//--------------------------------------------------------------------------------------------
void setup () {
    
    glcd.begin(0x18);    //begin glcd library and set contrast 0x20 is max, 0x18 seems to look best on emonGLCD
    glcd.backLight(200); //max 255
    glcd.setFont(font_clR6x8);
    
    pinMode(greenLED, OUTPUT); 
    pinMode(redLED, OUTPUT); 
    
    sensors.begin();                         // start up the DS18B20 temp sensor onboard  
    sensors.requestTemperatures();
    temp = (sensors.getTempCByIndex(0));     // get inital temperture reading
    
    Serial.begin(9600);
    Serial.println("emonGLCD demo");
    Serial.println("openenergymonitor.org");
    RED=1;  
}
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// Loop
//--------------------------------------------------------------------------------------------
void loop () {
  
 
  
 glcd.clear();
    
  
       sensors.requestTemperatures();
       temp = (sensors.getTempCByIndex(0));
       
       int LDR=analogRead(LDRpin); 

  glcd.drawString_P(0,  0, PSTR("emonGLCD tester"));
  glcd.drawString_P(0,  10, PSTR("OpenEnergyMonitor"));
  glcd.drawString_P(0,  30, PSTR("Temperature: "));
  glcd.drawString_P(0,  40, PSTR("Light level: "));
  
  char str[50];       
  dtostrf(temp,0,1,str); 
  strcat(str,"C");
  glcd.drawString(74,30,str); 
  
  itoa(LDR,str,10); 
  glcd.drawString(70,40,str); 
  
   if (millis()>(last+3000)){
   if (RED==1) Serial.println("Red LED");
   if (GREEN==1)  Serial.println("Green LED");
   RED=!RED;
   GREEN=!GREEN;
   last=millis();
   Serial.print("temp: "); Serial.print(temp); Serial.println(" ");
   Serial.print("Light: "); Serial.print(LDR); Serial.println(" ");
  
   
 }
  
  
  if (RED==1){
  glcd.drawString_P(0,  50, PSTR("Green LED"));
  digitalWrite(greenLED,HIGH); digitalWrite(redLED,LOW);
  
  
 
  }
  
  
  if (GREEN==1){
  glcd.drawString_P(0,  50, PSTR("RED LED"));
 digitalWrite(greenLED,LOW); digitalWrite(redLED,HIGH);
 
  }
    
    
    glcd.refresh();
} //end loop
//--------------------------------------------------------------------------------------------




