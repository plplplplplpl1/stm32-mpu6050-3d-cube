#include "Cube3D.h"
#include "OLED.h"
#include <math.h>

#define DEG_TO_RAD      0.0174532925f

/* OLED显示中心与透视参数 */
#define SCREEN_CX       64.0f
#define SCREEN_CY       32.0f
#define CAMERA_Z        100.0f
#define PROJ_SCALE      108.0f

/* 魔方线框半边长，单位可视作任意缩放单位 */
#define CUBE_HALF_SIZE  18.0f

typedef struct
{
	float x;
	float y;
	float z;
} Vec3f_t;

typedef struct
{
	int16_t x;
	int16_t y;
} Vec2i_t;

/* 立方体8个顶点 */
static const Vec3f_t kCubeVertices[8] =
{
	{-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE},
	{ CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE},
	{ CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE},
	{-CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE},
	{-CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE},
	{ CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE},
	{ CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE},
	{-CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE}
};

/* 立方体12条边，由顶点索引对组成 */
static const uint8_t kCubeEdges[12][2] =
{
	{0, 1}, {1, 2}, {2, 3}, {3, 0},
	{4, 5}, {5, 6}, {6, 7}, {7, 4},
	{0, 4}, {1, 5}, {2, 6}, {3, 7}
};

/*
 * 三维旋转：依次绕 X轴、Y轴、Z轴 旋转
 *   参数 pitchDeg — 绕 X 轴旋转角
 *   参数 rollDeg  — 绕 Y 轴旋转角
 * 所有旋转矩阵均为标准右手系 CCW（逆时针）方向。
 */
static Vec3f_t RotateXYZ(Vec3f_t v, float pitchDeg, float rollDeg, float yawDeg)
{
	float cx, sx, cy, sy, cz, sz;
	Vec3f_t r;
	float x1, y1, z1;
	float x2, y2, z2;

	cx = cosf(pitchDeg * DEG_TO_RAD);
	sx = sinf(pitchDeg * DEG_TO_RAD);
	cy = cosf(rollDeg * DEG_TO_RAD);
	sy = sinf(rollDeg * DEG_TO_RAD);
	cz = cosf(yawDeg * DEG_TO_RAD);
	sz = sinf(yawDeg * DEG_TO_RAD);

	/* X轴旋转 */
	x1 = v.x;
	y1 = v.y * cx - v.z * sx;
	z1 = v.y * sx + v.z * cx;

	/* Y轴旋转 */
	x2 = x1 * cy + z1 * sy;
	y2 = y1;
	z2 = -x1 * sy + z1 * cy;

	/* Z轴旋转 */
	r.x = x2 * cz - y2 * sz;
	r.y = x2 * sz + y2 * cz;
	r.z = z2;

	return r;
}

static Vec2i_t ProjectToScreen(Vec3f_t v)
{
	Vec2i_t p;
	float zCamera = v.z + CAMERA_Z;
	float factor;

	if (zCamera < 1.0f)
	{
		zCamera = 1.0f;
	}

	factor = PROJ_SCALE / zCamera;
	p.x = (int16_t)(SCREEN_CX + v.x * factor);
	p.y = (int16_t)(SCREEN_CY - v.y * factor);
	return p;
}

void Cube3D_Render(float pitchDeg, float rollDeg, float yawDeg)
{
	uint8_t i;
	Vec3f_t rotated[8];
	Vec2i_t projected[8];
	uint8_t idx0, idx1;
	int16_t sumX = 0;
	int16_t sumY = 0;
	int16_t offsetX;
	int16_t offsetY;

	/* 每帧仅清缓冲，最后统一刷新，避免"先黑屏再绘图"的闪烁 */
	OLED_ClearBuffer();

	for (i = 0; i < 8; i++)
	{
		rotated[i] = RotateXYZ(kCubeVertices[i], pitchDeg, rollDeg, yawDeg);
		projected[i] = ProjectToScreen(rotated[i]);
		sumX += projected[i].x;
		sumY += projected[i].y;
	}

	/* 将投影重心锁定在屏幕中心，防止整体漂移出画面 */
	offsetX = (int16_t)SCREEN_CX - (sumX / 8);
	offsetY = (int16_t)SCREEN_CY - (sumY / 8);
	for (i = 0; i < 8; i++)
	{
		projected[i].x += offsetX;
		projected[i].y += offsetY;
	}

	for (i = 0; i < 12; i++)
	{
		idx0 = kCubeEdges[i][0];
		idx1 = kCubeEdges[i][1];
		OLED_DrawLine(projected[idx0].x, projected[idx0].y,
					  projected[idx1].x, projected[idx1].y, 1);
	}

	OLED_Refresh();
}
