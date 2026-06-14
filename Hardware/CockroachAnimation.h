#ifndef __COCKROACHANIMATION_H
#define __COCKROACHANIMATION_H

#include "stm32f10x.h"

void CockroachAnimation_Init(void);
uint8_t CockroachAnimation_Play(void);  /* 返回: 2=菜单, 3=下一, 4=上一 */
void CockroachAnimation_Exit(void);

#endif /* __COCKROACHANIMATION_H */
