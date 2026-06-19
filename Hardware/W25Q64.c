/**
  ******************************************************************************
  * @file    W25Q64.c
  * @brief   W25Q64 软件 SPI — 对调 MISO/MOSI
  * @note    PB12=CS, PB13=SCK, PB14=MOSI(DI), PB15=MISO(DO)
  ******************************************************************************
  */
#include "W25Q64.h"
#include "Delay.h"

/* PB14=MOSI(DI), PB15=MISO(DO) */
static uint8_t sw_spi_rw(uint8_t tx)
{
    uint8_t i, rx=0;
    for(i=0;i<8;i++){
        if(tx&0x80) GPIOB->BSRR=GPIO_Pin_14; else GPIOB->BRR=GPIO_Pin_14;
        tx<<=1;
        GPIOB->BSRR=GPIO_Pin_13;              /* SCK↑ */
        rx<<=1; if(GPIOB->IDR&GPIO_Pin_15) rx|=1; /* 读 PB15=DO */
        GPIOB->BRR=GPIO_Pin_13;               /* SCK↓ */
    }
    return rx;
}

void W25Q64_Init(void)
{
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

    /* PB12/CS PB13/SCK PB14/MOSI → 推挽输出 */
    g.GPIO_Pin=GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14;
    g.GPIO_Mode=GPIO_Mode_Out_PP; g.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&g);

    /* PB15/MISO → 浮空输入 */
    g.GPIO_Pin=GPIO_Pin_15;
    g.GPIO_Mode=GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB,&g);

    GPIOB->BSRR=GPIO_Pin_12|GPIO_Pin_14;  /* CS=H MOSI=H */
    GPIOB->BRR=GPIO_Pin_13;               /* SCK=L */
}

uint8_t W25Q64_ReadSR(void){
    uint8_t r; GPIOB->BRR=GPIO_Pin_12;
    sw_spi_rw(0x05); r=sw_spi_rw(0xFF); GPIOB->BSRR=GPIO_Pin_12; return r;
}
uint8_t W25Q64_ReadSR1(void){ return W25Q64_ReadSR(); }
uint8_t W25Q64_ReadSR2(void){
    uint8_t r; GPIOB->BRR=GPIO_Pin_12;
    sw_spi_rw(0x35); r=sw_spi_rw(0xFF); GPIOB->BSRR=GPIO_Pin_12; return r;
}
uint8_t W25Q64_ReadSR3(void){
    uint8_t r; GPIOB->BRR=GPIO_Pin_12;
    sw_spi_rw(0x15); r=sw_spi_rw(0xFF); GPIOB->BSRR=GPIO_Pin_12; return r;
}
void W25Q64_WaitBusy(void){ while(W25Q64_ReadSR()&0x01); }
void W25Q64_WriteEnable(void){
    GPIOB->BRR=GPIO_Pin_12; sw_spi_rw(0x06); GPIOB->BSRR=GPIO_Pin_12;
}
void W25Q64_WriteDisable(void){
    GPIOB->BRR=GPIO_Pin_12; sw_spi_rw(0x04); GPIOB->BSRR=GPIO_Pin_12;
}
void W25Q64_ReadJEDECID(uint8_t*m,uint8_t*t,uint8_t*c){
    GPIOB->BRR=GPIO_Pin_12;
    sw_spi_rw(0x9F); *m=sw_spi_rw(0xFF); *t=sw_spi_rw(0xFF); *c=sw_spi_rw(0xFF);
    GPIOB->BSRR=GPIO_Pin_12;
}
void W25Q64_SectorErase(uint32_t a){
    W25Q64_WriteEnable(); GPIOB->BRR=GPIO_Pin_12;
    sw_spi_rw(0x20); sw_spi_rw((uint8_t)(a>>16)); sw_spi_rw((uint8_t)(a>>8)); sw_spi_rw((uint8_t)a);
    GPIOB->BSRR=GPIO_Pin_12; W25Q64_WaitBusy();
}
void W25Q64_BlockErase64K(uint32_t a){
    W25Q64_WriteEnable(); GPIOB->BRR=GPIO_Pin_12;
    sw_spi_rw(0xD8); sw_spi_rw((uint8_t)(a>>16)); sw_spi_rw((uint8_t)(a>>8)); sw_spi_rw((uint8_t)a);
    GPIOB->BSRR=GPIO_Pin_12; W25Q64_WaitBusy();
}
void W25Q64_ChipErase(void){
    W25Q64_WriteEnable(); GPIOB->BRR=GPIO_Pin_12; sw_spi_rw(0xC7); GPIOB->BSRR=GPIO_Pin_12; W25Q64_WaitBusy();
}
void W25Q64_PageProgram(uint32_t a,const uint8_t*d,uint16_t n){
    uint16_t i; if(!n||n>256)return;
    W25Q64_WriteEnable(); GPIOB->BRR=GPIO_Pin_12;
    sw_spi_rw(0x02); sw_spi_rw((uint8_t)(a>>16)); sw_spi_rw((uint8_t)(a>>8)); sw_spi_rw((uint8_t)a);
    for(i=0;i<n;i++) sw_spi_rw(d[i]); GPIOB->BSRR=GPIO_Pin_12; W25Q64_WaitBusy();
}
void W25Q64_ReadData(uint32_t a,uint8_t*b,uint32_t n){
    uint32_t i; GPIOB->BRR=GPIO_Pin_12;
    sw_spi_rw(0x03); sw_spi_rw((uint8_t)(a>>16)); sw_spi_rw((uint8_t)(a>>8)); sw_spi_rw((uint8_t)a);
    for(i=0;i<n;i++) b[i]=sw_spi_rw(0xFF); GPIOB->BSRR=GPIO_Pin_12;
}
