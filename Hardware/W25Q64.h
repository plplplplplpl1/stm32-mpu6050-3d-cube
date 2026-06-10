#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f10x.h"

/* W25Q64 引脚定义 — 硬件 SPI2 (PB12-PB15) */
#define W25Q64_CS_PORT      GPIOB
#define W25Q64_CS_PIN       GPIO_Pin_12
#define W25Q64_SCK_PORT     GPIOB
#define W25Q64_SCK_PIN      GPIO_Pin_13
#define W25Q64_MISO_PORT    GPIOB
#define W25Q64_MISO_PIN     GPIO_Pin_15  /* DO→PB15 */
#define W25Q64_MOSI_PORT    GPIOB
#define W25Q64_MOSI_PIN     GPIO_Pin_14  /* DI→PB14 */

/* W25Q64 指令集 */
#define W25Q64_CMD_WREN         0x06
#define W25Q64_CMD_WRDI         0x04
#define W25Q64_CMD_RDSR         0x05
#define W25Q64_CMD_WRSR         0x01
#define W25Q64_CMD_READ         0x03
#define W25Q64_CMD_FAST_READ    0x0B
#define W25Q64_CMD_PP           0x02
#define W25Q64_CMD_SE           0x20
#define W25Q64_CMD_BE32K        0x52
#define W25Q64_CMD_BE64K        0xD8
#define W25Q64_CMD_CE           0xC7
#define W25Q64_CMD_JEDEC_ID     0x9F
#define W25Q64_CMD_PD           0xB9
#define W25Q64_CMD_RDP          0xAB

/* W25Q64 参数 */
#define W25Q64_PAGE_SIZE        256
#define W25Q64_SECTOR_SIZE      4096
#define W25Q64_BLOCK32K_SIZE    32768
#define W25Q64_BLOCK64K_SIZE    65536
#define W25Q64_PAGE_COUNT       32768
#define W25Q64_CAPACITY         8388608

/* 状态寄存器位 */
#define W25Q64_SR_BUSY          0x01
#define W25Q64_SR_WEL           0x02

/* ── API ─────────────────────────────────── */

void W25Q64_Init(void);
uint8_t  W25Q64_ReadSR(void);
uint8_t  W25Q64_ReadSR1(void);
uint8_t  W25Q64_ReadSR2(void);
uint8_t  W25Q64_ReadSR3(void);
void     W25Q64_WaitBusy(void);
void     W25Q64_WriteEnable(void);
void     W25Q64_WriteDisable(void);
void     W25Q64_SectorErase(uint32_t addr);
void     W25Q64_BlockErase64K(uint32_t addr);
void     W25Q64_ChipErase(void);
void     W25Q64_PageProgram(uint32_t addr, const uint8_t *data, uint16_t len);
void     W25Q64_ReadData(uint32_t addr, uint8_t *buf, uint32_t len);
void     W25Q64_ReadJEDECID(uint8_t *mf, uint8_t *type, uint8_t *capacity);

#endif
