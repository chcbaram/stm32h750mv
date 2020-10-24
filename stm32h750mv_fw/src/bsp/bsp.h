/*
 * bsp.h
 *
 *  Created on: Oct 20, 2020
 *      Author: baram
 */

#ifndef SRC_BSP_BSP_H_
#define SRC_BSP_BSP_H_


#include "def.h"


#include "stm32h7xx_hal.h"



#ifdef __cplusplus
extern "C" {
#endif


void bspInit(void);

void delay(uint32_t ms);
uint32_t millis(void);


#ifdef __cplusplus
}
#endif

#endif /* SRC_BSP_BSP_H_ */
