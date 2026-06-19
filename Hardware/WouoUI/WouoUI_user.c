#include "WouoUI_user.h"
#include "WouoUI.h"
#include "Key.h"
#include "Delay.h"

static uint8_t g_result = 0xFF;

/* ── 主菜单4项 ── */
#define MAIN_NUM 4
static Option g_mainOpts[MAIN_NUM];
static const char *g_mainTexts[MAIN_NUM] = {
	"  3D&2D", "  Anime", "  Temp.", "  Step.",
};
static ListPage g_mainPage;

/* ── 子菜单3项 ── */
#define SUB_NUM 3
static Option g_subOpts[SUB_NUM];
static const char *g_subTexts[SUB_NUM] = {
	"  3D Mode", "  2D Mode", "  Return",
};
static ListPage g_subPage;

/* 编码器→WouoUI消息 */
static void FeedKeys(void)
{
	uint8_t k = Key_GetNum();
	switch (k) {
	case 3: WOUOUI_MSG_QUE_SEND(msg_up);     break;
	case 4: WOUOUI_MSG_QUE_SEND(msg_down);   break;
	case 1: WOUOUI_MSG_QUE_SEND(msg_click);  break;
	case 2: WOUOUI_MSG_QUE_SEND(msg_return); break;
	}
}

static bool MainReact(const Page *page, InputMsg msg)
{
	ListPage *lp = (ListPage *)page;
	if (msg == msg_up) {
		lp->select_item = (lp->select_item == 0) ? lp->item_num - 1 : lp->select_item - 1;
		WouoUI_ListPageLastItem(lp);  /* 重置动画 */
		return false;
	}
	if (msg == msg_down) {
		lp->select_item = (lp->select_item + 1) % lp->item_num;
		WouoUI_ListPageNextItem(lp);
		return false;
	}
	if (msg == msg_click) {
		g_result = lp->select_item; return false;
	}
	if (msg == msg_return) { g_result = 0xFF; return false; }
	return false;
}

static bool SubReact(const Page *page, InputMsg msg)
{
	ListPage *lp = (ListPage *)page;
	if (msg == msg_up) {
		lp->select_item = (lp->select_item == 0) ? lp->item_num - 1 : lp->select_item - 1;
		WouoUI_ListPageLastItem(lp);
		return false;
	}
	if (msg == msg_down) {
		lp->select_item = (lp->select_item + 1) % lp->item_num;
		WouoUI_ListPageNextItem(lp);
		return false;
	}
	if (msg == msg_click) {
		g_result = lp->select_item; return false;
	}
	if (msg == msg_return) { g_result = 2; return false; }
	return false;
}

void TestUI_Init(void)
{
	uint8_t i;
	for (i = 0; i < MAIN_NUM; i++) g_mainOpts[i].text = (String)g_mainTexts[i];
	WouoUI_ListPageInit(&g_mainPage, MAIN_NUM, g_mainOpts, NULL, &MainReact);
	g_mainPage.page.auto_deal_with_msg = false;  /* 关自动处理，回调控制一切 */
	for (i = 0; i < SUB_NUM;  i++) g_subOpts[i].text  = (String)g_subTexts[i];
	WouoUI_ListPageInit(&g_subPage,  SUB_NUM,  g_subOpts,  NULL, &SubReact);
	g_subPage.page.auto_deal_with_msg = false;
}

uint8_t WouoUI_MenuRun(void)
{
	uint8_t i;
	g_result = 0xFF;
	WouoUI_JumpToPage(&g_mainPage, NULL);
	/* 先跑完进入动画（~20帧）*/
	for (i = 0; i < 30; i++) WouoUI_Proc(5);
	while (g_result == 0xFF) { FeedKeys(); WouoUI_Proc(5); }
	return g_result;
}

uint8_t WouoUI_SubMenuRun(void)
{
	uint8_t i;
	g_result = 0xFF;
	WouoUI_JumpToPage(&g_subPage, NULL);
	for (i = 0; i < 30; i++) WouoUI_Proc(5);
	while (g_result == 0xFF) { FeedKeys(); WouoUI_Proc(5); }
	return g_result;
}
