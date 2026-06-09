#include "stm32f10x.h"                  // Device header
#include "Delay.h"

#define DEBOUNCE_CNT    3    // 连续N次读到低电平才确认按下（每次调用间隔≈10ms）

/**
  * 函    数：按键初始化
  * 参    数：无
  * 返 回 值：无
  */
void Key_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟

	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB1引脚初始化为上拉输入

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_4 | GPIO_Pin_2;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA6、PA4、PA2引脚初始化为上拉输入
		GPIOA->BSRR = GPIO_Pin_2;		//显式置位PA2上拉
}

/**
  * 函    数：按键获取键码（非阻塞，无 Delay_ms 消抖）
  * 参    数：无
  * 返 回 值：按下按键的键码值，范围：0~4，返回0代表没有按键按下
  * 注意事项：采用计数消抖（连续 N 次读到低电平才确认），不阻塞主循环。
  *           调用间隔建议 ≤20ms，配合 DEBOUNCE_CNT=3 可实现 ~30ms 消抖。
  */
uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;
	static uint8_t cnt1 = 0, cnt6 = 0, cnt12 = 0, cnt4 = 0;
	static uint8_t ready1 = 1, ready6 = 1, ready12 = 1, ready4 = 1;
	uint8_t cur1, cur6, cur12, cur4;

	cur1  = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
	cur6  = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6);
	cur4  = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4);
	cur12 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2);

	/* ── PB1 (Key1) ── */
	if (cur1 == 0) {
		if (ready1 && ++cnt1 >= DEBOUNCE_CNT) {
			KeyNum = 1;
			ready1 = 0;
			cnt1 = 0;
		}
	} else {
		ready1 = 1;
		cnt1 = 0;
	}

	/* ── PA6 (Key3) ── */
	if (cur6 == 0) {
		if (ready6 && ++cnt6 >= DEBOUNCE_CNT) {
			KeyNum = 3;
			ready6 = 0;
			cnt6 = 0;
		}
	} else {
		ready6 = 1;
		cnt6 = 0;
	}

	/* ── PA4 (Key4) ── */
	if (cur4 == 0) {
		if (ready4 && ++cnt4 >= DEBOUNCE_CNT) {
			KeyNum = 4;
			ready4 = 0;
			cnt4 = 0;
		}
	} else {
		ready4 = 1;
		cnt4 = 0;
	}

	/* ── PA2 (Key2) ── */
	if (cur12 == 0) {
		if (ready12 && ++cnt12 >= DEBOUNCE_CNT) {
			KeyNum = 2;
			ready12 = 0;
			cnt12 = 0;
		}
	} else {
		ready12 = 1;
		cnt12 = 0;
	}

	return KeyNum;
}
