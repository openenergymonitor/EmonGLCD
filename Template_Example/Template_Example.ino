/*

  EmonGLCD Template based display specification (in development)

  Contributors: Glyn Hudson, Trystan Lea
  Icons implementation by DrsDre
  GLCD library by Jean-Claude Wippler: JeeLabs.org

  Part of the: openenergymonitor.org project
  Licenced under GNU GPL V3
  
*/ 

#include <JeeLib.h>
#include <GLCD_ST7565.h>
#include <avr/pgmspace.h>
GLCD_ST7565 glcd;

unsigned long fast_update;

// Fixed values for this example
double temp = 17.4 ,maxtemp = 19 ,mintemp = 16;
int hour = 23, minute = 43;
double use = 252, usekwh = 2.5;
double gen = 1760, genkwh = 4.8;

void setup()
{
  glcd.begin(0x20);
  glcd.backLight(200);
}

void loop()
{
  //--------------------------------------------------------------------------------------------
  // Display update every 200ms
  //--------------------------------------------------------------------------------------------
  if ((millis()-fast_update)>200)
  {
    fast_update = millis();
    
    int S1 = digitalRead(15);
    int S2 = digitalRead(16);
    int S3 = digitalRead(19);

    if (S1==1)
    {
      // powerstr, power, energystr, kwh
      draw_power_page( "POWER" ,use, "USE", usekwh);

      // temp, mintemp, maxtemp, hour, minute
      draw_temperature_time_footer(temp, mintemp, maxtemp, hour,minute);
      
      glcd.refresh();
    }
    else if (S2==1)
    {
      // powerstr, power, energystr, kwh
      draw_power_page( "SOLAR" ,gen, "GEN", genkwh);

      // temp, mintemp, maxtemp, hour, minute
      draw_temperature_time_footer(temp, mintemp, maxtemp, hour,minute);
      
      glcd.refresh();
    }
    else
    {
      // use, usekwh, gen, maxgen, genkwh, temp, mintemp, maxtemp, hour, minute
      draw_solar_page(use, usekwh, gen, 2050, genkwh, temp, mintemp, maxtemp, 12,43,0,0);
      glcd.refresh();
    }
    
  } 
}
