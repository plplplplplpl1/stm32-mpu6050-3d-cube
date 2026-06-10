#include "CatAnimation.h"
#include "CatFrames.h"
#include "OLED.h"
#include "LED.h"
#include "Key.h"
#include "Delay.h"
#include "W25Q64.h"

/**
  * @brief  初始化月薪猫动画（点亮红色LED）
  * @param  无
  * @retval 无
  */
void CatAnimation_Init(void)
{
    LED1_ON();
}

/**
  * @brief  播放月薪猫动画循环
  * @note   从 W25Q64 读取帧数据，绕开 OLED_GRAM 直接写 GDDRAM
  *         按键KEY2（PA2）退出，返回菜单
  * @param  无
  * @retval 无
  */
void CatAnimation_Play(void)
{
    uint16_t frame = 0;
    uint8_t page;
    uint8_t exitReq = 0;
    uint8_t tick;
    uint8_t frameBuf[CAT_FRAME_SIZE];  /* 1024B 栈缓冲 */

    while (!exitReq)
    {
        /* 从 W25Q64 读取当前帧 */
        W25Q64_ReadData(W25Q_CATFRAMES_ADDR + (uint32_t)frame * CAT_FRAME_SIZE,
                        frameBuf, CAT_FRAME_SIZE);

        /* 逐页写 GDDRAM（直接 I2C 突发传输） */
        for (page = 0; page < 8; page++)
        {
            OLED_SetCursor(page, 0);
            OLED_WriteDataBurst(&frameBuf[page * CAT_FRAME_WIDTH], CAT_FRAME_WIDTH);
        }

        /* 循环帧号 */
        frame = (frame + 1) % CAT_FRAME_COUNT;

        /* 分包延时 + 按键轮询 */
        for (tick = 0; tick < CAT_FRAME_DELAY_MS / 10; tick++)
        {
            Delay_ms(10);
            if (Key_GetNum() == 2)
            {
                exitReq = 1;
                break;
            }
        }
    }
}

/**
  * @brief  退出月薪猫动画（关LED，清屏）
  * @param  无
  * @retval 无
  */
void CatAnimation_Exit(void)
{
    LED1_OFF();
    /* OLED_Init 会复位I2C总线并重发全部SSD1306配置命令（包含0xAF开启显示、0x8D电荷泵使能）。
       猫动画期间大量像素点亮可能导致SSD1306电荷泵电压跌落→控制器复位→显示关闭，
       OLED_Clear 不够——它只清GRAM+刷新，不会恢复被复位打乱的寄存器。*/
    OLED_Init();
}
