#include "HCSR04.h"
#include "Delay.h"

/* PA0=TRIG, PA1=ECHO(TIM2_CH2), TIM2 1MHz 自由运行 */

void HCSR04_Init(void)
{
	GPIO_InitTypeDef g;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	g.GPIO_Mode  = GPIO_Mode_Out_PP;
	g.GPIO_Pin   = GPIO_Pin_0;
	g.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &g);
	GPIO_ResetBits(GPIOA, GPIO_Pin_0);

	g.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	g.GPIO_Pin  = GPIO_Pin_1;
	GPIO_Init(GPIOA, &g);
}

/**
  * @brief  触发一次测距（阻塞，~40ms），返回 cm
  * @retval 2~400cm, 0=超时(>4m或未连接)
  * @note   TIM2 已配置 72MHz/72=1MHz, TIM2->CNT 每 tick=1μs
  */
uint16_t HCSR04_Measure(void)
{
	uint32_t t0, t1, t2, dt;

	/* 发 10μs 触发脉冲 */
	GPIO_SetBits(GPIOA, GPIO_Pin_0);
	Delay_us(15);
	GPIO_ResetBits(GPIOA, GPIO_Pin_0);

	/* 等 ECHO 上升沿，TIM2 计时超时 40ms */
	t0 = TIM2->CNT;
	while ((GPIOA->IDR & GPIO_Pin_1) == 0) {
		if ((uint32_t)(TIM2->CNT - t0) > 40000) return 0;
	}

	/* 记录上升沿时刻，等下降沿 */
	t1 = TIM2->CNT;
	t0 = t1;
	while ((GPIOA->IDR & GPIO_Pin_1) != 0) {
		if ((uint32_t)(TIM2->CNT - t0) > 40000) return 0;
	}
	t2 = TIM2->CNT;

	/* 脉宽 μs (TIM2 1MHz, 自动处理32位翻转) */
	dt = (uint32_t)(t2 - t1);

	/* 距离 = 脉宽(μs) × 0.0343 / 2 ≈ 脉宽 / 58 */
	dt = dt * 343 / 20000;
	if (dt > 400) dt = 400;
	return (uint16_t)dt;
}
