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

  pre_time = millis();
  while(1)
  {
    cmdifMain();

    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }
  }
}
