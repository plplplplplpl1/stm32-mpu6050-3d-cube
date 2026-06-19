#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

/* ── OLED 引脚定义（软件 SPI）── */
#define OLED_SCK_PIN    GPIO_Pin_5   /* PB5 — SPI 时钟 */
#define OLED_MOSI_PIN   GPIO_Pin_6   /* PB6 — SPI 数据 (MOSI) */
#define OLED_RES_PIN    GPIO_Pin_7   /* PB7 — 硬件复位 (低有效) */
#define OLED_DC_PIN     GPIO_Pin_8   /* PB8 — 数据/命令 (H=数据, L=命令) */
#define OLED_CS_PIN     GPIO_Pin_9   /* PB9 — 片选 (低有效) */

/* ── 公共 API ── */
extern uint8_t OLED_GRAM[8][128];       /* 图形缓冲区，页×列 */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ClearBuffer(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowChinese16(uint8_t Line, uint8_t Column, const uint8_t *Font16x16);
void OLED_ShowCharBuf(uint8_t Line, uint8_t Column, char Char);
void OLED_Refresh(void);
void OLED_SetCursor(uint8_t Y, uint8_t X);
void OLED_WriteDataBurst(const uint8_t *pData, uint16_t len);
void OLED_DrawPoint(int16_t X, int16_t Y, uint8_t IsOn);
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, uint8_t IsOn);
void OLED_DrawLineGray(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, uint8_t level);

#endif
