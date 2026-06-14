#include "stm32f10x.h"

/* 编码器引脚: A=PB0(EXTI0), B=PB1(EXTI1), SW(按下)=PA7 */
#define ENC_A_PORT   GPIOB
#define ENC_A_PIN    GPIO_Pin_0
#define ENC_B_PORT   GPIOB
#define ENC_B_PIN    GPIO_Pin_1
#define ENC_SW_PORT  GPIOA
#define ENC_SW_PIN   GPIO_Pin_7

#define ENC_STEP       4     /* 累加器阈值，一个 detent = 4 次跳变 */
#define ENC_COOLDOWN   2     /* 触发后冷却(次)，防一次 detent 多次触发 */
#define SW_DEBOUNCE    2     /* 按钮消抖(次) */
#define LONG_PRESS_CNT 50    /* 长按阈值 ≈500ms @ 10ms/call */

/* ── ISR 与主循环共享的编码器状态 ── */
static volatile uint8_t g_prevEnc = 0;
static volatile int8_t  g_encAcc = 0;

/**
  * @brief  Gray 码查表：编码器 AB 状态跳变 → 步进增量
  *         prev<<2|cur → +1(CW) / -1(CCW) / 0(无效/静止)
  *         AB 经反相编码（低电平=触点闭合=active）
  */
static const int8_t g_encTable[16] = {
	 0, -1,  1,  0,    /* 00→00, 00→01, 00→10, 00→11 */
	 1,  0,  0, -1,    /* 01→00, 01→01, 01→10, 01→11 */
	-1,  0,  0,  1,    /* 10→00, 10→01, 10→10, 10→11 */
	 0,  1, -1,  0     /* 11→00, 11→01, 11→10, 11→11 */
};

/**
  * @brief  编码器中断服务函数 — 任一引脚跳变立即触发
  */
static void Enc_ProcessISR(void)
{
	uint8_t curA = GPIO_ReadInputDataBit(ENC_A_PORT, ENC_A_PIN);
	uint8_t curB = GPIO_ReadInputDataBit(ENC_B_PORT, ENC_B_PIN);
	uint8_t curEnc = (curA ? 0 : 2) | (curB ? 0 : 1);   /* 反相：触点闭合=1 */
	g_encAcc += g_encTable[(g_prevEnc << 2) | curEnc];
	g_prevEnc = curEnc;
}

/* PB0 → EXTI0 */
void EXTI0_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
		Enc_ProcessISR();
		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}

/* PB1 → EXTI1 */
void EXTI1_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line1) != RESET) {
		Enc_ProcessISR();
		EXTI_ClearITPendingBit(EXTI_Line1);
	}
}

/**
  * @brief  编码器+按钮初始化（上拉输入 + EXTI 中断）
  */
void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
	                       RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitTypeDef g;
	g.GPIO_Mode  = GPIO_Mode_IPU;
	g.GPIO_Speed = GPIO_Speed_50MHz;

	g.GPIO_Pin = ENC_A_PIN;                  /* PB0 — 编码器 A */
	GPIO_Init(ENC_A_PORT, &g);

	g.GPIO_Pin = ENC_B_PIN;                  /* PB1 — 编码器 B */
	GPIO_Init(ENC_B_PORT, &g);

	g.GPIO_Pin = ENC_SW_PIN;                 /* PA7 — 按钮（轮询） */
	GPIO_Init(ENC_SW_PORT, &g);

	/* 先复位所有 EXTI 配置，清除之前 PA7 的 EXTI7 */
	EXTI_DeInit();

	/* EXTI 线映射到 GPIO 端口 */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);   /* PB0 → EXTI0 */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);   /* PB1 → EXTI1 */

	/* EXTI 配置：双边沿触发 */
	EXTI_InitTypeDef e;
	e.EXTI_Line    = EXTI_Line0 | EXTI_Line1;
	e.EXTI_Mode    = EXTI_Mode_Interrupt;
	e.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	e.EXTI_LineCmd = ENABLE;
	EXTI_Init(&e);

	/* NVIC 配置 */
	NVIC_InitTypeDef n;
	n.NVIC_IRQChannelPreemptionPriority = 1;
	n.NVIC_IRQChannelSubPriority       = 1;
	n.NVIC_IRQChannelCmd               = ENABLE;

	n.NVIC_IRQChannel = EXTI0_IRQn;          /* PB0 */
	NVIC_Init(&n);

	n.NVIC_IRQChannel = EXTI1_IRQn;          /* PB1 */
	NVIC_Init(&n);
}

/**
  * @brief  读取编码器+按钮事件（非阻塞）
  * @retval 0=无操作  1=短按(确认)  2=长按(返回)  3=CCW(逆时针)  4=CW(顺时针)
  */
uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;
	static uint8_t encCool = 0;
	static uint8_t swState = 0;
	static uint16_t swHold = 0;

	uint8_t curSW = GPIO_ReadInputDataBit(ENC_SW_PORT, ENC_SW_PIN);

	/* ── 编码器：读取中断累计的步数 ── */
	if (encCool > 0) {
		encCool--;
	} else {
		if (g_encAcc >= ENC_STEP) {
			KeyNum = 4;          /* CW */
			g_encAcc = 0;
			encCool  = ENC_COOLDOWN;
		} else if (g_encAcc <= -ENC_STEP) {
			KeyNum = 3;          /* CCW */
			g_encAcc = 0;
			encCool  = ENC_COOLDOWN;
		}
	}

	/* ── 按钮：状态机判短按/长按 ── */
	if (curSW == 0) {
		swHold++;
		if (swState == 0 && swHold >= SW_DEBOUNCE) {
			swState = 1;
		}
		if (swHold >= LONG_PRESS_CNT && swState == 1) {
			KeyNum  = 2;         /* 长按 */
			swState = 2;
			swHold  = 0;
		}
	} else {
		if (swState == 1) {
			KeyNum = 1;          /* 短按 */
		}
		swHold  = 0;
		swState = 0;
	}

	return KeyNum;
}

/**
  * @brief  读取并清零编码器中断累计的原始步数（±1/Gray跳变）
  *         用于需要细粒度连续调节的场景（如 sin 图滚动）
  */
int8_t Key_GetEncRaw(void)
{
	int8_t val = g_encAcc;
	g_encAcc = 0;
	return val;
}
