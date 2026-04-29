#include "stm32f10x.h"                  // Device header
#include "Delay.h"

/**
  * 函    数：按键初始化
  * 参    数：无
  * 返 回 值：无
  */
void Key_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟

	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB1和PB12引脚初始化为上拉输入
}

/**
  * 函    数：按键获取键码（非阻塞）
  * 参    数：无
  * 返 回 值：按下按键的键码值，范围：0~2，返回0代表没有按键按下
  * 注意事项：非阻塞，不会卡住主循环。每次调用只检测一次，不会重复触发。
  */
uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;
	static uint8_t prev1 = 1, prev12 = 1;
	static uint8_t released1 = 1, released12 = 1;
	uint8_t cur1, cur12;

	cur1 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
	cur12 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12);

	/* 检测到高电平（松手），标记为可再次触发 */
	if (cur1 == 1) released1 = 1;
	if (cur12 == 1) released12 = 1;

	/* 按键1：下降沿触发，且前一次已松手 */
	if (prev1 == 1 && cur1 == 0 && released1)
	{
		Delay_ms(20);											//延时消抖
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)		//确认仍按下
		{
			KeyNum = 1;
			released1 = 0;										//标记已触发，等待松手
		}
	}

	/* 按键2：下降沿触发，且前一次已松手 */
	if (prev12 == 1 && cur12 == 0 && released12)
	{
		Delay_ms(20);											//延时消抖
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0)		//确认仍按下
		{
			KeyNum = 2;
			released12 = 0;										//标记已触发，等待松手
		}
	}

	prev1 = cur1;
	prev12 = cur12;

	return KeyNum;												//返回键码值，无按键则返回0
}
