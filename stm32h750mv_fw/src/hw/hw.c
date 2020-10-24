/*
 * hw.c
 *
 *  Created on: Oct 20, 2020
 *      Author: baram
 */




#include "hw.h"





void hwInit(void)
{
  bspInit();

  cmdifInit();
  swtimerInit();
  ledInit();
  buttonInit();
  uartInit();


  usbInit();
  usbBegin(USB_CDC_MODE);
  vcpInit();

  uartOpen(_DEF_UART1, 57600);

}

