#include "Menu.h"
#include "OLED.h"
#include "Key.h"
#include "Delay.h"
#include "FontCN.h"

#define MENU_COUNT      4
#define ITEMS_PER_PAGE  3

static const uint8_t *g_menuCN[MENU_COUNT][4] = {
	{0, 0, 0, 0},                                           /* "3D&2D" (ASCII) */
	{HZK_52A8, HZK_753B, 0, 0},                           /* 动画 */
	{HZK_6E29, HZK_5EA6, HZK_76D1, HZK_6D4B},           /* 温度监测 */
	{HZK_8BA1, HZK_6B65, HZK_5668, 0}                   /* 计步器 */
};
static const uint8_t g_menuCNLen[MENU_COUNT] = {0, 2, 4, 3};
static const char    *g_menuASCII[MENU_COUNT] = {"3D&2D", 0, 0, 0};
static const uint8_t g_menuAvail[MENU_COUNT] = {1, 1, 0, 0};

void Menu_Init(void)
{
}

static void Menu_Redraw(uint8_t cursor)
{
	uint8_t i;
	const uint8_t *title[] = {HZK_4E3B, HZK_83DC, HZK_5355};  /* 主菜单 */

	OLED_Clear();
	OLED_ShowChineseStr(1, 3, title, 3);

	for (i = 0; i < ITEMS_PER_PAGE; i++)
	{
		OLED_ShowString(i + 2, 1, (i == cursor) ? ">" : " ");
		OLED_ShowChar(i + 2, 2, '1' + i);

		if (g_menuCNLen[i] == 0)
			OLED_ShowString(i + 2, 5, (char *)g_menuASCII[i]);
		else
			OLED_ShowChineseStr(i + 2, 3, g_menuCN[i], g_menuCNLen[i]);

		if (!g_menuAvail[i])
			OLED_ShowString(i + 2, 14, "x");
	}
}

uint8_t Menu_Show(void)
{
	uint8_t cursor = 0;
	uint8_t key;

	Menu_Redraw(cursor);

	while (1)
	{
		key = Key_GetNum();

		if (key == 3)
		{
			if (cursor > 0) { cursor--; Menu_Redraw(cursor); }
		}
		else if (key == 4)
		{
			if (cursor < MENU_COUNT - 1) { cursor++; Menu_Redraw(cursor); }
		}
		else if (key == 1)
		{
			if (g_menuAvail[cursor])
				return cursor;

			/* 待开发 / 按键返回 */
			{
				const uint8_t *na[] = {HZK_5F85, HZK_5F00, HZK_53D1};
				const uint8_t *pk[] = {HZK_6309, HZK_952E, HZK_8FD4, HZK_56DE};
				OLED_Clear();
				OLED_ShowChineseStr(2, 3, na, 3);
				OLED_ShowChineseStr(3, 2, pk, 4);
				while (Key_GetNum() == 0);
				Delay_ms(50);
				Menu_Redraw(cursor);
			}
		}

		Delay_ms(10);
	}
}

/**
  * @brief  3D&2D 子菜单
  * @retval 0=3D, 1=2D, 2=返回主菜单
  */
uint8_t Menu_Show3D2D(void)
{
	uint8_t cursor = 0;
	uint8_t key;

	OLED_Clear();
	OLED_ShowString(2, 5, ">1 3D");
	OLED_ShowString(3, 5, " 2 2D");

	while (1)
	{
		key = Key_GetNum();

		if (key == 3 || key == 4)  /* 上下切换 */
		{
			cursor = cursor ? 0 : 1;
			OLED_ShowString(2, 5, cursor == 0 ? ">1 3D" : " 1 3D");
			OLED_ShowString(3, 5, cursor == 0 ? " 2 2D" : ">2 2D");
		}
		else if (key == 1)  /* 确认 */
		{
			return cursor;
		}
		else if (key == 2)  /* 返回 */
		{
			return 2;
		}

		Delay_ms(10);
	}
}
