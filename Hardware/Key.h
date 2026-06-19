#ifndef __KEY_H
#define __KEY_H

void Key_Init(void);
uint8_t Key_GetNum(void);
int8_t Key_GetEncRaw(void);   /* 读取并清零编码器累加值（用于细粒度调节） */

#endif
