#include "utility/font_helvB18.h"
#include "utility/font_helvB14.h"
#include "utility/font_helvB12.h"
#include "utility/font_clR4x6.h"
#include "utility/font_clR6x8.h"

//------------------------------------------------------------------
// Draws a page showing Temperature/Humidity (based on draw_solar_page)
//------------------------------------------------------------------
void draw_th_page(double temp, double mintemp, double maxtemp, double hum,double minhum,double maxhum,double hour, double minute, unsigned long last_emonbase,int nodeid)
{
  
  int MINTEMP = -5;
  int MAXTEMP = 35;
  byte imageindex;
  
  glcd.clear();
  glcd.fillRect(0,0,128,64,0);
  
  glcd.drawLine(66, 0, 66, 64, WHITE);      //top vertical line

  //variables to store conversions
  char str[50];
  char str2[5];

  // Print time of last update from EmonBase/EmonPi (need RTC on emonBase)

//  if ((millis()-last_emonbase)>12000)
//  {
//    // small font
//    glcd.setFont(font_clR4x6);
//    itoa((millis()-last_emonbase)/60000, str, 10);
//    strcat(str,"m base!");
//    glcd.drawString(69,0,str);
//  }
  
  //** Relative Humidity **//

  // Water icon 
  glcd.drawBitmap(53,0,icon_water_12x12,16,16,1);

  //big bold font
  glcd.setFont(font_helvB18);
  
  dtostrf(hum,0,1,str); 
  strcat(str,"%");   
  
  glcd.drawString(0,16,str);

  // Minimum and maximum relative humidity
  glcd.setFont(font_clR4x6);
  itoa((int)minhum,str,10);
  strcat(str,"%");
  glcd.drawString_P(2,38,PSTR("min"));
  glcd.drawString(16,38,str);
  itoa((int)maxhum,str,10); 
  strcat(str,"%");
  glcd.drawString_P(31,38,PSTR("max"));
  glcd.drawString(45,38,str);
  
  //** Temperature **//  

  //big bold font
  glcd.setFont(font_helvB18);
               
  dtostrf(temp,0,1,str); 
  strcat(str,"c");
  if (temp>MAXTEMP)
  {
    imageindex=5;
  }
  else
  {
    if (temp<MINTEMP) imageindex=0; else imageindex=int((temp-MINTEMP)/(MAXTEMP-MINTEMP)*5-0.5);
  }
  glcd.drawBitmap(120,0,icon_heating_8x16[imageindex],8,16,1);
  glcd.drawString(70,16,str);   

  glcd.setFont(font_clR4x6);   		//small font - Kwh

  // Minimum and maximum temperatures
  itoa((int)mintemp,str,10);
  strcat(str,"c");
  glcd.drawString_P(70,38,PSTR("min"));
  glcd.drawString(84,38,str);
  itoa((int)maxtemp,str,10); 
  strcat(str,"c");
  glcd.drawString_P(99,38,PSTR("max"));
  glcd.drawString(113,38,str);

  // Current time
  glcd.drawString_P(5,58,PSTR("Time:"));
  itoa((int)hour,str,10);
  if  (minute<10) strcat(str,":0"); else strcat(str,":"); 
  itoa((int)minute,str2,10);
  strcat(str,str2); 
  glcd.drawString(28,58,str); 

  // Print Node ID to display for reference
  itoa((int)nodeid,str,10);
  strcat(str,"N"); 
  glcd.drawString(69,58,str);

}
