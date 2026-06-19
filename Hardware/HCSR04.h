#ifndef __HCSR04_H
#define __HCSR04_H

#include "stm32f10x.h"

void HCSR04_Init(void);
uint16_t HCSR04_Measure(void);  /* 返回距离(cm), 0=超时 */

#endif
