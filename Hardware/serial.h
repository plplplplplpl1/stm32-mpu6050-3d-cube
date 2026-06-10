#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

void Serial_Init(void);
void Serial_SendByte(uint8_t byte);
void Serial_SendString(const char *str);
uint8_t Serial_RecvByte(uint32_t timeout_ms);
void Serial_FlashBurn(void);

#endif
