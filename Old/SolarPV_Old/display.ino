//GLCD fonts - part of GLCD lib
#include "utility/font_helvB14.h" 	//big numberical digits 
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
   
  if (importing==1) glcd.drawString_P(2,34,PSTR("importing")); else glcd.drawString_P(2,34,PSTR("exporting"));
  
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
  glcd.drawString(3,42,str);    		//importing / exporting
               
  dtostrf(temp,0,1,str); 
  strcat(str,"C");
  glcd.drawString(74,42,str);   
               
  glcd.setFont(font_clR4x6);   		//small font - Kwh
  dtostrf((wh_consuming[0]/1000),0,1,str);
  strcat(str,"Kwh today");  
  glcd.drawString(6,26,str);
               
  dtostrf((wh_gen[0]/1000),0,1,str); 
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
  
  

  if (((millis()-last_emontx)<20000) && ((millis()-last_emonbase)<20000)) {
    
    DateTime now = RTC.now();
  glcd.drawString_P(5,58,PSTR("Time:"));
  char str2[5];
  itoa((int)now.hour(),str,10);
  if  (now.minute()<10) strcat(str,":0"); else strcat(str,":"); 
  itoa((int)now.minute(),str2,10);
  strcat(str,str2); 
  glcd.drawString(28,58,str); 
  }
  else
    if ((millis()-last_emontx)>20000) glcd.drawString_P(2,58,PSTR("--emonTx fail--"));
      else glcd.drawString_P(1,58,PSTR("-emonBase fail-"));

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
  if  (now.minute()<10) strcat(str,":0"); else strcat(str,":");
  itoa((int)now.minute(),str2,10);
  strcat(str,str2); 
               
  glcd.setFont(font_helvB14);  		//big bold font   
  glcd.drawString(2,10,str);  

  glcd.refresh();
  
  delay(2000); 
  
}

void draw_history()
{
  glcd.clear;
  glcd.fillRect(0,0,128,64,0);
  
  char str[50]; 
  
  glcd.setFont(font_clR6x8);
  glcd.drawString_P(40,0,PSTR("History"));
  
  glcd.setFont(font_clR4x6);   	

  glcd.drawString_P(2,16,PSTR("Today"));
  glcd.drawString_P(2,23,PSTR("Yesterday"));
  glcd.drawString_P(2,30,PSTR("2 days ago"));
  glcd.drawString_P(2,37,PSTR("3 days ago"));
  glcd.drawString_P(2,44,PSTR("4 days ago"));
  glcd.drawString_P(2,51,PSTR("5 days ago"));
  glcd.drawString_P(2,58,PSTR("6 days ago"));
  
  // draw grid consumption history
  glcd.setFont(font_clR4x6);   		
  glcd.drawString_P(50,9,PSTR("Power"));


  dtostrf((wh_consuming[0]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52,16,str);
  
  dtostrf((wh_consuming[1]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52,23,str);
  
  dtostrf((wh_consuming[2]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52,30,str);
  
  dtostrf((wh_consuming[3]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52,37,str);
  
  dtostrf((wh_consuming[4]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52,44,str);
  
  dtostrf((wh_consuming[5]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52,51,str);
  
  dtostrf((wh_consuming[6]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52,58,str);
  
  
  // draw solar PV generation history
  glcd.setFont(font_clR4x6);   		
  glcd.drawString_P(78,9,PSTR("PV"));

  dtostrf((wh_gen[0]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(76,16,str);
  
  dtostrf((wh_gen[1]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(76,23,str);
  
  dtostrf((wh_gen[2]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(76,30,str);
  
  dtostrf((wh_gen[3]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(76,37,str);
  
  dtostrf((wh_gen[4]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(76,44,str);
  
  dtostrf((wh_gen[5]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(76,51,str);
  
  dtostrf((wh_gen[6]/1000),0,1,str);
  glcd.setFont(font_clR4x6);
  glcd.drawString(76,58,str);
  
  
  
  glcd.refresh();
  //delay(20000);    //20 sec delay
}

void backlight_control()
{
   int LDR = analogRead(LDRpin);                    // Read the LDR Value so we can work out the light level in the room.
                                                      // GLCD settings
   int LDRbacklight = map(LDR, 0, 1023, 50, 250);    // Map the data from the LDR from 0-1023 (Max seen 1000) to var GLCDbrightness min/max
   LDRbacklight = constrain(LDRbacklight, 0, 255);   // Constrain the value to make sure its a PWM value 0-255
                                                 

  //--------------------------------------------------------------------
  // Turn off backlight and indicator LED's between 11pm and 6am
  //-------------------------------------------------------------------- 
  DateTime now = RTC.now();
  if (now.hour()>0) int hour = now.hour();                    //get hour digit in 24hr from software RTC
 
  if ((hour > 22) ||  (hour < 5) && ((millis()-last_emonbase)<20000)){       // turn off backlight between 11pm and 6am
    night=1; 
    glcd.backLight(0);
  } else {
    night=0; 
    glcd.backLight(LDRbacklight);  
  }
}

//--------------------------------------------------------------------
//Change color of LED on top of emonGLCD, red if consumption exceeds gen or green if gen is greater than consumption 
//-------------------------------------------------------------------- 
void led_control()
{
//--------------------------------------------------------------------
//Change color of LED on top of emonGLCD, red if consumption exceeds gen or green if gen is greater than consumption 
//-------------------------------------------------------------------- 
   int PWRleds= map(cval3, 0, 4000, 0, 255);     // Map importing value from 0-4Kw (LED brightness - cval3 is the smoothed grid value - see display above 
   PWRleds = constrain(PWRleds, 0, 255);             // Constrain the value to make sure its a PWM value 0-255
   
    if ((gen>0) && (night==0)) {
      if (cval2 > cval) {                //show green LED when gen>consumption cval are the smooth curve values  
	analogWrite(redLED, 0);         
	analogWrite(greenLED, PWRleds);    
        
      } else {                              //red if consumption>gen
        analogWrite(greenLED, 0); 
	analogWrite(redLED, PWRleds);   
           
      }
    } else{                                 //Led's off at night and when solar PV is not generating
      analogWrite(redLED, 0);
      analogWrite(greenLED, 0);
    }
}
