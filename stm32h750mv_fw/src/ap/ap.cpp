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
}

void apMain(void)
{
  uint32_t pre_time;

  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

    if (uartAvailable(_DEF_UART1))
    {
      uartPrintf(_DEF_UART1, "rx 0x%X\n", uartRead(_DEF_UART1));
    }
  }
}
