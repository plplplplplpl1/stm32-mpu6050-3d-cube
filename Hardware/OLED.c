#include "stm32f10x.h"
#include "OLED_Font.h"
#include "Delay.h"
#include <string.h>

/* ── 软件 SPI 引脚定义（1.3" 7脚 SSD1306 OLED）──
 *     GND  → GND 轨       VCC  → 3.3V 轨
 *     CLK  → PB5 (SCK)    MOS  → PB6 (MOSI)
 *     IRES → PB7 (RESET)   DC   → PB8 (Data/Cmd)
 *     CS   → PB9 (Chip Select)
 */
#define OLED_SCK_PORT   GPIOB
#define OLED_SCK_PIN    GPIO_Pin_5
#define OLED_MOSI_PORT  GPIOB
#define OLED_MOSI_PIN   GPIO_Pin_6
#define OLED_RES_PORT   GPIOB
#define OLED_RES_PIN    GPIO_Pin_7
#define OLED_DC_PORT    GPIOB
#define OLED_DC_PIN     GPIO_Pin_8
#define OLED_CS_PORT    GPIOB
#define OLED_CS_PIN     GPIO_Pin_9

/* SH1106: 132列内部RAM, 可视区从列2开始 */
#define OLED_COL_OFFSET     2

/* GPIO 位带操作 — BSRR置位 / BRR清零 */
#define OLED_SCK_H()    GPIOB->BSRR = GPIO_Pin_5
#define OLED_SCK_L()    GPIOB->BRR  = GPIO_Pin_5
#define OLED_MOSI_H()   GPIOB->BSRR = GPIO_Pin_6
#define OLED_MOSI_L()   GPIOB->BRR  = GPIO_Pin_6
#define OLED_RES_H()    GPIOB->BSRR = GPIO_Pin_7
#define OLED_RES_L()    GPIOB->BRR  = GPIO_Pin_7
#define OLED_DC_H()     GPIOB->BSRR = GPIO_Pin_8
#define OLED_DC_L()     GPIOB->BRR  = GPIO_Pin_8
#define OLED_CS_H()     GPIOB->BSRR = GPIO_Pin_9
#define OLED_CS_L()     GPIOB->BRR  = GPIO_Pin_9

/* 图形缓冲区：8页 × 128列（可视区），128x64像素 */
uint8_t OLED_GRAM[8][128];

/* 前向声明 */
void OLED_Refresh(void);

/**
  * @brief  软件 SPI 发送一字节（Mode 0, MSB first, ~5MHz）
  * @note   无显式延时，GPIO位带操作本身约140ns/bit，SSD1306上限10MHz
  */
static void OLED_SPI_WriteByte(uint8_t data)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		if (data & 0x80)
			OLED_MOSI_H();
		else
			OLED_MOSI_L();
		data <<= 1;
		OLED_SCK_H();
		OLED_SCK_L();
	}
}

/**
  * @brief  OLED写命令（CS=0, DC=0）
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_CS_L();
	OLED_DC_L();
	OLED_SPI_WriteByte(Command);
	OLED_CS_H();
}

/**
  * @brief  OLED写数据（CS=0, DC=1）
  */
void OLED_WriteData(uint8_t Data)
{
	OLED_CS_L();
	OLED_DC_H();
	OLED_SPI_WriteByte(Data);
	OLED_CS_H();
}

/**
  * @brief  OLED批量写数据（单次CS保持）
  */
void OLED_WriteDataBurst(const uint8_t *pData, uint16_t len)
{
	uint16_t i;
	OLED_CS_L();
	OLED_DC_H();
	for (i = 0; i < len; i++)
	{
		OLED_SPI_WriteByte(pData[i]);
	}
	OLED_CS_H();
}

/**
  * @brief  SSD1306 设置光标位置（页寻址模式）
  * @param  Y 页号（0~7）
  * @param  X 可视列（0~127）
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	uint8_t col = X + OLED_COL_OFFSET;
	OLED_WriteCommand(0xB0 | Y);
	OLED_WriteCommand(0x00 | (col & 0x0F));          /* 列低4位 */
	OLED_WriteCommand(0x10 | ((col >> 4) & 0x0F));   /* 列高4位 */
}

/**
  * @brief  OLED清屏
  */
void OLED_Clear(void)
{
	memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
	OLED_Refresh();
}

/**
  * @brief  仅清缓冲区，不刷新
  */
void OLED_ClearBuffer(void)
{
	memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

/**
  * @brief  OLED显示 8×16 ASCII 字符（直接写 GDDRAM）
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
	for (i = 0; i < 8; i++)
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
	for (i = 0; i < 8; i++)
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);
}

/**
  * @brief  将 ASCII 字符写入 GRAM 缓冲区（不刷新）
  */
void OLED_ShowCharBuf(uint8_t Line, uint8_t Column, char Char)
{
	uint8_t page = (Line - 1) * 2;
	uint8_t col  = (Column - 1) * 8;
	uint8_t i;
	uint8_t idx = Char - ' ';
	for (i = 0; i < 8; i++)
		OLED_GRAM[page][col + i] |= OLED_F8x16[idx][i];
	for (i = 0; i < 8; i++)
		OLED_GRAM[page + 1][col + i] |= OLED_F8x16[idx][i + 8];
}

/**
  * @brief  OLED显示字符串（直接写 GDDRAM）
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
		OLED_ShowChar(Line, Column + i, String[i]);
}

/**
  * @brief  OLED显示 16×16 汉字（直接写 GDDRAM，HZK16 行优先）
  */
void OLED_ShowChinese16(uint8_t Line, uint8_t Column, const uint8_t *Font16x16)
{
	uint8_t x, bit, upperByte, lowerByte, row, rowByte, mask;

	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 16);
	for (x = 0; x < 16; x++)
	{
		upperByte = 0;
		for (bit = 0; bit < 8; bit++)
		{
			row = bit;
			rowByte = Font16x16[row * 2 + (x / 8)];
			mask = 0x80 >> (x % 8);
			if (rowByte & mask)
				upperByte |= (1 << bit);
		}
		OLED_WriteData(upperByte);
	}

	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 16);
	for (x = 0; x < 16; x++)
	{
		lowerByte = 0;
		for (bit = 0; bit < 8; bit++)
		{
			row = bit + 8;
			rowByte = Font16x16[row * 2 + (x / 8)];
			mask = 0x80 >> (x % 8);
			if (rowByte & mask)
				lowerByte |= (1 << bit);
		}
		OLED_WriteData(lowerByte);
	}
}

/**
  * @brief  SSD1306 初始化 — 软件SPI GPIO + RESET + SSD1306 命令序列
  */
void OLED_Init(void)
{
	uint32_t i, j;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* 上电等待 */
	for (i = 0; i < 1000; i++)
		for (j = 0; j < 1000; j++);

	/* GPIOB 时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	/* SPI 控制引脚：推挽输出 */
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7
	                              | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* 初始电平 */
	OLED_CS_H();
	OLED_SCK_L();
	OLED_MOSI_L();
	OLED_DC_L();
	OLED_RES_H();
	Delay_ms(10);

	/* 硬件复位 */
	OLED_RES_L();
	Delay_ms(10);
	OLED_RES_H();
	Delay_ms(10);

	/* ── SH1106 硬件 + SSD1306 兼容命令序列 ── */
	OLED_WriteCommand(0xAE);	/* 关闭显示 */

	OLED_WriteCommand(0xD5);	/* 设置显示时钟分频比/振荡器频率 */
	OLED_WriteCommand(0x80);

	OLED_WriteCommand(0xA8);	/* 设置多路复用率 */
	OLED_WriteCommand(0x3F);

	OLED_WriteCommand(0xD3);	/* 设置显示偏移 */
	OLED_WriteCommand(0x00);

	OLED_WriteCommand(0x40);	/* 设置显示开始行 */

	OLED_WriteCommand(0xA1);	/* 段重映射（左右方向，0xA1正常 0xA0镜像）*/
	OLED_WriteCommand(0xC8);	/* COM扫描方向（上下方向，0xC8正常 0xC0翻转）*/

	OLED_WriteCommand(0xDA);	/* COM引脚硬件配置 */
	OLED_WriteCommand(0x12);

	OLED_WriteCommand(0x81);	/* 对比度 */
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	/* 预充电周期 */
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	/* VCOMH 取消选择级别 */
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0x8D);	/* 电荷泵使能 */
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xA4);	/* 全屏显示关闭（正常模式）*/
	OLED_WriteCommand(0xA6);	/* 正常显示（不反色）*/

	OLED_WriteCommand(0xAF);	/* 开启显示 */

	OLED_Clear();
}

/**
  * @brief  OLED刷新 — 逐页写入, 单次CS(7p优化)
  */
void OLED_Refresh(void)
{
	uint8_t page, i;
	OLED_CS_L();
	for (page = 0; page < 8; page++)
	{
		OLED_DC_L();
		OLED_SPI_WriteByte(0xB0 | page);
		OLED_SPI_WriteByte(0x00 | (OLED_COL_OFFSET & 0x0F));
		OLED_SPI_WriteByte(0x10 | ((OLED_COL_OFFSET >> 4) & 0x0F));
		OLED_DC_H();
		for (i = 0; i < 128; i++)
			OLED_SPI_WriteByte(OLED_GRAM[page][i]);
	}
	OLED_CS_H();
}

/**
  * @brief  OLED画点（写入 GRAM 缓冲区）
  */
void OLED_DrawPoint(int16_t X, int16_t Y, uint8_t IsOn)
{
	uint8_t page;
	uint8_t bitMask;

	if (X < 0 || X > 127 || Y < 0 || Y > 63)
		return;

	page    = Y / 8;
	bitMask = 1 << (Y % 8);

	if (IsOn)
		OLED_GRAM[page][X] |= bitMask;
	else
		OLED_GRAM[page][X] &= ~bitMask;
}

/**
  * @brief  OLED画线（Bresenham）
  */
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, uint8_t IsOn)
{
	int16_t dx = X1 - X0;
	int16_t dy = Y1 - Y0;
	int16_t sx = (dx >= 0) ? 1 : -1;
	int16_t sy = (dy >= 0) ? 1 : -1;
	int16_t err, e2;

	dx = (dx >= 0) ? dx : -dx;
	dy = (dy >= 0) ? dy : -dy;
	err = ((dx > dy) ? dx : -dy) / 2;

	while (1)
	{
		OLED_DrawPoint(X0, Y0, IsOn);
		if (X0 == X1 && Y0 == Y1) break;
		e2 = err;
		if (e2 > -dx) { err -= dy; X0 += sx; }
		if (e2 < dy)  { err += dx; Y0 += sy; }
	}
}

/**
  * @brief  OLED画灰度线
  */
void OLED_DrawLineGray(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, uint8_t level)
{
	int16_t dx = X1 - X0;
	int16_t dy = Y1 - Y0;
	int16_t sx = (dx >= 0) ? 1 : -1;
	int16_t sy = (dy >= 0) ? 1 : -1;
	int16_t err, e2;
	uint16_t counter = 0;
	uint8_t step;

	if (level >= 15) { OLED_DrawLine(X0, Y0, X1, Y1, 1); return; }
	if (level == 0) return;

	dx = (dx >= 0) ? dx : -dx;
	dy = (dy >= 0) ? dy : -dy;
	err = ((dx > dy) ? dx : -dy) / 2;
	step = 16 / level;
	if (step < 2) step = 2;

	while (1)
	{
		if ((counter++ % step) == 0)
			OLED_DrawPoint(X0, Y0, 1);
		if (X0 == X1 && Y0 == Y1) break;
		e2 = err;
		if (e2 > -dx) { err -= dy; X0 += sx; }
		if (e2 < dy)  { err += dx; Y0 += sy; }
	}
}
