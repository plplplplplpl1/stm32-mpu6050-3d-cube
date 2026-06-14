#ifndef __W25Q64_LAYOUT_H
#define __W25Q64_LAYOUT_H

/* ================================================================
 * W25Q64 8MB NOR Flash 分区布局
 * ================================================================ */

/* ── 月薪猫动画帧: 28帧 × 1024B ── */
#define W25Q_CATFRAMES_ADDR         0x000000UL
#define W25Q_CATFRAMES_SIZE         0x007000UL  /* 28KB */
#define W25Q_CATFRAME_COUNT         28
#define W25Q_CATFRAME_SIZE          1024

/* ── 3D/4D 图形数据 (顶点+边) ── */
#define W25Q_SHAPES_ADDR            0x007000UL
#define W25Q_SHAPES_SIZE            0x010000UL  /* 64KB */

/* ── 汉字字库 (16×16点阵) ── */
#define W25Q_FONT_CN_ADDR           0x017000UL
#define W25Q_FONT_CN_SIZE           0x004000UL  /* 16KB */
#define W25Q_FONT_CN_CHAR_SIZE      32          /* 每字 32 字节 */

/* ── OLED ASCII 字库 (8×16) ── */
#define W25Q_FONT_ASCII_ADDR        0x01B000UL
#define W25Q_FONT_ASCII_SIZE        0x001000UL  /* 4KB */
#define W25Q_FONT_ASCII_CHAR_SIZE   16          /* 每字 16 字节 */

/* ── 陀螺仪校准数据 (可读写) ── */
#define W25Q_CALIB_ADDR             0x01C000UL
#define W25Q_CALIB_SIZE             0x001000UL  /* 4KB */

/* ── 蟑螂动画帧: 28帧 × 1024B ── */
#define W25Q_COCKROACHFRAMES_ADDR   0x01D000UL
#define W25Q_COCKROACHFRAMES_SIZE   0x007000UL  /* 28KB */
#define W25Q_COCKROACHFRAME_COUNT   28
#define W25Q_COCKROACHFRAME_SIZE    1024

/* ── 预留扩展 ── */
#define W25Q_RESERVED_ADDR          0x024000UL

#endif
