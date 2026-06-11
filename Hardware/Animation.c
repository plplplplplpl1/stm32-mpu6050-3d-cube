/**
  ******************************************************************************
  * @file    Animation.c
  * @brief   多动画管理：切换逻辑完全对标 水平仪→3D图形
  *          OLED_Clear + 直接写 GDDRAM + 短延时，无帧缓冲/无init-stop开销
  ******************************************************************************
  */
#include "Animation.h"
#include "CatAnimation.h"
#include "OLED.h"
#include "FontCN.h"
#include "Delay.h"

/* ── 动画描述符 ──────────────────────────── */
typedef struct {
    const uint8_t **name;     /* 中文名称（字模数组） */
    uint8_t        nameLen;   /* 名称字数 */
    void          (*init)(void);
    uint8_t       (*play)(void);  /* 返回: 2=菜单, 3=下一切, 4=上一切 */
    void          (*exit)(void);  /* 完全退出（返回菜单） */
} AnimDesc_t;

static const uint8_t *g_catName[] = {HZK_YUE, HZK_XIN, HZK_MAO};

static const AnimDesc_t g_anims[ANIM_COUNT] = {
    { g_catName, 3, CatAnimation_Init, CatAnimation_Play,
      CatAnimation_Exit },  /* 月薪猫 */
};

/**
  * @brief  显示动画名称标签（对标形状切换：OLED_Clear + 直接写 + 400ms）
  */
static void Anim_ShowLabel(uint8_t idx)
{
    uint8_t col = (8 - g_anims[idx].nameLen) / 2 + 1;
    OLED_Clear();
    OLED_ShowChineseStr(2, col, g_anims[idx].name, g_anims[idx].nameLen);
    Delay_ms(400);
}

/**
  * @brief  动画播放主循环
  */
void Animation_Show(void)
{
    uint8_t curId = 0;
    uint8_t key;

    g_anims[curId].init();
    Anim_ShowLabel(curId);

    while (1)
    {
        key = g_anims[curId].play();

        if (key == 2)
        {
            g_anims[curId].exit();
            break;
        }
        else if (key == 3)
        {
            curId = (curId + 1) % ANIM_COUNT;
            Anim_ShowLabel(curId);
        }
        else if (key == 4)
        {
            curId = (curId + ANIM_COUNT - 1) % ANIM_COUNT;
            Anim_ShowLabel(curId);
        }
    }
}
