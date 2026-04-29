#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowChinese16(uint8_t Line, uint8_t Column, const uint8_t *Font16x16);
void OLED_ClearBuffer(void);
void OLED_DrawPoint(int16_t X, int16_t Y, uint8_t IsOn);
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, uint8_t IsOn);
void OLED_Refresh(void);

#endif
