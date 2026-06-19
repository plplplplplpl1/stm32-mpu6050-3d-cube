#ifndef __WOUOUI_USER_H__
#define __WOUOUI_USER_H__

#include "WouoUI.h"

extern void OLED_SendBuff(uint8_t buff[8][128]);  /* 移植层提供 */

void TestUI_Init(void);
uint8_t WouoUI_MenuRun(void);      /* 运行主菜单，返回0~3选项 */
uint8_t WouoUI_SubMenuRun(void);   /* 运行子菜单，返回0=3D, 1=2D, 2=返回 */

#endif
