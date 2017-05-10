//------------------------------------------------------------------------------------------------------------------------------------------------
// emonGLCD Test - tests LED's, temperature and light sensors
// emonGLCD documentation http://openEnergyMonitor.org/emon/emonglcd

// GLCD library by Jean-Claude Wippler: JeeLabs.org
// 2010-05-28 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
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
//	- GLCD_ST7565		https://github.com/jcw/glcdlib
//-------------------------------------------------------------------------------------------------------------------------------------------------

#include <OneWire.h>		    // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>      // http://download.milesburton.com/Arduino/MaximTemperature/ (3.7.2 Beta needed for Arduino 1.0)

//JeeLab libraires		       http://github.com/jcw
#define RF69_COMPAT 1 // set to 1 to use RFM69CW 
#include <JeeLib.h>   // make sure V12 (latest) is used if using RFM69CW
#include <JeeLib.h>		    // ports and RFM12 - used for RFM12B wireless

#include <GLCD_ST7565.h>            // Graphical LCD library
#include <avr/pgmspace.h>           // Part of Arduino libraries - needed for GLCD lib
#include "utility/font_clR6x8.h"    //font

GLCD_ST7565 glcd;

#define ONE_WIRE_BUS 5              // on board temperature sensor connection
const int redLED=9;               // Green tri-color LED
const int greenLED=6;                // Red tri-color LED - dig9 on old emonGLCD V1.2
const int LDRpin=A4;                // ADC pin of onboard lightsensor - ADC 0 on old emonGLCD V1.2

//const int enterswitchpin=A1;        // ADC pin of enter switch
//const int upswitchpin=A2;           // ADC pin of up switch
//const int downswitchpin=A5;         // ADC pin of down switch

const int enterswitchpin=15;        // digital pin of enter switch - low when pressed
const int upswitchpin=16;           // digital pin of up switch - low when pressed
const int downswitchpin=19;         // digital pin of down switch - low when pressed

long int last;
int RED,GREEN;
double temp;


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
    pinMode(enterswitchpin, INPUT); pinMode(upswitchpin, INPUT); pinMode(downswitchpin, INPUT);
    // UNCOMMENT THE FOLLOWING LINE IF YOU HAVE emonGLCD V1.3 (http://openenergymonitor.blogspot.co.uk/2012/04/emonglcd-v13-switches-fix.html)
    //digitalWrite(enterswitchpin, HIGH); digitalWrite(upswitchpin, HIGH); digitalWrite(downswitchpin, HIGH); //enable Atmega328 10K internal pullup resistors

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
       int S1=digitalRead(enterswitchpin);
       int S2=digitalRead(upswitchpin);
       int S3=digitalRead(downswitchpin);



  glcd.drawString_P(0,  0, PSTR("emonGLCD tester"));
  glcd.drawString_P(0,  10, PSTR("OpenEnergyMonitor"));
  glcd.drawString_P(0,  25, PSTR("Temperature: "));
  glcd.drawString_P(0,  35, PSTR("Light level:  "));
  glcd.drawString_P(0,  55, PSTR("Switches: "));

  char str[50];
  dtostrf(temp,0,1,str);
  strcat(str,"C");
  glcd.drawString(74,25,str);

  itoa(LDR,str,10);
  glcd.drawString(73,35,str);

   if (millis()>(last+3000)){
   //if (RED==1) Serial.println("Red LED");
  // if (GREEN==1)  Serial.println("Green LED");
   RED=!RED;
   GREEN=!GREEN;
   last=millis();

   Serial.print("temp: "); Serial.print(temp); Serial.println(" ");
   Serial.print("Light: "); Serial.print(LDR); Serial.println(" ");
   Serial.print("Switches (low when pressed) : ");
   Serial.print(S1); Serial.print(" ");
   Serial.print(S2); Serial.print(" ");
   Serial.print(S3); Serial.println(" ");

 }


  if (RED==1){
  glcd.drawString_P(0,  45, PSTR("Green LED"));
  digitalWrite(greenLED,HIGH); digitalWrite(redLED,LOW);
  }

  if (GREEN==1){
  glcd.drawString_P(0,  45, PSTR("RED LED"));
 digitalWrite(greenLED,LOW); digitalWrite(redLED,HIGH);
  }

  if (S1==1) glcd.drawString_P(60,  55, PSTR("Enter"));
  if (S2==1) glcd.drawString_P(60,  55, PSTR("Up"));
  if (S3==1) glcd.drawString_P(60,  55, PSTR("Down"));


    glcd.refresh();
} //end loop
//--------------------------------------------------------------------------------------------
