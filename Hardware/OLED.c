#include "stm32f10x.h"
#include "OLED_Font.h"
#include "Delay.h"
#include <string.h>

/*引脚配置*/
#define OLED_W_SCL(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)(x))
#define OLED_W_SDA(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)(x))

/* 图形缓冲区：8页 * 128列，对应128x64像素 */
static uint8_t OLED_GRAM[8][128];

/* 前向声明，避免在OLED_Clear中被隐式声明 */
void OLED_Refresh(void);

/*引脚初始化*/
void OLED_I2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C开始
  * @param  无
  * @retval 无
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	Delay_us(2);
	OLED_W_SDA(0);
	Delay_us(2);
	OLED_W_SCL(0);
}

/**
  * @brief  I2C停止
  * @param  无
  * @retval 无
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	Delay_us(2);
	OLED_W_SDA(1);
	Delay_us(2);
}

/**
  * @brief  I2C发送一个字节
  * @param  Byte 要发送的一个字节
  * @retval 无
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(!!(Byte & (0x80 >> i)));
		Delay_us(1);
		OLED_W_SCL(1);
		Delay_us(1);
		OLED_W_SCL(0);
		Delay_us(1);
	}
	OLED_W_SCL(1);	//额外的一个时钟，不处理应答信号
	Delay_us(1);
	OLED_W_SCL(0);
	Delay_us(1);
}

/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x00);		//写命令
	OLED_I2C_SendByte(Command); 
	OLED_I2C_Stop();
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x40);		//写数据
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/**
  * @brief  OLED批量写数据（单次I2C事务发送多个字节）
  * @param  pData 数据缓冲区指针
  * @param  len 数据长度
  * @retval 无
  * @note   跳过重复的Start/Stop，大幅提升刷新效率
  */
static void OLED_WriteDataBurst(const uint8_t *pData, uint16_t len)
{
	uint16_t i;
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x40);		//写数据
	for (i = 0; i < len; i++)
	{
		OLED_I2C_SendByte(pData[i]);
	}
	OLED_I2C_Stop();
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
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
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
