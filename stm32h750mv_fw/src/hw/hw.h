/*
 * hw.h
 *
 *  Created on: Oct 20, 2020
 *      Author: baram
 */

#ifndef SRC_HW_HW_H_
#define SRC_HW_HW_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"

#include "led.h"
#include "button.h"
#include "swtimer.h"
#include "usb.h"
#include "uart.h"
#include "cmdif.h"


void hwInit(void);



#ifdef __cplusplus
}
#endif

#endif /* SRC_HW_HW_H_ */
