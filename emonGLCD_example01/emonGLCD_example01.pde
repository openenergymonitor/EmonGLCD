//--------------------------------------------------------------------------------------
// GLCD Energy Monitor Display example
//
// All hard library building work done by Jean-Claude Wippler: Jee Labs
// 2010-05-28 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
//
// Energy monitor specific example by Trystan Lea and Glyn Hudson
// OpenEnergyMonitor.org
//--------------------------------------------------------------------------------------
#include <GLCD_ST7565.h>
#include <Ports.h>
#include <RF12.h> // needed to avoid a linker error :(
#include <avr/pgmspace.h>
#include "utility/font_clR6x8.h"
#include "utility/font_clR4x6.h"
#include "utility/font_courB18.h"

GLCD_ST7565 glcd;

// fixed RF12 settings
#define MYNODE 30            //node ID 30 reserved for base station
#define freq RF12_433MHZ     //frequency
#define group 215            //network group 

//########################################################################################################################
//Data Structure to be received 
//########################################################################################################################
typedef struct {
  	  int ct;		// current transformer
	  int supplyV;		// emontx voltage
	} Payload;
	Payload emontx;

int emontx_nodeID;    //node ID of emon tx, extracted from RF datapacket. Not transmitted as part of structure
//###############################################################

unsigned long last;

void setup () {
    rf12_initialize(MYNODE, freq,group);
    
    glcd.begin();
    glcd.backLight(0);
    last = millis();
    
    Serial.begin(9600);
}

void loop () {

   //--------------------------------------------------------------------
    // 1) Receive data from RFM12
    //--------------------------------------------------------------------
    if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {
        emontx=*(Payload*) rf12_data;   
       // emontx_nodeID=rf12_hdr & 0x1F;   //extract node ID from received packet 
        last = millis();
        Serial.println(emontx.ct);
    }
    
   glcd.setFont(font_clR6x8);
   glcd.drawString(0,0,"OpenEnergyMonitor");
   
   glcd.drawString(0,15,"Power:");
   
   
   char str[50];
   //memset(str,NULL,sizeof(str));
   itoa(emontx.ct,str,10);
   strcat(str,"W");
   
   glcd.setFont(font_courB18);
   glcd.drawString(0,30,str);
 
   //char fstr[10]; dtostrf((emontx.temp1/100.0),0,1,fstr); strcat(str,fstr); strcat(str,"C | ");
   //memset(str,NULL,sizeof(str));
   // Time since last update
   glcd.setFont(font_clR6x8);
   int seconds = (int)((millis()-last)/1000.0);
   itoa(seconds,str,10);
   strcat(str,"s");
   glcd.drawString(110,2,str);
   
   glcd.refresh();
   glcd.clear();

}
