//--------------------------------------------------------------------------------------
// emonGLCD Solar PV monitor example

// emonGLCD documentation http://openEnergyMonitor.org/emon/emonglcd
// solar PV monitor build documentation: http://openenergymonitor.org/emon/solarpv

// For use with emonTx setup with 2CT one monitoring consumption and the other monitoring gen
// RTC to reset Kwh counters at midnight is implemented is software. Correct time (from computer) is transfered to emonGLCD on code upload. Time lost if emonGLCD power is lost. Time can be set to midnight by holding push switch on top-left handside for 5s. In future emonGLCD should be able top receive time from NanodeRF NTP

// this sketch is currently setup for type 1 solar PV monitoring where CT's monitor generation and consumption separately
// to use this sketch for type 2 solar PV monitoring where CT's monitor consumption and grid import/export using an AC-AC adapter to detect current flow direction 
//    -change line 69-70, 160-164 - see comments in on specific lines. See Solar PV documentation for explination 

// GLCD library by Jean-Claude Wippler: JeeLabs.org
// 2010-05-28 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
//
// By Glyn Hudson OpenEnergyMonitor.org 
// http://openenergymonitor.org/emon/license

//-------------------------------------------------------------------------------------------------------------------------
// Note: for improved display contrast edit line 134 of GLCD_ST7565.cpp to be: st7565_Set_Brightness(0x18);
//-------------------------------------------------------------------------------------------------------------------------

#include <OneWire.h>		  	//http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>   	//http://milesburton.com/Dallas_Temperature_Control_Library

//JeeLab libraires - 			http://jeelabs.net/projects/cafe/wiki
#include <GLCD_ST7565.h>
#include <Ports.h>
#include <RF12.h> 			// needed to avoid a linker error :(
#include <Wire.h>
#include <RTClib.h>

#include <avr/pgmspace.h>
#include "utility/font_helvB14.h" 	//big numberical digits 
#include "utility/font_clR6x8.h" 	//title
#include "utility/font_clR4x6.h" 	//kwh


//--------------------------------------------------------------------------------------------
// emonGLCD setup 
//--------------------------------------------------------------------------------------------
    GLCD_ST7565 glcd;
    unsigned long lastLight;
    unsigned long lastTemp;
    boolean switchPress; 
    long int last; 
    
    #define ONE_WIRE_BUS 5      	//temperature sensor connection - hard wired 
    const int greenLED=8;               //Green tri-color LED
    const int redLED=9;                 //Red tri-color LED
    const int switchpin=15;		//digital pin of onboard pushswitch 
    const int LDRpin=4;    		//analog pin of onboard lightsensor 
    double cval, cval2, cval3, cval4;   //values to calulate smoothing
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// RFM12B Setup
//--------------------------------------------------------------------------------------------
#define MYNODE 20            //Should be unique on network, node ID 30 reserved for base station
#define freq RF12_433MHZ     //frequency - match to same frequency as RFM12B module (change to 868Mhz or 915Mhz if appropriate)
#define group 210            //network group, must be same as emonTx and emonBase

//###################################################################################################################################################
//Data Structure to be received - must be the same structure as transmitted by emonTx, variable names can be different but order and type must be trhe same
//###################################################################################################################################################
typedef struct {
           int gen;	     //solar PV generation 	  
          int consuming;     //consumption monitoring CT for type 1 solar PV monitoring - comment out for type 2
	  //int grid;        //CT monitoring grid import/output - un-comment for type 2 solar PV monitoring. Must be used with AC-AC plug-ing adapter to determine current flow direction.     
          int supplyV;       //supply voltage of emonTx in mV */
} Payload;
Payload emontx;      

int emontx_nodeID;    	      //node ID of emon tx, to be extracted from RF datapacket. Not transmitted as part of structure - old used when multiple emonTx's are on the same network. 
int importing, night;        //flag to indicate import/export
double consuming, gen, grid, whInc, wh_gen, wh_consuming;     //integer variables to store ammout of power currenty being consumed grid (in/out) +gen
unsigned long lwhtime, whtime;                    	      //used to calculate energy used per day (kWh/d)
//###################################################################################################################################################
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// DS18B20 temperature setup - onboard sensor 
//--------------------------------------------------------------------------------------------
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);
  double temp,maxtemp,mintemp;
//--------------------------------------------------------------------------------------------   

//--------------------------------------------------------------------------------------------
// Software RTC setup
//-------------------------------------------------------------------------------------------- 
   RTC_Millis RTC;
  int yesterday, today; 
  long int pushcount;
//-------------------------------------------------------------------------------------------- 
   
//--------------------------------------------------------------------------------------------
// Setup
//--------------------------------------------------------------------------------------------
void setup () {
    rf12_initialize(MYNODE, freq,group);
    
    glcd.begin();
    glcd.backLight(200); //max 255
    
    RTC.begin(DateTime(__DATE__, __TIME__));	//load time and time from computer into sofware RTC
    DateTime now = RTC.now();
    yesterday=now.day();
    
    last = millis();
    
    Serial.begin(9600);
    Serial.println("emonGLCD solar PV monitor - gen and use");
    Serial.println("openenergymonitor.org");
    Serial.print("Node: "); 
    Serial.print(MYNODE); 
    Serial.print(" Freq: "); 
     if (freq == RF12_433MHZ) Serial.print("433Mhz");
     if (freq == RF12_868MHZ) Serial.print("868Mhz");
     if (freq == RF12_915MHZ) Serial.print("915Mhz"); 
   Serial.print(" Network: "); 
   Serial.print(group);
   Serial.println("  ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);
    
   pinMode(greenLED, OUTPUT); 
   pinMode(redLED, OUTPUT);  
  
   sensors.begin();                    //start up the DS18B20 temp sensor onboard  
   sensors.requestTemperatures();
   temp=(sensors.getTempCByIndex(0));  //get inital temperture reading
   mintemp=temp; maxtemp=temp;          //reset min and max
    
}
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// Loop
//--------------------------------------------------------------------------------------------
void loop () {

    //--------------------------------------------------------------------
    // Receive data from RFM12
    //--------------------------------------------------------------------
    if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) 
    {
        emontx=*(Payload*) rf12_data;   
       // emontx_nodeID=rf12_hdr & 0x1F;   //extract node ID from received packet - only used when multiple emonTx's are on the same network 
        last = millis();
        
     //--------------------------------------------------------------------
     // Perform power and kWhr/d calculations 
     //-------------------------------------------------------------------- 
         gen=emontx.gen;  if (gen<100) gen=0;	//remove noise offset 
	 consuming=emontx.consuming; 		//for type 1 solar PV monitoring
	 //grid=emontx.grid; 			//for type 2 solar PV monitoring                     
         
         grid=emontx.consuming - gen;		//type 1 solar PV monitoring
	 //consuming=gen + emontx.grid; 	//type 2 solar PV monitoring - grid should be positive when importing and negastive when exporting. Flip round CT cable clap orientation if not
         
         if (gen > consuming) {
            importing=0; 			//set importing flag 
            grid= grid*-1;			//set grid to be positive - the text 'importing' will change to 'exporting' instead. 
         }
            else importing=1;
            
          //--------------------------------------------------
          // kWh calculation
          //--------------------------------------------------
          lwhtime = whtime;
          whtime = millis();
          whInc = gen * ((whtime-lwhtime)/3600000.0);
          wh_gen=wh_gen+whInc;
          
          whInc= consuming *((whtime-lwhtime)/3600000.0);
          wh_consuming=wh_consuming+whInc;
     //---------------------------------------------------------------------- 
    
        Serial.print(gen);
        Serial.print("  ");
        Serial.print(consuming);
        Serial.print("  ");
	Serial.print(grid);
        Serial.print("  ");
        Serial.println(temp);
    } 
            //--------------------------------------------------------------------
            // Display data on GLCD
            //-------------------------------------------------------------------- 
                glcd.drawLine(64, 0, 64, 64, WHITE);      //top vertical line
                glcd.drawLine(0, 32, 128, 32, WHITE);     //middle horizontal line 
                
                char str[50];    			 //variable to store conversion 
                
                glcd.setFont(font_clR6x8);             
                glcd.drawString_P(12,0,PSTR("using"));
                glcd.drawString_P(69,0,PSTR("solar PV"));
                glcd.drawString_P(85,33,PSTR("room"));
   
                if (importing==1) glcd.drawString_P(2,36,PSTR("importing"));
                   else glcd.drawString_P(2,36,PSTR("exporting"));
   

               glcd.setFont(font_helvB14);  		//big bold font
                 
                cval = cval + (consuming - cval)*0.50;
                itoa((int)cval,str,10);
                strcat(str,"w");   
               glcd.drawString(3,9,str);     		//ammount of power currently being used 
               
               cval2 = cval2 + (gen - cval2)*0.5;
                itoa((int)cval2,str,10);
                strcat(str,"w");   
               glcd.drawString(71,9,str);    		//pv
               
               cval3 = cval3 + (grid - cval3)*0.5;
                itoa((int)cval3,str,10);
                strcat(str,"w");   
               glcd.drawString(3,45,str);    		//importing / exporting
               
               dtostrf(temp,0,1,str); 
               strcat(str,"C");
               glcd.drawString(74,42,str);   
               
               glcd.setFont(font_clR4x6);   		//small font - Kwh
               dtostrf((wh_consuming/1000),0,1,str);
                strcat(str,"Kwh today");  
               glcd.drawString(6,26,str);
               
               dtostrf((wh_gen/1000),0,1,str); 
                strcat(str,"Kwh today");  
               glcd.drawString(71,26,str);
                             
               itoa((int)mintemp,str,10);
               strcat(str,"C");
               glcd.drawString_P(68,58,PSTR("min"));
               glcd.drawString(82,58,str);
               
               itoa((int)maxtemp,str,10); 
               strcat(str,"C");
               glcd.drawString_P(97,58,PSTR("max"));
               glcd.drawString(111,58,str);
               
               
               
               int seconds = (int)((millis()-last)/1000.0);
                 if (seconds>10) 
                 glcd.drawString_P(32,58,PSTR("RF fail"));

              delay(200);
              
              glcd.refresh();
              glcd.clear();
              
  //--------------------------------------------------------------------
  //work out if midnight has passsed - if it has then reset wh_incremens 
  //-------------------------------------------------------------------- 
    DateTime now = RTC.now();
    today=now.day();
    if (today > yesterday){
      wh_gen=0; 
      wh_consuming=0;
    }
    yesterday=now.day()  ;
  //--------------------------------------------------------------------
  
  
  //--------------------------------------------------------------------
  //Change color of LED on top of emonGLCD, red if consumption exceeds gen or green if gen is greater than consumption 
  //-------------------------------------------------------------------- 
 if ((gen>0) && (night==0))  {
   if (gen > consuming)  {  //show green LED when gen>consumption   
     digitalWrite(greenLED, HIGH);    
     digitalWrite(redLED, LOW); 
   }
   
   else{ //red if consumption>gen
   digitalWrite(redLED, HIGH);   
   digitalWrite(greenLED, LOW);    
   }
 }
 else{ //Led's off at night and when solar PV is not generating
   digitalWrite(redLED, LOW);
   digitalWrite(greenLED, LOW);
 }

    //--------------------------------------------------------------------
    // Get temperatue from onboard sensor - every 10s
    //--------------------------------------------------------------------
    if (millis()>lastTemp+10000){
       sensors.requestTemperatures();
       temp=(sensors.getTempCByIndex(0));
       lastTemp=millis();
    }
    
    if (temp> maxtemp) maxtemp=temp;
    if (temp<mintemp) mintemp=temp;
   //--------------------------------------------------------------------
   
   
   //--------------------------------------------------------------------
    // Push and hold switch to reset time to midnight 
    //--------------------------------------------------------------------
     pushcount=millis();
     while (digitalRead(switchpin) == TRUE){
       
       
     
       
            glcd.setFont(font_clR6x8);
           glcd.drawString_P(2,0,PSTR("Current Time:"));
               
               DateTime now = RTC.now();
                char str2[5];
                itoa((int)now.hour(),str,10);
                strcat(str,":");   
                itoa((int)now.minute(),str2,10);
                strcat(str,str2); 
               
               glcd.setFont(font_helvB14);  		//big bold font   
               glcd.drawString(2,10,str);  
               
               glcd.setFont(font_clR4x6);
               glcd.drawString_P(0,30,PSTR("Hold button for 5 seconds to   set time to midnight"));
               int holdtime=(millis()-pushcount)/1000;
               Serial.println(holdtime);
               
               
               glcd.refresh();
               glcd.clear;
               
               delay(200);
  
       if (((pushcount+5000)<millis())   )     			//when push button is held for 5's 
         {
           RTC.begin(DateTime("Dec  8 2011" , "00:00:00")); 	//reset RTC to midnight 
           wh_consuming=0;					//reset kWh
           wh_gen=0;
           maxtemp=0;						//reset temperature min/max
           mintemp=0;
            
           glcd.setFont(font_clR6x8);
           glcd.drawString_P(12,50,PSTR("Midnight Set"));
           glcd.clear;
           glcd.refresh();
           delay(2000);
         }
     }
     
     //--------------------------------------------------------------------
     
   //--------------------------------------------------------------------
   // Turn off backlight and indicator LED's between 12pm and 6am
   //--------------------------------------------------------------------

   int hour=now.hour();                  //get hour digit in 24hr from software RTC
   
   if ((hour > 23) ||  (hour < 6)) {
     night=1; 
     glcd.backLight(0);

   }
   else {
     night=0; 
     glcd.backLight(200); 
   }  
   
  //--------------------------------------------------------------------
   
} //end loop
//--------------------------------------------------------------------------------------------
