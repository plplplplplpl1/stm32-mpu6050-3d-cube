#include "CockroachAnimation.h"
#include "CockroachFrames.h"
#include "OLED.h"
#include "LED.h"
#include "Key.h"
#include "Delay.h"
#include "W25Q64.h"
#include "W25Q64_Layout.h"

/**
  * @brief  初始化蟑螂动画（点亮红色LED）
  */
void CockroachAnimation_Init(void)
{
    LED1_ON();
}

/**
  * @brief  播放蟑螂动画循环
  * @note   从 W25Q64 读取帧数据，直接写 SSD1306 GDDRAM
  * @retval 返回触发退出的按键: 2=KEY2菜单, 3=KEY3下一, 4=KEY4上一
  */
uint8_t CockroachAnimation_Play(void)
{
    uint16_t frame = 0;
    uint8_t page;
    uint8_t exitKey = 0;
    uint8_t tick;
    uint8_t key;
    uint8_t frameBuf[COCKROACH_FRAME_SIZE];

    while (!exitKey)
    {
        /* 从 W25Q64 读取当前帧 */
        W25Q64_ReadData(W25Q_COCKROACHFRAMES_ADDR + (uint32_t)frame * COCKROACH_FRAME_SIZE,
                        frameBuf, COCKROACH_FRAME_SIZE);

        /* 逐页写 GDDRAM */
        for (page = 0; page < 8; page++)
        {
            OLED_SetCursor(page, 0);
            OLED_WriteDataBurst(&frameBuf[page * COCKROACH_FRAME_WIDTH], COCKROACH_FRAME_WIDTH);
        }

        frame = (frame + 1) % COCKROACH_FRAME_COUNT;

        /* 分包延时 + 按键轮询 */
        for (tick = 0; tick < COCKROACH_FRAME_DELAY_MS / 10; tick++)
        {
            Delay_ms(10);
            key = Key_GetNum();
            if (key == 2 || key == 3 || key == 4)
            {
                exitKey = key;
                break;
            }
        }
    }
    return exitKey;
}

/**
  * @brief  退出蟑螂动画（关LED，重初始化OLED）
  */
void CockroachAnimation_Exit(void)
{
    LED1_OFF();
    OLED_Init();
}
