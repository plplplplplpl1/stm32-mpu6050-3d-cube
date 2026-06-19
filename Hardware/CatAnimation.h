#ifndef __CATANIMATION_H
#define __CATANIMATION_H

#include "stm32f10x.h"

void CatAnimation_Init(void);
uint8_t CatAnimation_Play(void);  /* 返回触发退出的按键: 2=KEY2菜单, 3=KEY3下一, 4=KEY4上一 */
void CatAnimation_Exit(void);     /* 完全退出：关LED + OLED重新初始化（返回菜单用） */

#endif /* __CATANIMATION_H */
