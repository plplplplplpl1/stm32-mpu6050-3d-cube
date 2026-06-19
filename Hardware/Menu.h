#ifndef __MENU_H
#define __MENU_H

#include "stm32f10x.h"

void Menu_Init(void);
uint8_t Menu_Show(void);
uint8_t Menu_Show3D2D(void);  /* 3D&2D 子菜单: 返回 0=3D, 1=2D, 2=返回 */

#endif
