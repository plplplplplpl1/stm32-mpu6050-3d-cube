#include "stm32f10x.h"
#include "OLED_Font.h"
#include "Delay.h"
#include <string.h>

/* 硬件I2C1：PB6=SCL, PB7=SDA */
#define OLED_I2C              I2C1
#define OLED_SLAVE_ADDR       0x78
#define I2C_TIMEOUT           100000UL

/* 图形缓冲区：8页 * 128列，对应128x64像素 */
static uint8_t OLED_GRAM[8][128];

/* 前向声明 */
void OLED_Refresh(void);

/**
  * @brief  硬件I2C总线恢复：SWRST外设 + 9个SCL脉冲释放SDA + STOP + 重新初始化
  * @param  无
  * @retval 无
  * @note   STM32F1 I2C外设已知缺陷：BUSY标志可能卡死。此函数彻底复位总线。
  */
static void OLED_I2C_BusRecover(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStructure;
	uint8_t i;

	/* 1. 复位I2C1外设（SWRST） */
	I2C_SoftwareResetCmd(OLED_I2C, ENABLE);
	I2C_SoftwareResetCmd(OLED_I2C, DISABLE);

	/* 2. 切换PB6/PB7为GPIO开漏，手动发9个SCL脉冲释放SDA */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_SetBits(GPIOB, GPIO_Pin_6 | GPIO_Pin_7);	/* 先释放总线 */
	for (i = 0; i < 10; i++)
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_6);
		Delay_us(10);
		GPIO_SetBits(GPIOB, GPIO_Pin_6);
		Delay_us(10);
	}
	/* 产生STOP条件：SDA拉低→SCL拉高→SDA拉高 */
	GPIO_ResetBits(GPIOB, GPIO_Pin_7);
	Delay_us(10);
	GPIO_SetBits(GPIOB, GPIO_Pin_6);
	Delay_us(10);
	GPIO_SetBits(GPIOB, GPIO_Pin_7);
	Delay_us(10);

	/* 3. 切回复用开漏，重新初始化I2C1外设 */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_ClockSpeed = 400000;		/* 400kHz：OLED帧刷新需高速，SSD1306支持 */
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;
	I2C_Init(OLED_I2C, &I2C_InitStructure);
	I2C_Cmd(OLED_I2C, ENABLE);
}

/**
  * @brief  硬件I2C等待事件（带超时保护 + 总线自动恢复）
  * @param  event I2C事件宏
  * @retval 1=成功，0=超时（已自动复位总线）
  */
static uint8_t OLED_I2C_WaitEvent(uint32_t event)
{
	volatile uint32_t timeout = I2C_TIMEOUT;
	while (!I2C_CheckEvent(OLED_I2C, event))
	{
		if (--timeout == 0)
		{
			OLED_I2C_BusRecover();	/* 超时→复位总线→下次调用可用 */
			return 0;
		}
	}
	return 1;
}

/**
  * @brief  硬件I2C1初始化（PB6=SCL, PB7=SDA, 100kHz）
  * @param  无
  * @retval 无
  */
static void OLED_I2C_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	OLED_I2C_BusRecover();	/* 统一使用总线恢复函数完成复位+初始化 */
}

/**
  * @brief  OLED写命令（硬件I2C），超时自动恢复+重试
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
	uint8_t retry;
	for (retry = 0; retry < 2; retry++)
	{
		I2C_GenerateSTART(OLED_I2C, ENABLE);
		if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT)) continue;
		I2C_Send7bitAddress(OLED_I2C, OLED_SLAVE_ADDR, I2C_Direction_Transmitter);
		if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) continue;
		I2C_SendData(OLED_I2C, 0x00);
		if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) continue;
		I2C_SendData(OLED_I2C, Command);
		if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) continue;
		I2C_GenerateSTOP(OLED_I2C, ENABLE);
		return;	/* 成功 */
	}
	I2C_GenerateSTOP(OLED_I2C, ENABLE);	/* 重试耗尽，强制STOP释放总线 */
}

/**
  * @brief  OLED写数据（硬件I2C），超时自动恢复+重试
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
	uint8_t retry;
	for (retry = 0; retry < 2; retry++)
	{
		I2C_GenerateSTART(OLED_I2C, ENABLE);
		if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT)) continue;
		I2C_Send7bitAddress(OLED_I2C, OLED_SLAVE_ADDR, I2C_Direction_Transmitter);
		if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) continue;
		I2C_SendData(OLED_I2C, 0x40);
		if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) continue;
		I2C_SendData(OLED_I2C, Data);
		if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) continue;
		I2C_GenerateSTOP(OLED_I2C, ENABLE);
		return;
	}
	I2C_GenerateSTOP(OLED_I2C, ENABLE);
}

/**
  * @brief  OLED批量写数据（硬件I2C，单次事务发送多个字节），超时自动恢复+重试
  * @param  pData 数据缓冲区指针
  * @param  len 数据长度
  * @retval 无
  */
void OLED_WriteDataBurst(const uint8_t *pData, uint16_t len)
{
	uint16_t i;
	uint8_t retry;
	uint8_t ok;

	for (retry = 0; retry < 2; retry++)
	{
		ok = 1;
		I2C_GenerateSTART(OLED_I2C, ENABLE);
		if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT)) ok = 0;
		if (ok) {
			I2C_Send7bitAddress(OLED_I2C, OLED_SLAVE_ADDR, I2C_Direction_Transmitter);
			if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) ok = 0;
		}
		if (ok) {
			I2C_SendData(OLED_I2C, 0x40);
			if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) ok = 0;
		}
		if (ok) {
			for (i = 0; i < len; i++)
			{
				I2C_SendData(OLED_I2C, pData[i]);
				if (!OLED_I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) { ok = 0; break; }
			}
		}
		if (ok) {
			I2C_GenerateSTOP(OLED_I2C, ENABLE);
			return;	/* 成功 */
		}
	}
	I2C_GenerateSTOP(OLED_I2C, ENABLE);
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
	OLED_Refresh();
}

/**
  * @brief  仅清空图形缓冲区，不立即刷新屏幕
  * @param  无
  * @retval 无
  */
void OLED_ClearBuffer(void)
{
	memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED显示一个16x16汉字
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~8（16像素宽）
  * @param  Font16x16 汉字点阵数据，32字节，按行排列（每行2字节）
  * @retval 无
  */
void OLED_ShowChinese16(uint8_t Line, uint8_t Column, const uint8_t *Font16x16)
{
	uint8_t x;
	uint8_t bit;
	uint8_t upperByte;
	uint8_t lowerByte;
	uint8_t row;
	uint8_t rowByte;
	uint8_t mask;
	
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
			{
				upperByte |= (1 << bit);
			}
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
			{
				lowerByte |= (1 << bit);
			}
		}
		OLED_WriteData(lowerByte);
	}
}

/**
  * @brief  OLED初始化
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{
	uint32_t i, j;

	for (i = 0; i < 1000; i++)			//上电延时
	{
		for (j = 0; j < 1000; j++);
	}

	OLED_I2C_Init();			//端口初始化
	Delay_ms(50);				//等待OLED内部POR完成
	
	OLED_WriteCommand(0xAE);	//关闭显示
	
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示
		
	OLED_Clear();				//OLED清屏
}

/**
  * @brief  OLED刷新图形缓冲区到屏幕
  * @param  无
  * @retval 无
  */
void OLED_Refresh(void)
{
	uint8_t j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		OLED_WriteDataBurst(OLED_GRAM[j], 128);
	}
}

/**
  * @brief  OLED画点
  * @param  X 横坐标，范围：0~127
  * @param  Y 纵坐标，范围：0~63
  * @param  IsOn 1点亮，0熄灭
  * @retval 无
  */
void OLED_DrawPoint(int16_t X, int16_t Y, uint8_t IsOn)
{
	uint8_t page;
	uint8_t bitMask;
	
	if (X < 0 || X > 127 || Y < 0 || Y > 63)
	{
		return;
	}
	
	page = Y / 8;
	bitMask = 1 << (Y % 8);
	
	if (IsOn)
	{
		OLED_GRAM[page][X] |= bitMask;
	}
	else
	{
		OLED_GRAM[page][X] &= ~bitMask;
	}
}

/**
  * @brief  OLED画线（Bresenham算法）
  * @param  X0,Y0 起点坐标
  * @param  X1,Y1 终点坐标
  * @param  IsOn 1点亮，0熄灭
  * @retval 无
  */
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, uint8_t IsOn)
{
	int16_t dx = X1 - X0;
	int16_t dy = Y1 - Y0;
	int16_t sx = (dx >= 0) ? 1 : -1;
	int16_t sy = (dy >= 0) ? 1 : -1;
	int16_t err;
	int16_t e2;
	
	dx = (dx >= 0) ? dx : -dx;
	dy = (dy >= 0) ? dy : -dy;
	err = ((dx > dy) ? dx : -dy) / 2;
	
	while (1)
	{
		OLED_DrawPoint(X0, Y0, IsOn);
		if (X0 == X1 && Y0 == Y1)
		{
			break;
		}
		e2 = err;
		if (e2 > -dx)
		{
			err -= dy;
			X0 += sx;
		}
		if (e2 < dy)
		{
			err += dx;
			Y0 += sy;
		}
	}
}


/**
  * @brief  OLED画线（灰度/点画线）
  * @param  X0, Y0, X1, Y1 线段端点坐标
  * @param  level 灰度等级：1=最暗，15=最亮（实线）
  * @retval 无
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

	if (level >= 15)
	{
		OLED_DrawLine(X0, Y0, X1, Y1, 1);
		return;
	}
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
