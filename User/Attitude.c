#include "Attitude.h"
#include "MPU6050.h"
#include "Delay.h"
#include <math.h>

/* MPU6050当前配置：陀螺仪 ±2000dps，对应灵敏度 16.4 LSB/(deg/s) */
#define GYRO_SENS_2000DPS       16.4f

/* 加速度计当前配置：±16g，对应灵敏度 2048 LSB/g */
#define ACC_SENS_16G            2048.0f

/* 高响应互补滤波：仅少量使用加速度修正，避免"二维锁死感" */
#define COMPLEMENTARY_ALPHA     0.999f

/* 统一角速度增益：提高旋转速度，使物理旋转与屏幕显示1:1 */
#define GYRO_GAIN               1.00f

/* 上电零偏标定样本数 */
#define GYRO_CALIB_SAMPLES      300

/* 角度弧度换算 */
#define RAD_TO_DEG              57.2957795f
#define DEFAULT_DT_SEC          0.01f
#define MIN_DENORM_EPS          0.0001f

static Attitude_t g_attitude;
static float g_gyroBiasXDps = 0.0f;
static float g_gyroBiasYDps = 0.0f;
static float g_gyroBiasZDps = 0.0f;
static float g_zeroPitchDeg = 0.0f;
static float g_zeroRollDeg = 0.0f;
static float g_zeroYawDeg = 0.0f;

/*
 * 注意：本模块内部使用非标准命名约定——
 *   "PitchDeg" 实际存储绕 X 轴的旋转（标准的 Roll）
 *   "RollDeg"  实际存储绕 Y 轴的旋转（标准的 Pitch）
 * 这是为了与 Cube3D 渲染的轴映射保持一致，请勿单独修改一处。
 */

static float WrapAngleDeg(float angleDeg)
{
	while (angleDeg > 180.0f)
	{
		angleDeg -= 360.0f;
	}
	while (angleDeg < -180.0f)
	{
		angleDeg += 360.0f;
	}
	return angleDeg;
}

void Attitude_CalibrateGyro(void)
{
	int32_t gxSum = 0, gySum = 0, gzSum = 0;
	int32_t axSum = 0, aySum = 0, azSum = 0;
	int16_t axRaw, ayRaw, azRaw;
	int16_t gxRaw, gyRaw, gzRaw;
	uint16_t i;
	float axG, ayG, azG;
	float denom;

	/* 静止时标定陀螺零偏，减轻漂移 */
	for (i = 0; i < GYRO_CALIB_SAMPLES; i++)
	{
		MPU6050_GetData(&axRaw, &ayRaw, &azRaw, &gxRaw, &gyRaw, &gzRaw);
		axSum += axRaw;
		aySum += ayRaw;
		azSum += azRaw;
		gxSum += gxRaw;
		gySum += gyRaw;
		gzSum += gzRaw;
		Delay_ms(2);
	}
	g_gyroBiasXDps = ((float)gxSum / GYRO_CALIB_SAMPLES) / GYRO_SENS_2000DPS;
	g_gyroBiasYDps = ((float)gySum / GYRO_CALIB_SAMPLES) / GYRO_SENS_2000DPS;
	g_gyroBiasZDps = ((float)gzSum / GYRO_CALIB_SAMPLES) / GYRO_SENS_2000DPS;

	/* 以当前静止姿态作为零点，消除安装倾角导致的固定偏移 */
	axG = ((float)axSum / GYRO_CALIB_SAMPLES) / ACC_SENS_16G;
	ayG = ((float)aySum / GYRO_CALIB_SAMPLES) / ACC_SENS_16G;
	azG = ((float)azSum / GYRO_CALIB_SAMPLES) / ACC_SENS_16G;
	g_zeroPitchDeg = atan2f(ayG, azG) * RAD_TO_DEG;
	denom = sqrtf(ayG * ayG + azG * azG);
	if (denom < MIN_DENORM_EPS)
	{
		g_zeroRollDeg = 0.0f;
	}
	else
	{
		g_zeroRollDeg = atan2f(-axG, denom) * RAD_TO_DEG;
	}
	g_zeroYawDeg = 0.0f;

	/* 运行时重标定后同步清零姿态，防止旧积分状态残留 */
	g_attitude.PitchDeg = 0.0f;
	g_attitude.RollDeg = 0.0f;
	g_attitude.YawDeg = 0.0f;
}

void Attitude_Init(void)
{
	g_attitude.PitchDeg = 0.0f;
	g_attitude.RollDeg = 0.0f;
	g_attitude.YawDeg = 0.0f;

	Attitude_CalibrateGyro();
}

void Attitude_Update(float dtSec)
{
	int16_t axRaw, ayRaw, azRaw;
	int16_t gxRaw, gyRaw, gzRaw;
	float axG, ayG, azG;
	float gxDps, gyDps, gzDps;
	float pitchAccDeg, rollAccDeg;
	float denom;

	/* 防御性保护：调用方若传入异常dt，回退到默认值 */
	if (dtSec <= 0.0f || dtSec > 0.05f)
	{
		dtSec = DEFAULT_DT_SEC;
	}

	MPU6050_GetData(&axRaw, &ayRaw, &azRaw, &gxRaw, &gyRaw, &gzRaw);

	/* 原始值转为工程量 */
	axG = (float)axRaw / ACC_SENS_16G;
	ayG = (float)ayRaw / ACC_SENS_16G;
	azG = (float)azRaw / ACC_SENS_16G;

	gxDps = (((float)gxRaw / GYRO_SENS_2000DPS) - g_gyroBiasXDps) * GYRO_GAIN;
	gyDps = (((float)gyRaw / GYRO_SENS_2000DPS) - g_gyroBiasYDps) * GYRO_GAIN;
	gzDps = (((float)gzRaw / GYRO_SENS_2000DPS) - g_gyroBiasZDps) * GYRO_GAIN;

	/*
	 * 加速度计静态姿态估计（本模块内部约定）：
	 *   "Pitch" ← 绕 X 轴旋转角，atan2(ay, az)
	 *   "Roll"  ← 绕 Y 轴旋转角，atan2(-ax, sqrt(ay^2+az^2))
	 */
	pitchAccDeg = atan2f(ayG, azG) * RAD_TO_DEG;
	denom = sqrtf(ayG * ayG + azG * azG);
	if (denom < MIN_DENORM_EPS)
	{
		/* 分母接近0时，维持上一帧参考，避免数值突跳 */
		rollAccDeg = g_attitude.RollDeg;
	}
	else
	{
		rollAccDeg = atan2f(-axG, denom) * RAD_TO_DEG;
	}

	/* 互补滤波：陀螺积分 + 加速度修正 */
	g_attitude.PitchDeg = COMPLEMENTARY_ALPHA * (g_attitude.PitchDeg + gxDps * dtSec)
						+ (1.0f - COMPLEMENTARY_ALPHA) * pitchAccDeg;
	g_attitude.RollDeg = COMPLEMENTARY_ALPHA * (g_attitude.RollDeg + gyDps * dtSec)
					   + (1.0f - COMPLEMENTARY_ALPHA) * rollAccDeg;

	/* Yaw完全使用陀螺积分，实现真正三维联动 */
	g_attitude.YawDeg += gzDps * dtSec;

	/* 角度约束：避免长时间运行后数值累积影响精度 */
	g_attitude.PitchDeg = WrapAngleDeg(g_attitude.PitchDeg);
	g_attitude.RollDeg = WrapAngleDeg(g_attitude.RollDeg);
	g_attitude.YawDeg = WrapAngleDeg(g_attitude.YawDeg);
}

Attitude_t Attitude_Get(void)
{
	Attitude_t output = g_attitude;
	output.PitchDeg = WrapAngleDeg(output.PitchDeg - g_zeroPitchDeg);
	output.RollDeg = WrapAngleDeg(output.RollDeg - g_zeroRollDeg);
	output.YawDeg = WrapAngleDeg(output.YawDeg - g_zeroYawDeg);
	return output;
}
