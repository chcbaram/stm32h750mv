/*
 * ap.cpp
 *
 *  Created on: Oct 20, 2020
 *      Author: baram
 */




#include "ap.h"





void apInit(void)
{
  hwInit();
  cmdifOpen(_DEF_UART1, 57600);
}

void apMain(void)
{
  uint32_t pre_time;
  uint16_t x = 0;
  uint16_t y = 0;
  uint32_t show_data[2];


  pre_time = millis();
  while(1)
  {
    cmdifMain();

    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);

      show_data[0] = lcdGetFpsTime();
      show_data[1] = lcdGetFps();
    }

    if (lcdDrawAvailable())
    {
      lcdClearBuffer(black);


      lcdPrintf(0,  0, white, "%d ms", show_data[0]);
      lcdPrintf(0, 16, white, "%d fps", show_data[1]);
      lcdPrintfRect(0, 0, LCD_WIDTH, LCD_HEIGHT, red, 2, LCD_ALIGN_H_CENTER | LCD_ALIGN_V_BOTTOM,  "LCD보드");

      lcdDrawFillRect(x, 32, 6, 6, red);
      lcdDrawFillRect(lcdGetWidth()-x, 38, 6, 6, green);
      lcdDrawFillRect(x, 44, 6, 6, blue);

      x += 2;

      x %= lcdGetWidth();
      y %= lcdGetHeight();

      lcdRequestDraw();
    }

  }
}
