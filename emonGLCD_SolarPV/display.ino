//GLCD fonts - part of GLCD lib
#include "utility/font_helvB14.h"   //big numberical digits 
#include "utility/font_clR6x8.h" 	//title
#include "utility/font_clR4x6.h" 	//kwh

double cval, cval2, cval3, cval4;   //values to calulate smoothing

void draw_main_screen()
{
  glcd.clear();
  glcd.fillRect(0,0,128,64,0);
  
  glcd.drawLine(64, 0, 64, 64, WHITE);      //top vertical line
  glcd.drawLine(0, 32, 128, 32, WHITE);     //middle horizontal line 
                
  char str[50];    			 //variable to store conversion 
  glcd.setFont(font_clR6x8);             
  glcd.drawString_P(12,0,PSTR("using"));
  glcd.drawString_P(69,0,PSTR("solar PV"));
  glcd.drawString_P(85,33,PSTR("room"));
   
//  if (importing==1) glcd.drawString_P(2,36,PSTR("importing")); else glcd.drawString_P(2,36,PSTR("exporting")); 
  if (importing==1) glcd.drawString_P(2,34,PSTR("importing")); else glcd.drawString_P(2,34,PSTR("exporting")); // - RW - Moved up to match Room on other side
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
//  glcd.drawString(3,45,str);    		//importing / exporting
  glcd.drawString(3,42,str);    		//importing / exporting - RW - Moved up to match Temp on other side
               
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

                                                         // RW - NEW - 
                                                         // Added time under import / export as there is now room after moving these feilds And we dont have buttons working.
  DateTime now = RTC.now();
  glcd.drawString_P(5,58,PSTR("Time:"));
  char str2[5];
  itoa((int)now.hour(),str,10);
  strcat(str,":");   
  itoa((int)now.minute(),str2,10);
  strcat(str,str2); 
  glcd.drawString(28,58,str); 
  
  int LDR=analogRead(LDRpin);                             // RW - See if we can use some data from the LDR
  itoa((int)LDR,str,10);
  glcd.drawString(49,58,str); 

//  if ((millis()-last_emontx)>10000) glcd.drawString_P(32,58,PSTR("RF fail"));
  if ((millis()-last_emontx)>10000) glcd.drawString_P(01,58,PSTR("--- RF fail ---")); // RW - Not sure how to blank time to display error

  glcd.refresh();
                    
}

void draw_page_two()
{
  glcd.clear;
  glcd.fillRect(0,0,128,64,0);
  
  glcd.setFont(font_clR6x8);
  glcd.drawString_P(2,0,PSTR("Current Time:"));
               
  DateTime now = RTC.now();
  char str[20];
  char str2[5];
  itoa((int)now.hour(),str,10);
  strcat(str,":");   
  itoa((int)now.minute(),str2,10);
  strcat(str,str2); 
               
  glcd.setFont(font_helvB14);  		//big bold font   
  glcd.drawString(2,10,str);  

  glcd.refresh();
  
}

void backlight_control()
{
  //--------------------------------------------------------------------
  // Turn off backlight and indicator LED's between 12pm and 6am
  //-------------------------------------------------------------------- 
  DateTime now = RTC.now();
  int hour = now.hour();                  //get hour digit in 24hr from software RTC
   
  if ((hour > 22) ||  (hour < 6)) {       // RW - Changed from 23 and 7 as we go to be early ad get up early.
    night=1; 
    glcd.backLight(0);
  } else {
    night=0; 
    glcd.backLight(150);                  // RW - Changed from 200 to 150.
  }
}

//--------------------------------------------------------------------
//Change color of LED on top of emonGLCD, red if consumption exceeds gen or green if gen is greater than consumption 
//-------------------------------------------------------------------- 
void led_control()
{
  if ((gen>0) && (night==0)) {
    if (gen > consuming) {  //show green LED when gen>consumption   
      analogWrite(greenLED, 200);    
      analogWrite(redLED, 0); 
    } else { //red if consumption>gen
//      digitalWrite(redLED, HIGH);   
//      digitalWrite(greenLED, LOW);  
      analogWrite(redLED, 200);   
      analogWrite(greenLED, 0);    
    }
  } else{ //Led's off at night and when solar PV is not generating
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, LOW);
  }
}
