//--------------------------------------------------------------------------------------
// Solar PV emonGLCD Display example
// http://openEnergyMonitor.org/emon/emonglcd
//
// For use with emonTx setup with 2CT and voltage monitoring (AC-AC adapter)
// Voltage monitoring enables direction of power flow to be determind (import/export) 
//
// GLCD library by Jean-Claude Wippler: JeeLabs.org
// 2010-05-28 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
//
// By Glyn Hudson OpenEnergyMonitor.org
// for improved display contrast edit line 134 of GLCD_ST7565.cpp to be: st7565_Set_Brightness(0x18);
//--------------------------------------------------------------------------------------

#include <OneWire.h>
#include <DallasTemperature.h>

//JeeLab libraires 
#include <GLCD_ST7565.h>
#include <Ports.h>
#include <RF12.h> // needed to avoid a linker error :(
#include <Wire.h>
#include <RTClib.h>

#include <avr/pgmspace.h>
#include "utility/font_helvB14.h" //big numberical digits 
#include "utility/font_clR6x8.h" //title
#include "utility/font_clR4x6.h" //kwh


//--------------------------------------------------------------------------------------------
// GLCD setup 
//--------------------------------------------------------------------------------------------
    GLCD_ST7565 glcd;
    unsigned long lastLight;
    unsigned long lastTemp;
    boolean switchPress; 
    long int last; 
    
    #define ONE_WIRE_BUS 5      //temperature sensor connection - hardwired 
    const int greenLED=8;       
    const int redLED=9;
    const int switchpin=14;
    const int LDRpin=4;    
    double cval, cval2, cval3, cval4;  //values to calulate smoothing
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// RFM12B Setup
//--------------------------------------------------------------------------------------------
#define MYNODE 20            //node ID 30 reserved for base station
#define freq RF12_433MHZ     //frequency
#define group 210            //network group, must be same as emonTx and emonBase

//########################################################################################################################
//Data Structure to be received - must be the same structure as transmitted by emonTx 
//########################################################################################################################
typedef struct {
  	  double grid;	     //grid in/out (negative if export), real1 on emonTx  
          double gen;	     //solar PV generation real3 on emonTx
          int supplyV;       //supply voltage of emontx 

} Payload;
Payload emontx;      

int emontx_nodeID;    //node ID of emon tx, to be extracted from RF datapacket. Not transmitted as part of structure
int importing;        //flag to indicate import/export
double consuming, gen, grid, whInc, wh_gen, wh_consuming;     //integer variables to store ammout of power currenty being consumed grid (in/out) +gen
unsigned long lwhtime, whtime;                    //used to calculate energy used.
//###############################################################
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// DS18B20 temperature setup - onboard sensor 
//--------------------------------------------------------------------------------------------
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);
  double temp,maxtemp,mintemp;
//--------------------------------------------------------------------------------------------   

//--------------------------------------------------------------------------------------------
// RTC setup
//-------------------------------------------------------------------------------------------- 
   RTC_Millis RTC;
  int yesterday, today; 
//-------------------------------------------------------------------------------------------- 
   
//--------------------------------------------------------------------------------------------
// Setup
//--------------------------------------------------------------------------------------------
void setup () {
    rf12_initialize(MYNODE, freq,group);
    
    glcd.begin();
    glcd.backLight(200); //max 255
    
    RTC.begin(DateTime(__DATE__, __TIME__));
    DateTime now = RTC.now();
    yesterday=now.day();
    
    last = millis();
    
    Serial.begin(9600);
    Serial.println("emonGLCD solar PV monitor");
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
       // emontx_nodeID=rf12_hdr & 0x1F;   //extract node ID from received packet 
        last = millis();
        
     //--------------------------------------------------------------------
     // Perform power and kWhr/d calculations 
     //-------------------------------------------------------------------- 
         grid=emontx.grid;                      //convert to interger
         gen=emontx.gen;                        //convert to integer
        
         
         if (gen<20) gen=0;
         consuming=(emontx.grid+emontx.gen);   //calculate power being consumed from grid in/out (negative when out) and solar gen
         
         if (emontx.gen > consuming) {
            importing=1; //set flag 
            grid= grid*-1;                            //when grid is negative, set to postive. exporting will be indicated by the word 'importing' changing to 'exporting' on the LCD 
         }
            else importing=0;
            
          //--------------------------------------------------
          // kwh increment calculation
          // 1) find out how much time there has been since the last measurement of power
          //--------------------------------------------------
          lwhtime = whtime;
          whtime = millis();
          whInc = gen * ((whtime-lwhtime)/3600000.0);
          wh_gen=wh_gen+whInc;
          
          whInc= consuming *((whtime-lwhtime)/3600000.0);
          wh_consuming=wh_consuming+whInc;
     //---------------------------------------------------------------------- 
    
        Serial.print(grid);
        Serial.print("  ");
        Serial.print(gen);
        Serial.print("  ");
        Serial.print(consuming);
        Serial.print("  ");
        Serial.println(temp);
    } 
            //--------------------------------------------------------------------
            // Display data on GLCD
            //-------------------------------------------------------------------- 
                glcd.drawLine(64, 0, 64, 64, WHITE);      //top vertical line
                glcd.drawLine(0, 32, 128, 32, WHITE);     //middle horizontal line 
                
                char str[50];    //variable to store conversion 
                
                glcd.setFont(font_clR6x8);             
                glcd.drawString_P(12,0,PSTR("using"));
                glcd.drawString_P(69,0,PSTR("solar PV"));
                glcd.drawString_P(85,33,PSTR("room"));
   
                if (importing=1) glcd.drawString_P(2,36,PSTR("importing"));
                   else glcd.drawString_P(2,36,PSTR("exporting"));
   

               glcd.setFont(font_helvB14);  //big bold font
                 
                cval = cval + (consuming - cval)*0.50;
                itoa((int)cval,str,10);
                strcat(str,"w");   
               glcd.drawString(3,9,str);     //ammount of power currently being used 
               
               cval2 = cval2 + (gen - cval2)*0.5;
                itoa((int)cval2,str,10);
                strcat(str,"w");   
               glcd.drawString(71,9,str);    //pv
               
               cval3 = cval3 + (grid - cval3)*0.5;
                itoa((int)cval3,str,10);
                strcat(str,"w");   
               glcd.drawString(3,45,str);    //importing / exporting
               
               dtostrf(temp,0,1,str); 
               strcat(str,"C");
               glcd.drawString(74,42,str);   
               
               glcd.setFont(font_clR4x6);   //small font - Kwh
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
  //Change color of LED on top of emonGLCD, red if consumption exceeds gen, ornage if gen almost (by 100W) meets consumption, or green if gen is greater than consumption 
  //-------------------------------------------------------------------- 
   
   if (gen > consuming)    //show green LED       
     digitalWrite(greenLED, HIGH);    
     digitalWrite(redLED, LOW); 
   
      if ((gen > (consuming -30) ) && (consuming > 50)) {  ///if generation is almost bigger then consumption then show amber 
   digitalWrite(greenLED, HIGH);    
   digitalWrite(redLED, HIGH);    
   }
   
   else{ // generation is lower than consumption by at least 200W
   digitalWrite(redLED, HIGH);    // set the green LED off
   digitalWrite(greenLED, LOW);    // set the red LED on
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
   

   
} //end loop
//--------------------------------------------------------------------------------------------
