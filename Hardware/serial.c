/**
  ******************************************************************************
  * @file    serial.c
  * @brief   USART2 串口 + W25Q64 镜像烧录
  *          PA9=TX, PA10=RX, 115200-8-N-1
  *          菜单项触发: 选中后 STM32 发 READY, PC 发镜像, OLED 显示进度
  ******************************************************************************
  */
#include "serial.h"
#include "W25Q64.h"
#include "W25Q64_Layout.h"
#include "OLED.h"
#include "Delay.h"

/* ── USART2 初始化 ──────────────────────────── */

void Serial_Init(void)
{
    GPIO_InitTypeDef  gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    /* PA9=TX(USART1, AF_PP) */
    gpio.GPIO_Pin   = GPIO_Pin_9;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    /* PA3=RX(USART2, IN_FLOATING) */
    gpio.GPIO_Pin   = GPIO_Pin_3;
    gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);

    /* USART1 TX only */
    USART_StructInit(&usart);
    usart.USART_BaudRate            = 115200;
    usart.USART_WordLength          = USART_WordLength_8b;
    usart.USART_StopBits            = USART_StopBits_1;
    usart.USART_Parity              = USART_Parity_No;
    usart.USART_Mode                = USART_Mode_Tx;
    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);

    /* USART2 RX only */
    usart.USART_Mode                = USART_Mode_Rx;
    USART_Init(USART2, &usart);
    USART_Cmd(USART2, ENABLE);
}

/* ── 基础收发 ──────────────────────────────── */

void Serial_SendByte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, byte);
}

void Serial_SendString(const char *str)
{
    while (*str) Serial_SendByte((uint8_t)*str++);
}

/**
  * @brief  快速轮询接收一个字节 (无 Delay, ~5us 粒度)
  * @param  timeout_ms: 超时毫秒数
  * @return 收到的字节, 超时返回 0
  */
uint8_t Serial_RecvByte(uint32_t timeout_ms)
{
    uint32_t ms = 0;
    while (ms < timeout_ms)
    {
        uint32_t i;
        for (i = 0; i < 7200; i++)
        {
            if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
                return (uint8_t)USART_ReceiveData(USART2);
        }
        ms++;
    }
    return 0;
}

/* ── W25Q64 镜像烧录 (菜单项触发) ───────────── */

/**
  * @brief  菜单触发的 W25Q64 烧录流程
  * @note   OLED 显示进度, USART2 接收数据
  *         协议: STM32 发 READY → PC 发 size_hex\n → STM32 发 OK → PC 发 raw → STM32 发 DONE
  */
void Serial_FlashBurn(void)
{
    uint8_t  buf[4096];
    uint32_t totalSize = 0;
    uint32_t received  = 0;
    uint32_t addr;
    uint16_t chunk;
    uint16_t i;
    uint8_t  c;
    char     line[16];

    /* ── 握手: 反复发 READY, 等 PC 发 size ── */
    OLED_ShowString(2, 1, "FLASH: wait 3s");
    OLED_ShowString(3, 1, "Run serial_flash");

    /* RX 诊断: 直接读 PA10 GPIO 电平, 显示在 OLED 上 */
    {
        uint8_t level = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3);
        char dbg[6];
        dbg[0] = 'R'; dbg[1] = 'X'; dbg[2] = '=';
        dbg[3] = level ? 'H' : 'L';
        dbg[4] = ' '; dbg[5] = '\0';
        OLED_ShowString(4, 1, dbg);
    }

    {
        uint8_t  idx = 0;
        uint8_t  gotSize = 0;
        uint32_t endTime = 3000;

        /* 每 200ms 发一次 READY, 持续 3 秒 */
        while (endTime > 0 && !gotSize)
        {
            Serial_SendString("READY\n");

            /* 快速扫描 RX, 200ms */
            {
                uint32_t rxMs = 200;
                while (rxMs-- && !gotSize)
                {
                    c = Serial_RecvByte(1);
                    endTime--;
                    if (c == 0) continue;

                    if (c == '\n')
                    {
                        line[idx] = '\0';
                        totalSize = 0;
                        {
                            const char *p = line;
                            while (*p)
                            {
                                totalSize <<= 4;
                                if      (*p >= '0' && *p <= '9') totalSize += (uint32_t)(*p - '0');
                                else if (*p >= 'A' && *p <= 'F') totalSize += (uint32_t)(*p - 'A' + 10);
                                else if (*p >= 'a' && *p <= 'f') totalSize += (uint32_t)(*p - 'a' + 10);
                                p++;
                            }
                        }
                        if (totalSize > 0 && totalSize <= 256 * 1024)
                        {
                            gotSize = 1;
                            break;
                        }
                        idx = 0;
                    }
                    else if (idx < 14)
                    {
                        line[idx++] = (char)c;
                    }
                }
            }
        }

        if (!gotSize)
        {
            OLED_ShowString(3, 1, "PC timeout!");
            Delay_ms(1000);
            return;
        }
    }

    Serial_SendString("OK\n");
    OLED_ShowString(2, 1, "Erasing...    ");

    /* ── 擦除 ── */
    {
        uint32_t endAddr = (totalSize + 4095) & ~4095UL;
        if (endAddr > W25Q_RESERVED_ADDR) endAddr = W25Q_RESERVED_ADDR;

        for (addr = 0; addr < endAddr; addr += 4096)
        {
            W25Q64_SectorErase(addr);
        }
    }

    Serial_SendString("GO\n");
    OLED_ShowString(2, 1, "Receiving...  ");

    /* ── 接收 + 编程: 每 4KB 发 ACK 流控 ── */
    addr = 0;
    while (received < totalSize)
    {
        uint16_t blk = 4096;
        uint16_t pg, pgOff;
        if ((uint32_t)blk > totalSize - received)
            blk = (uint16_t)(totalSize - received);

        /* 接收整个 4KB 块 */
        for (i = 0; i < blk; i++)
        {
            while (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET);
            buf[i] = (uint8_t)USART_ReceiveData(USART2);
        }

        /* 按 256B 页编程 */
        for (pg = 0; pg < blk; pg += 256)
        {
            uint16_t pgLen = (blk - pg < 256) ? (blk - pg) : 256;
            W25Q64_PageProgram(addr + pg, &buf[pg], pgLen);
        }

        addr     += blk;
        received += blk;

        /* 进度 */
        {
            uint8_t pct = (uint8_t)(received * 100 / totalSize);
            char str[4];
            str[0] = (char)('0' + pct / 10);
            str[1] = (char)('0' + pct % 10);
            str[2] = '%';
            str[3] = '\0';
            OLED_ShowString(3, 1, str);
        }

        Serial_SendString("ACK\n");
    }

    Serial_SendString("DONE\n");
    OLED_ShowString(2, 1, "Burn OK!Reset");
    OLED_ShowString(3, 1, "              ");
    while (1); /* 等待复位 */
}
