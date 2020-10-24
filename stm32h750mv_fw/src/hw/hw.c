/*
 * hw.c
 *
 *  Created on: Oct 20, 2020
 *      Author: baram
 */




#include "hw.h"


extern bool st7735Init(void);


void hwInit(void)
{
  bspInit();

  cmdifInit();
  swtimerInit();
  ledInit();
  buttonInit();
  gpioInit();
  spiInit();
  uartInit();


  usbInit();
  usbBegin(USB_CDC_MODE);
  vcpInit();

  uartOpen(_DEF_UART1, 57600);

  st7735Init();

}



