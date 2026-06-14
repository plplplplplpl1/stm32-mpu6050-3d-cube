#ifndef __FONTCN_H
#define __FONTCN_H

#include "stm32f10x.h"

/* === Existing characters (from main.c, tested on OLED) === */
extern const uint8_t HZK_FAN[32];

/* === New characters (SimSun 12pt) === */
extern const uint8_t HZK_4E3B[32];
extern const uint8_t HZK_83DC[32];
extern const uint8_t HZK_5355[32];
extern const uint8_t HZK_529F[32];
extern const uint8_t HZK_80FD[32];
extern const uint8_t HZK_9009[32];
extern const uint8_t HZK_62E9[32];
extern const uint8_t HZK_6C34[32];
extern const uint8_t HZK_5E73[32];
extern const uint8_t HZK_4EEA[32];
extern const uint8_t HZK_7535[32];
extern const uint8_t HZK_5B50[32];
extern const uint8_t HZK_7F57[32];
extern const uint8_t HZK_76D8[32];
extern const uint8_t HZK_6E29[32];
extern const uint8_t HZK_5EA6[32];
extern const uint8_t HZK_76D1[32];
extern const uint8_t HZK_6D4B[32];
extern const uint8_t HZK_8BA1[32];
extern const uint8_t HZK_6B65[32];
extern const uint8_t HZK_5668[32];
extern const uint8_t HZK_5F85[32];
extern const uint8_t HZK_5F00[32];
extern const uint8_t HZK_53D1[32];
extern const uint8_t HZK_4E0D[32];
extern const uint8_t HZK_53EF[32];
extern const uint8_t HZK_7528[32];
extern const uint8_t HZK_6309[32];
extern const uint8_t HZK_952E[32];
extern const uint8_t HZK_8FD4[32];
extern const uint8_t HZK_56DE[32];
extern const uint8_t HZK_4F20[32];
extern const uint8_t HZK_611F[32];
extern const uint8_t HZK_6B63[32];
extern const uint8_t HZK_5E38[32];
extern const uint8_t HZK_9519[32];
extern const uint8_t HZK_8BEF[32];
extern const uint8_t HZK_68C0[32];
extern const uint8_t HZK_67E5[32];
extern const uint8_t HZK_63A5[32];
extern const uint8_t HZK_7EBF[32];
extern const uint8_t HZK_65B9[32];
extern const uint8_t HZK_5411[32];
extern const uint8_t HZK_56FE[32];
extern const uint8_t HZK_5F62[32];
extern const uint8_t HZK_9000[32];
extern const uint8_t HZK_51FA[32];
extern const uint8_t HZK_6821[32];
extern const uint8_t HZK_662F[32];
extern const uint8_t HZK_5426[32];

/* === 月薪猫 (2026-06-09) === */
extern const uint8_t HZK_YUE[32];
extern const uint8_t HZK_XIN[32];
extern const uint8_t HZK_MAO[32];

/* === 动画菜单 (2026-06-11) === */
extern const uint8_t HZK_52A8[32];  /* 动 */
extern const uint8_t HZK_753B[32];  /* 画 */

/* === 蟑螂动画 (2026-06-14) === */
extern const uint8_t HZK_87E5[32];  /* 蟑 */
extern const uint8_t HZK_8782[32];  /* 螂 */
extern const uint8_t HZK_4E09[32];  /* 三 */
extern const uint8_t HZK_89D2[32];  /* 角 */
extern const uint8_t HZK_51FD[32];  /* 函 */
extern const uint8_t HZK_5207[32];  /* 切 */
extern const uint8_t HZK_6570[32];  /* 数 */
extern const uint8_t HZK_6307[32];  /* 指 */

void OLED_ShowChineseStr(uint8_t Line, uint8_t Column, const uint8_t **Fonts, uint8_t Count);

#endif
