#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "stm32f1xx.h"

// uncomment line below if RTOS is used
//#define RTOS_USED

#define EDEBUG 1
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
#define  stdinout_huart   huart1

#endif
