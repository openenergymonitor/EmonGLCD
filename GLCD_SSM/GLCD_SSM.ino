/* RF Signal Strength Meter and scanner for 464 or 868MHz bands, Martin Roberts 30/05/13
based on OpenEnergyMonitor emonGLCD hardware http://openenergymonitor.org/emon/emonglcd

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this software; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

History:
Version 1.00 30/05/13 - first release
Version 1.01 05/06/13 - sleep_bod_disable macro added
*/

#define VERSION "v1.01"
#define RF69_COMPAT 0 // set to 1 to use RFM69CW 
#include <JeeLib.h>       // https://github.com/jcw/jeelib (only needed because referenced by GLCDlib)
#include <SPI.h>
#include <GLCD_ST7565.h>  // https://github.com/jcw/glcdlib
#include <EEPROM.h>
#include <avr/sleep.h>
#include "utility/font_clR4x6.h"
#include "utility/font_courBO14.h"

//--------------------------------------------------------------------------------------------
// RFM12B Settings
// #define USE868MHZ       // uncomment this for 868MHz, else 464MHz
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Arduino I/O pin useage
#define GREENLEDPIN 6        // Green tri-color LED
#define REDLEDPIN 9          // Red tri-color LED
#define LDRPIN 4    	     // analog pin of onboard light sensor, used as ARSSIPIN in this application
#define SWITCH1PIN 15        // Push switch digital pins (requires V1.4 or greater, active high)
#define SWITCH2PIN 16        
#define SWITCH3PIN 19
#define ARSSIPIN 4           // analogue receive signal strength indicator on RFM12
#define RFMSELPIN 10         // RFM12 chip select pin
#define GCDRESETPIN 7        // reset pin for LCD
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// EEPROM addresses
#define EE_SCALE 0
#define EE_TRIG_LEVEL 1
#define EE_TRIG_POS 2
#define EE_ATTN 3
#define EE_GLITCH 4
//--------------------------------------------------------------------------------------------------

#define Y0 57 // Y screen coordinate for start of plot

GLCD_ST7565 glcd;
boolean triggered;
boolean scanComplete;
boolean timerFlag;
byte scanBuf[128];
byte count;
byte minv; // RSSI output voltage for -100dBm
byte flashState;
byte cursorPos;
word samplesPerCount;
// these variables are saved in EEPROM
byte scale;
byte trigLevel;
byte trigPos;
boolean attenuationOn;
boolean glitchDetectionOn;

void setup()
{
  pinMode(GREENLEDPIN,OUTPUT);
  pinMode(REDLEDPIN,OUTPUT);
  bitSet(DIDR0,ARSSIPIN); // disable digital input buffer for ARSSI input
  RFM12init();
  glcd.begin(0x1c);
  glcd.clear();
  glcd.setFont(font_courBO14);
  glcd.drawString_P(20,26,PSTR("GLCD SSM"));
  glcd.setFont(font_clR4x6);
  glcd.drawString_P(0,59,PSTR("MR 2013"));
  glcd.drawString_P(106,59,PSTR(VERSION));
  glcd.refresh();
  glcd.backLight(200);
  delay(4000);
  minv=47; // start at 600mV 0.6/3.3 x 256 = 47
  for(int i=0; i<128; i++) scanBuf[i]=minv;
  cursorPos=0;
  
  // restore settings from EEPROM
  restoreSetting(EE_SCALE, &scale, 10, 5);
  restoreSetting(EE_TRIG_LEVEL, &trigLevel, Y0, 10);
  restoreSetting(EE_TRIG_POS, &trigPos, 127, 64);
  restoreSetting(EE_ATTN, &attenuationOn, 1, 0);
  restoreSetting(EE_GLITCH, &glitchDetectionOn, 1, 1);
  setSamplesPerCount();
  updateScreen();
  
  //set timer 1 interrupt for required period
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS10); // CTC mode, no prescaling
  TCNT1  = 0; // clear counter
  OCR1A = 125 * 16; // set compare reg for 125 microsecond timer period
  bitSet(TIMSK1,OCIE1A); // enable timer 1 compare interrupt
  bitSet(ADCSRA,ADIE); // enable ADC interrupt
  interrupts();
}

void loop()
{
  if(scanComplete) updateScreen();
  if(timerFlag) checkSwitches();
}

void updateScreen()
{
  byte i,y,oldy,maxv=0;
  
  glcd.clear();
  glcd.setFont(font_clR4x6);
  y=getY(count);
  for(i=0; i<128; i++)
  {
    oldy=y;
    y=getY(count);
    glcd.drawLine(i,oldy,i,y,1);
    count++;
    count&=0x7f;
    if(scanBuf[i]>maxv) maxv=scanBuf[i];
  }
  
  glcd.drawString_P(0,59,PSTR("BL"));
  glcd.drawString_P(55,59,PSTR("GD"));
  glcd.drawString_P(98,59,PSTR("TB"));
  displayPeak(maxv);
  displayTriggerLevel(1);
  displayTriggerPosition(1);
  displayScale(1);
  displayAttenuation(1);
  displayGlitchDetection(1);
  glcd.drawLine(0,Y0,127,Y0,1);
  glcd.refresh();
  triggered=false;
  scanComplete=false;
  digitalWrite(GREENLEDPIN,LOW);
}

void displayPeak(byte peakv)
{
  byte deltav;
  float dbm;
  char str[10];
  
  deltav=peakv-minv;
  if(deltav>=(Y0-2)) glcd.drawString_P(0,0,PSTR("OVL"));
  else
  {
    dbm= (deltav*3.3/256)*35/0.7 - (attenuationOn?80:100); // RSSI o/p swings from -100dBm to -65dBm over 0.7V
    itoa((int)(dbm+0.5),str,10);
    strcat(str,"dBm");
    glcd.drawString(0,Y0-deltav-2,str);
  }
}

byte getY(byte index)
{
  byte y;
  
  y=scanBuf[index]-minv;
  if(y>Y0) y=0;
  else y=Y0-y;
  return y;
}

void checkSwitches()
{
  static byte lastState,lastS1State;
  static byte flashTimer,repeatTimeout,saveTimeout,sleepTimeout;
  byte sState;
  boolean repeatFlag=false;
  
  sState=digitalRead(SWITCH1PIN);
  if(sState)
  {
    if(sleepTimeout) sleepTimeout--;
    if(sleepTimeout==0) powerDown();
  }
  else sleepTimeout=100;

  if(sState && !lastS1State)
  {    
    displayCursorItem(1); // make sure last item is displayed
    cursorPos++;
    if(cursorPos>4) cursorPos=0;
  }
  lastS1State=sState;

  sState=digitalRead(SWITCH2PIN) | (digitalRead(SWITCH3PIN)<<1);
  if(sState==lastState)
  {
    if(sState)
    {
      if(repeatTimeout) repeatTimeout--;
      if(repeatTimeout==0) repeatFlag=true;
    }
    else
    {
      if(saveTimeout)
      {
        saveTimeout--;
        if(saveTimeout==0) saveSettings();
      }
    }
  }
  else repeatTimeout=20;
  
  if(repeatFlag || (sState && (sState!=lastState)))
  {
    adjustCursorItem(sState & 2);
    saveTimeout=50;
  }
  lastState=sState;
  
  flashTimer++;
  if(flashTimer>24)
  {
    flashState^=1;
    displayCursorItem(flashState);
    glcd.refresh();
    flashTimer=0;
  }

  timerFlag=false;
}

void saveSettings()
{
  if(EEPROM.read(EE_TRIG_LEVEL) != trigLevel) EEPROM.write(EE_TRIG_LEVEL,trigLevel);
  if(EEPROM.read(EE_TRIG_POS) != trigPos) EEPROM.write(EE_TRIG_POS,trigPos);
  if(EEPROM.read(EE_ATTN) != attenuationOn) EEPROM.write(EE_ATTN,attenuationOn);
  if(EEPROM.read(EE_GLITCH) != glitchDetectionOn) EEPROM.write(EE_GLITCH,glitchDetectionOn);
  if(EEPROM.read(EE_SCALE) != scale) EEPROM.write(EE_SCALE,scale);
}

void adjustCursorItem(boolean down)
{
  switch(cursorPos)
  {
    case 0: adjustTriggerLevel(down); break;
    case 1: adjustTriggerPosition(down); break;
    case 2: adjustAttenuation(down); break;
    case 3: adjustGlitchDetection(down); break;
    case 4: adjustScale(down); break;
  }
}

void displayCursorItem(byte colour)
{
  switch(cursorPos)
  {
    case 0: displayTriggerLevel(colour); break;
    case 1: displayTriggerPosition(colour); break;
    case 2: displayAttenuation(colour); break;
    case 3: displayGlitchDetection(colour); break;
    case 4: displayScale(colour); break;
  }
}

void adjustTriggerLevel(boolean down)
{
  displayTriggerLevel(0);
  if(!down && (trigLevel<Y0)) trigLevel++;
  if(down && trigLevel) trigLevel--;
  displayTriggerLevel(1);
  glcd.refresh();
}

void adjustTriggerPosition(boolean down)
{
  displayTriggerPosition(0);
  if(!down && (trigPos<127)) trigPos++;
  if(down && trigPos) trigPos--;
  displayTriggerPosition(1);
  glcd.refresh();
}

void adjustAttenuation(boolean down)
{
  displayAttenuation(0);
  attenuationOn=!down;
  setLNAgain(attenuationOn?3:0);
  displayAttenuation(1);
  glcd.refresh();
}

void adjustGlitchDetection(boolean down)
{
  displayGlitchDetection(0);
  glitchDetectionOn=!down;
  displayGlitchDetection(1);
  glcd.refresh();
}

void adjustScale(boolean down)
{
  displayScale(0);
  if(!down && (scale<10)) scale++;
  if(down && scale) scale--;
  setSamplesPerCount();
  displayScale(1);
  glcd.refresh();
}

void setSamplesPerCount()
{
  switch(scale)
  {
    case 0: samplesPerCount=1; break;
    case 1: samplesPerCount=2; break;
    case 2: samplesPerCount=4; break;
    case 3: samplesPerCount=8; break;
    case 4: samplesPerCount=16; break;
    case 5: samplesPerCount=31; break;
    case 6: samplesPerCount=63; break;
    case 7: samplesPerCount=125; break;
    case 8: samplesPerCount=250; break;
    case 9: samplesPerCount=500; break;
    case 10: samplesPerCount=1000; break;
  }
}

void displayTriggerLevel(byte colour)
{
  glcd.drawLine(124,Y0-trigLevel,127,Y0-trigLevel,colour);
  glcd.setPixel(125,Y0-trigLevel-1,colour);
  glcd.setPixel(125,Y0-trigLevel+1,colour);
}

void displayAttenuation(byte colour)
{
  if(colour) glcd.drawString_P(9,59,attenuationOn?PSTR("-80dBm"):PSTR("-100dBm"));
  else glcd.fillRect(9,59,27,6,0);
}

void displayGlitchDetection(byte colour)
{
  if(colour) glcd.drawString_P(64,59,glitchDetectionOn?PSTR("On"):PSTR("off"));
  else glcd.fillRect(64,59,16,6,0);
}

void displayTriggerPosition(byte colour)
{
  glcd.drawLine(trigPos,0,trigPos,3,colour);
  glcd.setPixel(trigPos-1,2,colour);
  glcd.setPixel(trigPos+1,2,colour);
}

void displayScale(byte colour)
{
  char s[6];
  
  if(colour)
  {
    switch(scale)
    {
      case 0: glcd.drawString_P(107,59,PSTR("16ms")); break;
      case 1: glcd.drawString_P(107,59,PSTR("32ms")); break;
      case 2: glcd.drawString_P(107,59,PSTR("64ms")); break;
      case 3: glcd.drawString_P(107,59,PSTR("128ms")); break;
      case 4: glcd.drawString_P(107,59,PSTR("256ms")); break;
      case 5: glcd.drawString_P(107,59,PSTR("500ms")); break;
      case 6: glcd.drawString_P(107,59,PSTR("1s")); break;
      case 7: glcd.drawString_P(107,59,PSTR("2s")); break;
      case 8: glcd.drawString_P(107,59,PSTR("4s")); break;
      case 9: glcd.drawString_P(107,59,PSTR("8s")); break;
      case 10: glcd.drawString_P(107,59,PSTR("16s")); break;
      default: glcd.drawString_P(107,59,PSTR("???")); break;
    }
  }
  else glcd.fillRect(107,59,19,6,0);
}

// timer 1 interrupt handler
ISR(TIMER1_COMPA_vect)
{
  static byte timerCount=0;
  
  ADMUX = _BV(REFS0) | _BV(ADLAR) | ARSSIPIN; // Vcc ref, 8-bit result, select ARSSI
  ADCSRA |= _BV(ADSC); // start ADC conversion
  
  timerCount++;
  if(timerCount>159) // 20ms
  {
    timerCount=0;
    timerFlag=true;
  }
}

// ADC interrupt handler
ISR(ADC_vect)
{
  static word sampleCount=1;
  static byte v=0;
  static byte scanCompleteCount;
    
  if(ADCH<minv) minv=ADCH;
  if(scanComplete) return; // screen is still updating
  
  if(!glitchDetectionOn || (ADCH>v)) v=ADCH; // detect peak for sample
  if((ADCH-minv)>=(Y0-2)) digitalWrite(REDLEDPIN,HIGH);
  else digitalWrite(REDLEDPIN,LOW);
  
  if(--sampleCount == 0)
  {
    if(!triggered && ((v-minv)>=trigLevel))
    {
      scanCompleteCount=(count+(127-trigPos))&0x7f;
      triggered=true;
      digitalWrite(GREENLEDPIN,HIGH);
    }
    
    if(triggered && (count==scanCompleteCount)) scanComplete=true;

    scanBuf[count]=v;
    v=0;
    count++;
    count&=0x7f;
    sampleCount=samplesPerCount;
  }
}

void RFM12init()
{
  // start the SPI library:
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(0);
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  // initialise RFM12
  delay(200); // wait for RFM12 POR
  rfm_write(0x0000); // clear SPI
#ifdef USE868MHZ
  rfm_write(0x80E7); // EL (ena dreg), EF (ena RX FIFO), 12.0pF 
#else
  rfm_write(0x80D7); // EL (ena dreg), EF (ena RX FIFO), 12.0pF 
#endif
  rfm_write(0x8280); // enable receiver
  rfm_write(0xA640); // 434MHz/868MHz
  rfm_write(0xC606); // approx 49.2 Kbps, as used by emonTx
  //rfm_write(0xC657); // approx 3.918 Kbps, better for long range
  rfm_write(0xCC77); // PLL 
  rfm_write(0x94A0); // VDI,FAST,134kHz,0dBm,-103dBm 
  rfm_write(0xC2AC); // AL,!ml,DIG,DQD4 
  rfm_write(0xCA83); // FIFO8,2-SYNC,!ff,DR 
  rfm_write(0xCEd2); // SYNC=2DD2
  rfm_write(0xC483); // @PWR,NO RSTRIC,!st,!fi,OE,EN 
  rfm_write(0x9850); // !mp,90kHz,MAX OUT 
  rfm_write(0xE000); // wake up timer - not used 
  rfm_write(0xC800); // low duty cycle - not used 
  rfm_write(0xC000); // 1.0MHz,2.2V
}

// write a command to the RFM12
word rfm_write(word cmd)
{
  word result;
  
  digitalWrite(RFMSELPIN,LOW);
  result=(SPI.transfer(cmd>>8)<<8) | SPI.transfer(cmd & 0xff);
  digitalWrite(RFMSELPIN,HIGH);
  return result;
}

void setLNAgain(byte level)
{
  // LNA has four gain levels, 0,-6,-14,-20 dB relative to maximum
  rfm_write(0x94a0 | ((level & 3)<<3));
}

void restoreSetting(int address, byte *dest, byte maxval, byte defaultVal)
{
  byte data;
  
  data=EEPROM.read(address);
  if(data>maxval)
  {
    data=defaultVal;
    EEPROM.write(address,data);
  }
  *dest=data;
}

#ifndef sleep_bod_disable() // not present in old versions of AVR toolset
#define sleep_bod_disable() \
do { \
  uint8_t tempreg; \
  __asm__ __volatile__("in %[tempreg], %[mcucr]" "\n\t" \
                       "ori %[tempreg], %[bods_bodse]" "\n\t" \
                       "out %[mcucr], %[tempreg]" "\n\t" \
                       "andi %[tempreg], %[not_bodse]" "\n\t" \
                       "out %[mcucr], %[tempreg]" \
                       : [tempreg] "=&d" (tempreg) \
                       : [mcucr] "I" _SFR_IO_ADDR(MCUCR), \
                         [bods_bodse] "i" (_BV(BODS) | _BV(BODSE)), \
                         [not_bodse] "i" (~_BV(BODSE))); \
} while (0)
#endif

void powerDown()
{
  bitClear(TIMSK1,OCIE1A); // stop timer 1 compare interrupts
  saveSettings();
  rfm_write(0x8201); // shut down RFM12
  glcd.backLight(0);
  digitalWrite(GREENLEDPIN,LOW);
  digitalWrite(REDLEDPIN,LOW);
  digitalWrite(GCDRESETPIN,LOW); // reset display
  bitClear(ADCSRA,ADEN);  // disable ADC
  
  PCMSK1=_BV(PCINT10) | _BV(PCINT13); // unmask pin change interrupts for up/down buttons
  bitSet(PCICR,PCIE1); // enable pin change interrupt

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_bod_disable();
  sleep_cpu();
  // CPU is sleeping
  bitSet(ADCSRA,ADEN); // re-enable ADC
  setup();
}

// pin change interrupt for waking from sleep
ISR(PCINT1_vect)
{
  sleep_disable();
  PCMSK1=0;
  bitClear(PCICR,PCIE1); // disable pin change interrupt
}

