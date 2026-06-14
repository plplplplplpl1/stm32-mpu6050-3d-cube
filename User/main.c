#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "MPU6050.h"
#include "Key.h"
#include "Attitude.h"
#include "Cube3D.h"
#include "FontCN.h"
#include "Menu.h"
#include "Animation.h"
#include "W25Q64.h"
#include "W25Q64_Layout.h"
#include "serial.h"
#include <math.h>

#define TIM2_CLOCK_HZ       1000000UL
#define DT_MAX_SEC          0.2f
#define DT_MIN_SEC          0.001f

static uint8_t MPU_ID_IsSupported(uint8_t id)
{
	return (id == 0x68 || id == 0x69 || id == 0x70 || id == 0x71);
}

static void ShowDirPage(int8_t dir)
{
	OLED_Clear();
	OLED_ShowChinese16(2, 2, HZK_FAN);
	OLED_ShowString(2, 5, ":");
	if (dir == 1)
		OLED_ShowString(2, 6, "NORMAL");
	else
		OLED_ShowString(2, 6, "REVERSE");
}

static void Timer2_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM2->PSC = 72 - 1;
	TIM2->ARR = 0xFFFFFFFFUL;
	TIM2->CR1 |= TIM_CR1_CEN;
}

int main(void)
{
	uint8_t mpuId;
	uint8_t keyNum;
	int8_t rotationDir = 1;
	uint8_t dirChanged = 0;
	uint8_t shape = SHAPE_CUBE;
	Attitude_t attitude;
	uint32_t lastTick, curTick;
	float dtSec;

	OLED_Init();
	Key_Init();
	MPU6050_Init();
	Serial_Init();
	/* MPU6050从睡眠唤醒后需等待时钟和MEMS稳定（datasheet: 30ms min） */
	Delay_ms(100);

	/* ── W25Q64 初始化 ── */
	W25Q64_Init();

	#if 0  /* 串口烧录 — 需要时改为 1 */
	Serial_FlashBurn();
#endif

#if 0  /* W25Q64 诊断 — 需要时改为 1 */
	OLED_Clear();
	{
		uint8_t m,tp,c,s1,s2,s3,d[4]; char h[3];
		W25Q64_ReadJEDECID(&m,&tp,&c);
		OLED_ShowString(1,0,"JD");
		h[0]="0123456789ABCDEF"[m>>4]; h[1]="0123456789ABCDEF"[m&0xF]; h[2]=0; OLED_ShowString(1,2,h);
		h[0]="0123456789ABCDEF"[tp>>4]; h[1]="0123456789ABCDEF"[tp&0xF]; OLED_ShowString(1,4,h);
		h[0]="0123456789ABCDEF"[c>>4]; h[1]="0123456789ABCDEF"[c&0xF]; OLED_ShowString(1,6,h);
		OLED_ShowString(1,9,"OK");
		s1=W25Q64_ReadSR1(); s2=W25Q64_ReadSR2(); s3=W25Q64_ReadSR3();
		OLED_ShowString(2,0,"S");
		h[0]="0123456789ABCDEF"[s1>>4]; h[1]="0123456789ABCDEF"[s1&0xF]; h[2]=0; OLED_ShowString(2,1,h);
		OLED_ShowString(2,3,"/");
		h[0]="0123456789ABCDEF"[s2>>4]; h[1]="0123456789ABCDEF"[s2&0xF]; OLED_ShowString(2,4,h);
		OLED_ShowString(2,6,"/");
		h[0]="0123456789ABCDEF"[s3>>4]; h[1]="0123456789ABCDEF"[s3&0xF]; OLED_ShowString(2,7,h);
		OLED_ShowString(2,10,(s1&0x80)?"P+":"P-");
		OLED_ShowString(2,12,(s1&0x02)?"W+":"W-");
		OLED_ShowString(2,14,(s1&0x01)?"B+":"B-");
		h[0]="0123456789ABCDEF"[(s1>>2)&0xF]; h[1]=0; OLED_ShowString(2,16,h);
		W25Q64_WriteEnable(); s1=W25Q64_ReadSR1();
		OLED_ShowString(3,0,"WE");
		h[0]="0123456789ABCDEF"[s1>>4]; h[1]="0123456789ABCDEF"[s1&0xF]; h[2]=0; OLED_ShowString(3,2,h);
		OLED_ShowString(3,5,(s1&0x80)?"P+":"P-");
		OLED_ShowString(3,7,(s1&0x02)?"WEL=1":"WEL0");
		OLED_ShowString(3,12,(s1&0x01)?"BUSY":"-");
		W25Q64_SectorErase(W25Q_CALIB_ADDR);
		W25Q64_WriteEnable();
		d[0]=0xA5; d[1]=0x5A; d[2]=0x3C; d[3]=0xC3;
		W25Q64_PageProgram(W25Q_CALIB_ADDR,d,4);
		d[0]=d[1]=d[2]=d[3]=0;
		W25Q64_ReadData(W25Q_CALIB_ADDR,d,4);
		OLED_ShowString(4,0,"W");
		h[0]="0123456789ABCDEF"[d[0]>>4]; h[1]="0123456789ABCDEF"[d[0]&0xF]; h[2]=0; OLED_ShowString(4,1,h);
		h[0]="0123456789ABCDEF"[d[1]>>4]; h[1]="0123456789ABCDEF"[d[1]&0xF]; OLED_ShowString(4,3,h);
		h[0]="0123456789ABCDEF"[d[2]>>4]; h[1]="0123456789ABCDEF"[d[2]&0xF]; OLED_ShowString(4,5,h);
		h[0]="0123456789ABCDEF"[d[3]>>4]; h[1]="0123456789ABCDEF"[d[3]&0xF]; OLED_ShowString(4,7,h);
		OLED_ShowString(4,9,(d[0]==0xA5)?"OK":"FAIL");
	}
	Delay_ms(5000);
#endif

	Timer2_Init();
	Attitude_Init();

	/* 启动传感器自检（中文显示） */
	mpuId = MPU6050_GetID();
	OLED_Clear();
	{
		const uint8_t *label[] = {HZK_4F20, HZK_611F, HZK_5668};  /* 传感器 */
		char hex[3];
		OLED_ShowChineseStr(1, 1, label, 3);
		OLED_ShowString(1, 5, ":");
		hex[0] = "0123456789ABCDEF"[mpuId >> 4];
		hex[1] = "0123456789ABCDEF"[mpuId & 0x0F];
		hex[2] = '\0';
		OLED_ShowString(1, 6, hex);
	}
	if (!MPU_ID_IsSupported(mpuId))
	{
		const uint8_t *err[] = {HZK_9519, HZK_8BEF};               /* 错误 */
		const uint8_t *chk[] = {HZK_68C0, HZK_67E5, HZK_63A5, HZK_7EBF}; /* 检查接线 */
		OLED_ShowChineseStr(2, 3, err, 2);
		OLED_ShowChineseStr(3, 2, chk, 4);
	}
	else
	{
		const uint8_t *ok[] = {HZK_6B63, HZK_5E38};               /* 正常 */
		OLED_ShowChineseStr(2, 3, ok, 2);
	}
	Delay_ms(300);

	while (1)
	{
		uint8_t menuRet = Menu_Show();

			if (menuRet == 0)  /* 3D&2D */
			{
				while (1)
				{
				uint8_t subRet = Menu_Show3D2D();

				if (subRet == 2)  /* 返回主菜单 */
					break;

				if (subRet == 0)  /* 3D */
				{
				lastTick = TIM2->CNT;

			while (1)
			{
				keyNum = Key_GetNum();

				if (keyNum == 2)  /* PA2: 返回菜单 */
					break;

				if (keyNum == 1)  /* PB1: 切换旋转方向 */
				{
					rotationDir = -rotationDir;
					dirChanged = 1;
				}

				if (keyNum == 3)  /* PA6: 下一个图形 */
				{
					shape = (shape + 1) % TOTAL_SHAPE_COUNT;
					OLED_Clear();
					if (shape == SHAPE_CUBE)            OLED_ShowString(2, 4, "CUBE");
					else if (shape == SHAPE_OCTAHEDRON)  OLED_ShowString(2, 4, "OCTA");
					else if (shape == SHAPE_TETRAHEDRON) OLED_ShowString(2, 4, "TETRA");
					else if (shape == SHAPE_DODECAHEDRON) OLED_ShowString(2, 4, "DODEC");
					else if (shape == SHAPE_ICOSAHEDRON)  OLED_ShowString(2, 4, "ICOSA");
					else if (shape == SHAPE_CUBOCTAHEDRON) OLED_ShowString(2, 4, "CUBOCT");
					else if (shape == SHAPE_TRUNCATED_TETRA) OLED_ShowString(2, 4, "TRTET");
					else if (shape == SHAPE_SMALL_STELLATED) OLED_ShowString(2, 4, "SSTEL");
					else if (shape == SHAPE_GREAT_STELLATED) OLED_ShowString(2, 4, "GSTEL");
					else if (shape == SHAPE_H3_7)  OLED_ShowString(2, 4, "{3,7}");
					else if (shape == SHAPE_H3_8)  OLED_ShowString(2, 4, "{3,8}");
					else if (shape == SHAPE_H4_5)  OLED_ShowString(2, 4, "{4,5}");
					else if (shape == SHAPE_H5_4)  OLED_ShowString(2, 4, "{5,4}");
					else if (shape == SHAPE_H4_6)  OLED_ShowString(2, 4, "{4,6}");
					else if (shape == SHAPE_H6_4)  OLED_ShowString(2, 4, "{6,4}");
					else if (shape == SHAPE_H5_5)  OLED_ShowString(2, 4, "{5,5}");
					else if (shape == SHAPE_H5_6)  OLED_ShowString(2, 4, "{5,6}");
					else if (shape == SHAPE_H6_5)  OLED_ShowString(2, 4, "{6,5}");
					else if (shape == SHAPE_H7_3)  OLED_ShowString(2, 4, "{7,3}");
					else if (shape == SHAPE_H8_3)  OLED_ShowString(2, 4, "{8,3}");
					else if (shape == SHAPE_H3_10) OLED_ShowString(2, 4, "{3,10}");
					else if (shape == SHAPE_H10_3) OLED_ShowString(2, 4, "{10,3}");
					else if (shape == SHAPE_5_CELL)  OLED_ShowString(2, 4, "5CELL");
					else if (shape == SHAPE_TESSERACT) OLED_ShowString(2, 4, "TESS");
					else if (shape == SHAPE_16_CELL) OLED_ShowString(2, 4, "16CEL");
					else if (shape == SHAPE_24_CELL) OLED_ShowString(2, 4, "24CEL");
					else if (shape == SHAPE_600_CELL) OLED_ShowString(2, 4, "600CEL");
					else if (shape == SHAPE_P600_STAR1) OLED_ShowString(2, 4, "S1{5,5/2,5}");
					else if (shape == SHAPE_P600_STAR2) OLED_ShowString(2, 4, "S2{3,3,5/2}");
					else if (shape == SHAPE_P600_STAR3) OLED_ShowString(2, 4, "S3{3,5/2,3}");
					else                                OLED_ShowString(2, 4, "S4{5/2,3,5/2}");
					Delay_ms(400);
					lastTick = TIM2->CNT;
				}

				if (keyNum == 4)  /* PA4: 上一个图形 */
				{
					shape = (shape + TOTAL_SHAPE_COUNT - 1) % TOTAL_SHAPE_COUNT;
					OLED_Clear();
					if (shape == SHAPE_CUBE)            OLED_ShowString(2, 4, "CUBE");
					else if (shape == SHAPE_OCTAHEDRON)  OLED_ShowString(2, 4, "OCTA");
					else if (shape == SHAPE_TETRAHEDRON) OLED_ShowString(2, 4, "TETRA");
					else if (shape == SHAPE_DODECAHEDRON) OLED_ShowString(2, 4, "DODEC");
					else if (shape == SHAPE_ICOSAHEDRON)  OLED_ShowString(2, 4, "ICOSA");
					else if (shape == SHAPE_CUBOCTAHEDRON) OLED_ShowString(2, 4, "CUBOCT");
					else if (shape == SHAPE_TRUNCATED_TETRA) OLED_ShowString(2, 4, "TRTET");
					else if (shape == SHAPE_SMALL_STELLATED) OLED_ShowString(2, 4, "SSTEL");
					else if (shape == SHAPE_GREAT_STELLATED) OLED_ShowString(2, 4, "GSTEL");
					else if (shape == SHAPE_H3_7)  OLED_ShowString(2, 4, "{3,7}");
					else if (shape == SHAPE_H3_8)  OLED_ShowString(2, 4, "{3,8}");
					else if (shape == SHAPE_H4_5)  OLED_ShowString(2, 4, "{4,5}");
					else if (shape == SHAPE_H5_4)  OLED_ShowString(2, 4, "{5,4}");
					else if (shape == SHAPE_H4_6)  OLED_ShowString(2, 4, "{4,6}");
					else if (shape == SHAPE_H6_4)  OLED_ShowString(2, 4, "{6,4}");
					else if (shape == SHAPE_H5_5)  OLED_ShowString(2, 4, "{5,5}");
					else if (shape == SHAPE_H5_6)  OLED_ShowString(2, 4, "{5,6}");
					else if (shape == SHAPE_H6_5)  OLED_ShowString(2, 4, "{6,5}");
					else if (shape == SHAPE_H7_3)  OLED_ShowString(2, 4, "{7,3}");
					else if (shape == SHAPE_H8_3)  OLED_ShowString(2, 4, "{8,3}");
					else if (shape == SHAPE_H3_10) OLED_ShowString(2, 4, "{3,10}");
					else if (shape == SHAPE_H10_3) OLED_ShowString(2, 4, "{10,3}");
					else if (shape == SHAPE_5_CELL)  OLED_ShowString(2, 4, "5CELL");
					else if (shape == SHAPE_TESSERACT) OLED_ShowString(2, 4, "TESS");
					else if (shape == SHAPE_16_CELL) OLED_ShowString(2, 4, "16CEL");
					else if (shape == SHAPE_24_CELL) OLED_ShowString(2, 4, "24CEL");
					else if (shape == SHAPE_600_CELL) OLED_ShowString(2, 4, "600CEL");
					else if (shape == SHAPE_P600_STAR1) OLED_ShowString(2, 4, "S1{5,5/2,5}");
					else if (shape == SHAPE_P600_STAR2) OLED_ShowString(2, 4, "S2{3,3,5/2}");
					else if (shape == SHAPE_P600_STAR3) OLED_ShowString(2, 4, "S3{3,5/2,3}");
					else                                OLED_ShowString(2, 4, "S4{5/2,3,5/2}");
					Delay_ms(400);
					lastTick = TIM2->CNT;
				}

				if (dirChanged)
				{
					ShowDirPage(rotationDir);
					Delay_ms(400);
					dirChanged = 0;
					lastTick = TIM2->CNT;
				}

				curTick = TIM2->CNT;
				dtSec = (float)(curTick - lastTick) / TIM2_CLOCK_HZ;
				if (dtSec > DT_MAX_SEC) dtSec = DT_MAX_SEC;
				if (dtSec < DT_MIN_SEC) dtSec = DT_MIN_SEC;
				lastTick = curTick;

				Attitude_Update(dtSec);
				attitude = Attitude_Get();

				Cube3D_Render(-attitude.PitchDeg * rotationDir,
				              -attitude.RollDeg * rotationDir,
				               attitude.YawDeg * rotationDir,
				               shape, dtSec);
			}
				}
				else  /* 2D — 三角函数 / 指数函数 */
				{
					const uint8_t *trig[] = {HZK_4E09, HZK_89D2, HZK_51FD, HZK_6570};
					const uint8_t *expn[] = {HZK_6307, HZK_6570, HZK_51FD, HZK_6570};
					uint8_t cur2d = 0;
					OLED_Clear();
					OLED_ShowString(2, 1, ">");
					OLED_ShowChineseStr(2, 2, trig, 4);
					OLED_ShowString(3, 1, " ");
					OLED_ShowChineseStr(3, 2, expn, 4);

					while (1)
					{
						keyNum = Key_GetNum();
						if (keyNum == 2)  /* PA2: 返回子菜单 */
							break;
						if (keyNum == 3 || keyNum == 4)  /* 上下切换 */
						{
							cur2d = cur2d ? 0 : 1;
							OLED_ShowString(2, 1, cur2d == 0 ? ">" : " ");
							OLED_ShowString(3, 1, cur2d == 0 ? " " : ">");
						}
						if (keyNum == 1)  /* 確認 */
						{
							if (cur2d == 0)  /* 三角函數 — y=sin(t) */
							{
								float t0 = 0.0f;
								int16_t x;
								float tVal, yVal;
								int16_t px, py;

								while (1)
								{
									OLED_ClearBuffer();
									/* y轴(竖) + 上箭头 */
									OLED_DrawLine(32, 0, 32, 63, 1);
									OLED_DrawLine(31, 5, 32,  0, 1);
									OLED_DrawLine(33, 5, 32,  0, 1);
									/* t轴(横) + 右箭头 */
									OLED_DrawLine( 0,32,127, 32, 1);
									OLED_DrawLine(122,31,127, 32, 1);
									OLED_DrawLine(122,33,127, 32, 1);

									for (x = 0; x < 128; x++)
									{
										tVal = t0 + (float)(x - 32) / 10.0f;
										yVal = sinf(tVal);
										px = x;
										py = 32 - (int16_t)(yVal * 20.0f);
										OLED_DrawPoint(px, py, 1);
									}
									/* 标签 t, y (写入缓冲，随 Refresh 刷新) */
									OLED_ShowCharBuf(1, 4, 'y');
									OLED_ShowCharBuf(3, 15, 't');
									OLED_Refresh();

									if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
										t0 -= 0.01f;
									if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0)
										t0 += 0.01f;
									if (Key_GetNum() == 2)
										break;

									Delay_ms(10);
								}
							}
							else
							{
								OLED_Clear();
								OLED_ShowChineseStr(2, 3, expn, 4);
								OLED_ShowString(3, 3, "WIP");
								while (Key_GetNum() == 0);
								Delay_ms(200);
							}
							OLED_Clear();
							OLED_ShowString(2, 1, cur2d == 0 ? ">" : " ");
							OLED_ShowChineseStr(2, 2, trig, 4);
							OLED_ShowString(3, 1, cur2d == 0 ? " " : ">");
							OLED_ShowChineseStr(3, 2, expn, 4);
						}
						Delay_ms(10);
					}
				}
				}  /* sub-menu while(1) */
			}
			else if (menuRet == 1)  /* 动画 */
		{
			Animation_Show();
		}
	}
}
