#include "utility/font_helvB24.h"
#include "utility/font_helvB14.h"
#include "utility/font_helvB12.h"
#include "utility/font_clR4x6.h"
#include "utility/font_clR6x8.h"
#include "utility/font_helvB08.h"
#include "utility/font_lubB12.h"



int MINTEMP = -25;
int MAXTEMP = 60;

//------------------------------------------------------------------
// Draws a page showing a single power and energy value in big font
//------------------------------------------------------------------
void draw_power_page(char* powerstr, double powerval, char* energystr,  double energyval)
{
  glcd.clear();
  glcd.fillRect(0, 0, 128, 64, 0);

  char str[50];    			 //variable to store conversion
  glcd.setFont(font_clR6x8);
  strcpy(str, powerstr);
  strcat(str, " now:");
  glcd.drawString(0, 0, str);
  strcpy(str, energystr);
  strcat(str, " TODAY:");
  glcd.drawString(0, 38, str);

  // power value
  glcd.setFont(font_helvB24);
  itoa((int)powerval, str, 10);
  strcat(str, "w");
  glcd.drawString(3, 9, str);

  // kwh per day value
  glcd.setFont(font_clR6x8);
  if (energyval < 10.0) dtostrf(energyval, 0, 1, str); else itoa((int)energyval, str, 10);
  strcat(str, "kWh");
  glcd.drawString(85, 38, str);
}

void draw_temperature_footer(double temp, double mintemp, double maxtemp) {

  char str[10];
  char str2[5];
  glcd.setFont(font_helvB12);

  if (temp < MINTEMP ) {
    strcpy (str, "--.-C");
  } else {
    dtostrf(temp, 0, 1, str);
    strcat(str, "C");
  }

  glcd.drawString(0, 50, str);

  // Minimum and maximum GLCD temperature
  glcd.setFont(font_clR4x6);

  if (mintemp < MINTEMP || maxtemp < MINTEMP ) {
    strcpy (str, "--C");
  }

  if (mintemp >= MINTEMP) {
    itoa((int)mintemp, str, 10);
    strcat(str, "C");
  }

  glcd.drawString_P(46, 51, PSTR("MIN"));
  glcd.drawString(62, 51, str);

  if (maxtemp >= MINTEMP) {
    itoa((int)maxtemp, str, 10);
    strcat(str, "C");
  }
  glcd.drawString_P(46, 59, PSTR("MAX"));
  glcd.drawString(62, 59, str);

}



void draw_date_footer(DateTime now) {

  char str[10];
  char str2[5];

  // Current date
  glcd.setFont(font_helvB12);

  if (now.day() < 10) {
    strcpy(str, "0") ;
    itoa((int)now.day(), str2, 10);
    strcat(str, str2) ;
  } else {
    itoa((int)now.day(), str, 10);
  }

  strcat(str, "/");

  if (now.month() < 10) {
    strcat(str, "0") ;
  }

  itoa((int)now.month(), str2, 10);
  strcat(str, str2);
  strcat(str, "/");

  itoa((int)(now.year() - 2000), str2, 10);
  strcat(str, str2);
  strcat (str, " ") ;
  glcd.drawString(0, 50, str);

}

//------------------------------------------------------------------
// Draws a footer showing GLCD temperature and the date / time
//------------------------------------------------------------------
void draw_temperature_time_footer(double temp, double mintemp, double maxtemp, DateTime now)
{

  char str[10];
  char str2[5];

  /*toggleTemp is incremented every call (approx 200ms) ,
    when it reaches 127 display temperatures, when it rolls over to 0 display date
    resulting in display toggling between both every 25.4 secs
  */
  static byte toggleTemp = 0 ;

  glcd.drawLine(0, 47, 128, 47, WHITE);     //middle horizontal line


#ifdef EMONTX
  //Only display the temp & the time - no way to get a date over the air
  draw_temperature_footer(temp, mintemp, maxtemp) ;
#else

  if (toggleTemp < 127 ) {
    draw_date_footer(now) ;
  } else {
    draw_temperature_footer(temp, mintemp, maxtemp) ;
  }

  toggleTemp ++ ;

#endif

  //for both EMONTX & EMONPI operation, display the current time
  glcd.setFont(font_helvB12);

  //glcd.drawString_P(5, 58, PSTR("Time:"));
  if (now.hour() < 10) {
    strcpy(str, "0") ;
    itoa((int)now.hour(), str2, 10);
    strcat(str, str2) ;
  } else {
    itoa((int)now.hour(), str, 10);
  }

  glcd.drawString(82, 50, str);

  if (now.second() % 2 == 0) {
    strcpy (str, ":") ;
  } else {
    strcpy (str, "") ;
  }
  glcd.drawString(98, 50, str);
  strcpy (str, "") ;

  if  (now.minute() < 10) strcat(str, "0"); else strcat(str, " ");


  itoa((int)now.minute(), str2, 10);
  strcat(str, str2);
  glcd.drawString(102, 50, str);

  /*
    // Time
    strcpy (str, "") ;
    char str2[5];
    itoa((int)hour, str2, 10);
    if  (hour < 10) {
      str[0] = '0';
      str[1] = '\0';
      strcat (str, str2) ;
    } else {
      strcpy (str, str2) ;
    }

    if  (minute < 10) strcat(str, ": 0"); else strcat(str, ": ");
    itoa((int)minute, str2, 10);
    strcat(str, str2);
  */
  //glcd.setFont(font_helvB12);
  //glcd.drawString(82, 50, str);

}

//------------------------------------------------------------------
// Draws the Solar import/export page
//------------------------------------------------------------------
void draw_solar_page(double use, double usekwh, double gen, double maxgen, double genkwh, double intemp, double outemp, DateTime now, unsigned long last_emontx, unsigned long last_emonbase)
{


  byte imageindex;

  int importing = 0;
  if (use > gen) importing = 1;
  double grid = use - gen;

  glcd.clear();
  glcd.fillRect(0, 0, 128, 64, 0);

  //glcd.drawLine(64, 0, 64, 64, WHITE);      //top vertical line
  //glcd.drawLine(0, 18, 128, 18, WHITE);     //top horizontal line
  glcd.drawLine(0, 26, 128, 26, WHITE);     //2nd horizontal line

  glcd.drawLine(0, 48, 128, 48, WHITE);     //2nd horizontal line

  //variables to store conversion
  char str[50];
  char str2[5];

#ifdef EMONTX
  // Last seen information from EmonTX
  if ((millis() - last_emontx) > 120000)
  {
    // small font
    glcd.setFont(font_clR4x6);
    int emonTx_fail = (millis() - last_emontx) / 1000;
    if (emonTx_fail < 100) {
      itoa(emonTx_fail, str, 10);
      strcat(str, "sec TxFail");
      glcd.drawString(66, 0, str);
    }
    else
      glcd.drawString(66, 0, PSTR("TxFail"));

  }
#endif

  if ((millis() - last_emonbase) > 180000)
  {
    // small font
    glcd.setFont(font_clR4x6);
    int emonbase_fail = ((millis() - last_emonbase) / 60000);
    if (emonbase_fail < 10) {
      itoa(emonbase_fail, str, 10);
      strcat(str, "min baseFail");
      glcd.drawString(67, 34, str);
    }
    else glcd.drawString(67, 34, PSTR("baseFail"));

  }

  // medium font
  glcd.setFont(font_clR6x8);

  // Indicator for consuming or generating
  if (importing == 1) {
    glcd.drawBitmap(60, 0, icon_lines_12x12, 16, 12, 1);  //pylon
    //glcd.drawString(57, 15, PSTR(">>"));

  }


  glcd.setFont(font_helvB14);


  /*
    //big bold font
    glcd.setFont(font_helvB14);

    // Amount of power currently being used
    if (use > 1000)
    {
      dtostrf(use / 1000, 2, 1, str);
      strcat(str, "kw");
    }
    else
    {
      itoa((int)use, str, 10);
      strcat(str, "w");
    }
    glcd.drawBitmap(49, 0, icon_home_13x12, 16, 12, 1);
    glcd.drawString(3, 9, str);

  */


  // Amount of energy coming from or going into the grid
  if (grid < -1000 || grid > 1000)
  {
    dtostrf(grid / 1000, 2, 1, str);
    strcat(str, "kw");
  }
  else
  {
    itoa((int)grid, str, 10);
    strcat(str, "w");
  }
  glcd.drawString(3, 0, str);




  // Amount of energy being generated
  if (gen > 1000)
  {
    dtostrf(gen / 1000, 2, 1, str);
    strcat(str, "kw");
  }
  else
  {
    itoa((int)gen, str, 10);
    strcat(str, "w");
  }
  //if (gen>maxgen) maxgen=gen;                  -could cause an error if any large spurious readings are detected, max gen can be set manually at beginning of sketch
  //imageindex = int(gen / maxgen * 5 - 0.5);
  //      glcd.drawBitmap(57, 0, icon_solar_12x12[imageindex], 16, 12, 1);  //clouds

  glcd.drawString(80, 0, str);



  glcd.setFont(font_clR4x6);       //small font - Kwh

  strcpy (str, "") ;
  // Kwh consumed today
  dtostrf(usekwh, 0, 1, str);
  strcat(str, "kWh");
  glcd.drawString(6, 18, str);

  strcpy(str, "Today") ;
  glcd.drawString(54, 18, str);

  // Kwh generated today
  dtostrf(genkwh, 0, 1, str);
  strcat(str, "kWh");
  glcd.drawString(90, 18, str);


  glcd.setFont(font_clR4x6);       //small font - Kwh
  strcpy (str, "Out") ;
  glcd.drawString(0, 50, str);


  strcpy (str, "In") ;
  glcd.drawString(115, 50, str);

  // Temperature
  glcd.setFont(font_helvB08);

  //in temp
  strcpy (str, "") ;
  dtostrf(outtemp, 0, 1, str);
  strcat(str, "c");
  glcd.drawString(12, 55, str);

  //out temp
  strcpy (str, "") ;
  dtostrf(intemp, 0, 1, str);
  strcat(str, "c");
  glcd.drawString(85, 55, str);


  //Middle of the bottom temp bar cloud/sunshine icon
  if (gen > PV_gen_offset ) {

    imageindex = int(gen / maxgen * 5 - 0.5);
    glcd.drawBitmap(60, 49, icon_solar_12x12[imageindex], 16, 12, 1); //0 is 3 clouds ,1 is sun with single dot as rays, 2 is sun with larger rays, 3 is sun with heavier rays, 4 is sun with v heavy rays
  }


#ifdef EMONTX

  //glcd.drawString_P(5, 58, PSTR("Time: "));
  strcpy(str, "Time:  ") ;

#else

  glcd.setFont(font_lubB12);


  // Current date + time
  itoa((int)now.day(), str, 10);
  strcat(str, "/");

  itoa((int)now.month(), str2, 10);
  strcat(str, str2);
  strcat(str, "/");

  itoa((int)(now.year() - 2000), str2, 10);
  strcat(str, str2);
  strcat (str, " ") ;
#endif

  glcd.drawString(5, 30, str);


  strcpy(str, "") ;

  //glcd.drawString_P(5, 58, PSTR("Time:"));
  if (now.hour() < 10) {
    strcpy(str, "0") ;
    itoa((int)now.hour(), str2, 10);
    strcat(str, str2) ;
  } else {
    itoa((int)now.hour(), str, 10);
  }

  glcd.drawString(75, 30, str);

  if (now.second() % 2 == 0) {
    strcpy (str, ":") ;
  } else {
    strcpy (str, "") ;
  }
  glcd.drawString(95, 30, str);
  strcpy (str, "") ;

  if  (now.minute() < 10) strcat(str, "0"); else strcat(str, "");


  itoa((int)now.minute(), str2, 10);
  strcat(str, str2);


  glcd.drawString(100, 30, str);



}

/*void draw_dhw_page(double CYLT)
  {
  glcd.clear();
  glcd.fillRect(0,0,128,64,0);

  glcd.drawLine(0, 47, 128, 47, WHITE);     //middle horizontal line

  char str[50];    			 //variable to store conversion
  glcd.setFont(font_clR6x8);
  glcd.drawString_P(0,0,PSTR("BATH READY IN:"));
  glcd.drawString_P(0,53,PSTR("TEMPERATURE:"));

  glcd.setFont(font_helvB24);  		//big bold font

  int mins = 1.9 *(50.0 - (CYLT));
  itoa((int)mins,str,10);
  strcat(str," min");
  glcd.drawString(3,16,str);     		//ammount of power currently being used

  glcd.setFont(font_helvB12);  		//big bold font
  dtostrf((CYLT),0,1,str);
  strcat(str,"C");
  glcd.drawString(80,50,str);

  glcd.refresh();
  }*/

#ifdef ENABLE_HIST
void draw_history_page(double genkwh[7], double usekwh[7])
{
  glcd.clear;
  glcd.fillRect(0, 0, 128, 64, 0);

  char str[50];

  glcd.setFont(font_clR6x8);
  glcd.drawString_P(40, 0, PSTR("History"));

  glcd.setFont(font_clR4x6);

  glcd.drawString_P(2, 16, PSTR("Today"));
  glcd.drawString_P(2, 23, PSTR("Yesterday"));
  glcd.drawString_P(2, 30, PSTR("2 days ago"));
  glcd.drawString_P(2, 37, PSTR("3 days ago"));
  glcd.drawString_P(2, 44, PSTR("4 days ago"));
  glcd.drawString_P(2, 51, PSTR("5 days ago"));
  glcd.drawString_P(2, 58, PSTR("6 days ago"));

  // draw grid consumption history
  char kWh[4] = "kWh";
  glcd.setFont(font_clR4x6);
  glcd.drawString_P(56, 9, PSTR("Used"));

  dtostrf((usekwh[0]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52, 16, str);

  dtostrf((usekwh[1]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52, 23, str);

  dtostrf((usekwh[2]), 0, 1, str);  strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52, 30, str);

  dtostrf((usekwh[3]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52, 37, str);

  dtostrf((usekwh[4]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52, 44, str);

  dtostrf((usekwh[5]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52, 51, str);

  dtostrf((usekwh[6]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(52, 58, str);

  // draw solar PV generation history
  glcd.setFont(font_clR4x6);
  glcd.drawString_P(90, 9, PSTR("PV"));

  dtostrf((genkwh[0]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(83, 16, str);

  dtostrf((genkwh[1]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(83, 23, str);

  dtostrf((genkwh[2]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(83, 30, str);

  dtostrf((genkwh[3]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(83, 37, str);

  dtostrf((genkwh[4]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(83, 44, str);

  dtostrf((genkwh[5]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(83, 51, str);

  dtostrf((genkwh[6]), 0, 1, str); strcat(str, kWh);
  glcd.setFont(font_clR4x6);
  glcd.drawString(83, 58, str);

  glcd.refresh();
}
#endif
