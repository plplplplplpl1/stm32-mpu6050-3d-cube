/**
 * WouoUI 移植层 — 对接我们的 SPI OLED 和编码器
 */
#include "WouoUI.h"
#include "OLED.h"
#include "Key.h"
#include "Delay.h"
#include <string.h>

/* WouoUI 内部缓冲区 — 直接映射到我们的 OLED_GRAM */
extern uint8_t OLED_GRAM[8][128];

/**
 * 刷屏回调 — WouoUI 渲染完后调用
 */
void OLED_SendBuff(uint8_t buff[8][128])
{
	memcpy(OLED_GRAM, buff, 1024);
	OLED_Refresh();
}

/**
 * 编码器输入 — 在主循环轮询，转成 WouoUI 消息
 */
void WouoUI_FeedEncoder(void)
{
	uint8_t key = Key_GetNum();
	if (key == 0) return;

	switch (key) {
		case 3:  WOUOUI_MSG_QUE_SEND(msg_up);    break;  /* CCW = 上 */
		case 4:  WOUOUI_MSG_QUE_SEND(msg_down);  break;  /* CW  = 下 */
		case 1:  WOUOUI_MSG_QUE_SEND(msg_click); break;  /* 短按 */
		case 2:  WOUOUI_MSG_QUE_SEND(msg_return);break;  /* 长按 */
	}
}

/* WouoUI 日志输出（可选） */
#ifdef WOUOUI_LOG_ENABLE
#include <stdio.h>
void WouoUI_Log(const char *fmt, ...) { /* 不用串口，空实现 */ }
#endif
