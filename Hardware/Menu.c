#include "Menu.h"
#include "OLED.h"
#include "Key.h"
#include "Delay.h"
#include "FontCN.h"

#define MENU_COUNT      4
#define ITEMS_PER_PAGE  3

static const uint8_t *g_menuCN[MENU_COUNT][4] = {
	{HZK_6C34, HZK_5E73, HZK_4EEA, 0},                 /* 水平仪 */
	{HZK_YUE, HZK_XIN, HZK_MAO, 0},                     /* 月薪猫 */
	{HZK_6E29, HZK_5EA6, HZK_76D1, HZK_6D4B},           /* 温度监测 */
	{HZK_8BA1, HZK_6B65, HZK_5668, 0}                   /* 计步器 */
};
static const uint8_t g_menuCNLen[MENU_COUNT] = {3, 3, 4, 3};
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
