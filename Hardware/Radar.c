#include "Radar.h"
#include "OLED.h"
#include "Key.h"
#include "Delay.h"
#include "Attitude.h"
#include "FontCN.h"
#include <math.h>

#define CX   64
#define CY   32
#define R    31

/* ── Bresenham 画圆（与 2D 图像同款 DrawLine/DrawPoint）── */
static void DrawCircle(int16_t cx, int16_t cy, int16_t r)
{
	int16_t x = 0, y = r, d = 3 - 2 * r;
	while (x <= y)
	{
		OLED_DrawPoint(cx + x, cy + y, 1);
		OLED_DrawPoint(cx - x, cy + y, 1);
		OLED_DrawPoint(cx + x, cy - y, 1);
		OLED_DrawPoint(cx - x, cy - y, 1);
		OLED_DrawPoint(cx + y, cy + x, 1);
		OLED_DrawPoint(cx - y, cy + x, 1);
		OLED_DrawPoint(cx + y, cy - x, 1);
		OLED_DrawPoint(cx - y, cy - x, 1);
		if (d < 0) d += 4 * x + 6;
		else       { d += 4 * (x - y) + 10; y--; }
		x++;
	}
}

void Radar_Run(void)
{
	uint8_t key;
	float angle = 0.0f;
	int16_t ex, ey;

	OLED_Clear();

	while (1)
	{
		key = Key_GetNum();
		if (key == 2) break;         /* 长按返回 */
		if (key == 4) angle += 0.0872665f;  /* CW +5° */
		if (key == 3) angle -= 0.0872665f;  /* CCW -5° */
		if (angle >  6.2831853f) angle -= 6.2831853f;
		if (angle < 0)            angle += 6.2831853f;

		/* ── 与 2D 图像同款绘制流程：ClearBuffer → 画 → Refresh ── */
		OLED_ClearBuffer();

		/* 静态网格 */
		DrawCircle(CX, CY, R);
		DrawCircle(CX, CY, R * 2 / 3);
		DrawCircle(CX, CY, R / 3);
		OLED_DrawLine(CX, CY - R, CX, CY + R, 1);
		OLED_DrawLine(CX - R, CY, CX + R, CY, 1);

		/* 扫描线 — 和 2D sin 图像完全一样的 sinf/cosf 用法 */
		ex = CX + (int16_t)((float)R * sinf(angle));
		ey = CY - (int16_t)((float)R * cosf(angle));
		OLED_DrawLine(CX, CY, ex, ey, 1);

		/* 角度数字 */
		{
			int16_t deg = (int16_t)(angle * 57.29578f);
			char buf[4];
			if (deg < 0) deg += 360;
			buf[0] = (deg >= 100) ? ('0' + deg / 100) : ' ';
			buf[1] = '0' + (deg / 10) % 10;
			buf[2] = '0' + deg % 10;
			OLED_ShowCharBuf(1, 12, buf[0]);
			OLED_ShowCharBuf(1, 13, buf[1]);
			OLED_ShowCharBuf(1, 14, buf[2]);
		}

		OLED_Refresh();
		Delay_ms(10);

		/* 自动旋转（不转编码器时慢慢转） */
		angle += 0.00872665f;  /* +0.5°/帧 @100Hz ≈ 50°/s */
		if (angle > 6.2831853f) angle -= 6.2831853f;
	}

	OLED_Clear();
}
