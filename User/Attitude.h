#ifndef __ATTITUDE_H
#define __ATTITUDE_H

/*
 * 内部姿态定义（非标准航空约定）：
 *   PitchDeg — 绕 X 轴的旋转（通常对应 roll）
 *   RollDeg  — 绕 Y 轴的旋转（通常对应 pitch）
 *   YawDeg   — 绕 Z 轴的旋转
 * 此命名沿袭原版代码并与 Cube3D 渲染约定保持一致，请勿单独修改。
 */
typedef struct
{
	float PitchDeg;
	float RollDeg;
	float YawDeg;
} Attitude_t;

void Attitude_Init(void);
void Attitude_CalibrateGyro(void);
void Attitude_Update(float dtSec);
Attitude_t Attitude_Get(void);

#endif
