#include "Cube3D.h"
#include "OLED.h"
#include <math.h>

#define DEG_TO_RAD      0.0174532925f

/* OLED显示中心与透视参数 */
#define SCREEN_CX       64.0f
#define SCREEN_CY       32.0f
#define CAMERA_Z        100.0f
#define PROJ_SCALE      108.0f

/* 线框半边长，单位可视作任意缩放单位 */
#define HALF_SIZE      18.0f
/* 八面体放大到与正方体相同的视觉大小（正方体角点距 = 18*√3 ≈ 31.18） */
#define OCT_HALF_SIZE  31.18f

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

/* ========== 立方体：8顶点、12边 ========== */
#define CUBE_VTX_COUNT  8
#define CUBE_EDGE_COUNT 12

static const Vec3f_t kCubeVertices[CUBE_VTX_COUNT] =
{
	{-HALF_SIZE, -HALF_SIZE, -HALF_SIZE},
	{ HALF_SIZE, -HALF_SIZE, -HALF_SIZE},
	{ HALF_SIZE,  HALF_SIZE, -HALF_SIZE},
	{-HALF_SIZE,  HALF_SIZE, -HALF_SIZE},
	{-HALF_SIZE, -HALF_SIZE,  HALF_SIZE},
	{ HALF_SIZE, -HALF_SIZE,  HALF_SIZE},
	{ HALF_SIZE,  HALF_SIZE,  HALF_SIZE},
	{-HALF_SIZE,  HALF_SIZE,  HALF_SIZE}
};

static const uint8_t kCubeEdges[CUBE_EDGE_COUNT][2] =
{
	{0, 1}, {1, 2}, {2, 3}, {3, 0},
	{4, 5}, {5, 6}, {6, 7}, {7, 4},
	{0, 4}, {1, 5}, {2, 6}, {3, 7}
};

/* ========== 八面体：6顶点、12边 ========== */
#define OCT_VTX_COUNT  6
#define OCT_EDGE_COUNT 12

static const Vec3f_t kOctVertices[OCT_VTX_COUNT] =
{
	{ 0.0f,           0.0f,            OCT_HALF_SIZE},   /* 0: 上顶点 */
	{ 0.0f,           0.0f,           -OCT_HALF_SIZE},   /* 1: 下顶点 */
	{ OCT_HALF_SIZE,  0.0f,           0.0f          },   /* 2: +X */
	{-OCT_HALF_SIZE,  0.0f,           0.0f          },   /* 3: -X */
	{ 0.0f,           OCT_HALF_SIZE,  0.0f          },   /* 4: +Y */
	{ 0.0f,          -OCT_HALF_SIZE,  0.0f          }    /* 5: -Y */
};

static const uint8_t kOctEdges[OCT_EDGE_COUNT][2] =
{
	{0, 2}, {0, 3}, {0, 4}, {0, 5},   /* 上顶点 → 赤道 */
	{1, 2}, {1, 3}, {1, 4}, {1, 5},   /* 下顶点 → 赤道 */
	{2, 4}, {4, 3}, {3, 5}, {5, 2}    /* 赤道四边形 */
};

/* ========== 正四面体：4顶点、6边 ========== */
#define TET_VTX_COUNT  4
#define TET_EDGE_COUNT 6

/* 取正方体的4个交替角点，构成内接正四面体，与正方体等大 */
static const Vec3f_t kTetVertices[TET_VTX_COUNT] =
{
	{ HALF_SIZE,  HALF_SIZE,  HALF_SIZE},
	{ HALF_SIZE, -HALF_SIZE, -HALF_SIZE},
	{-HALF_SIZE,  HALF_SIZE, -HALF_SIZE},
	{-HALF_SIZE, -HALF_SIZE,  HALF_SIZE}
};

static const uint8_t kTetEdges[TET_EDGE_COUNT][2] =
{
	{0, 1}, {0, 2}, {0, 3},
	{1, 2}, {1, 3}, {2, 3}
};

/* ========== 正十二面体：20顶点、30边 ========== */
#define DOD_VTX_COUNT  20
#define DOD_EDGE_COUNT 30

/* 黄金比例 φ = (1+√5)/2，用于十二面体坐标 */
#define PHI  1.61803398875f

static const Vec3f_t kDodVertices[DOD_VTX_COUNT] =
{
	/* 0-7: 正方体角点 (±1, ±1, ±1) */
	{ HALF_SIZE,  HALF_SIZE,  HALF_SIZE},
	{ HALF_SIZE,  HALF_SIZE, -HALF_SIZE},
	{ HALF_SIZE, -HALF_SIZE,  HALF_SIZE},
	{ HALF_SIZE, -HALF_SIZE, -HALF_SIZE},
	{-HALF_SIZE,  HALF_SIZE,  HALF_SIZE},
	{-HALF_SIZE,  HALF_SIZE, -HALF_SIZE},
	{-HALF_SIZE, -HALF_SIZE,  HALF_SIZE},
	{-HALF_SIZE, -HALF_SIZE, -HALF_SIZE},
	/* 8-19: 循环置换 (0, ±1/φ, ±φ) × HALF_SIZE */
	{ 0.0f,              HALF_SIZE/PHI,  HALF_SIZE*PHI},
	{ 0.0f,              HALF_SIZE/PHI, -HALF_SIZE*PHI},
	{ 0.0f,             -HALF_SIZE/PHI,  HALF_SIZE*PHI},
	{ 0.0f,             -HALF_SIZE/PHI, -HALF_SIZE*PHI},
	{ HALF_SIZE*PHI,     0.0f,           HALF_SIZE/PHI},
	{ HALF_SIZE*PHI,     0.0f,          -HALF_SIZE/PHI},
	{-HALF_SIZE*PHI,     0.0f,           HALF_SIZE/PHI},
	{-HALF_SIZE*PHI,     0.0f,          -HALF_SIZE/PHI},
	{ HALF_SIZE/PHI,     HALF_SIZE*PHI,  0.0f          },
	{ HALF_SIZE/PHI,    -HALF_SIZE*PHI,  0.0f          },
	{-HALF_SIZE/PHI,     HALF_SIZE*PHI,  0.0f          },
	{-HALF_SIZE/PHI,    -HALF_SIZE*PHI,  0.0f          }
};

/* 正十二面体30条边：每个立方体角点连接3个非立方体顶点，非立方体顶点配对成6条边 */
static const uint8_t kDodEdges[DOD_EDGE_COUNT][2] =
{
	/* 立方体角点 → 非立方体顶点 (8×3 = 24条) */
	{0,8}, {0,12}, {0,16},
	{1,9}, {1,13}, {1,16},
	{2,10}, {2,12}, {2,17},
	{3,11}, {3,13}, {3,17},
	{4,8}, {4,14}, {4,18},
	{5,9}, {5,15}, {5,18},
	{6,10}, {6,14}, {6,19},
	{7,11}, {7,15}, {7,19},
	/* 非立方体顶点间配对 (6条) */
	{8,10}, {9,11}, {12,13},
	{14,15}, {16,18}, {17,19}
};

/* ========== 正二十面体：12顶点、30边 ========== */
#define ICO_VTX_COUNT  12
#define ICO_EDGE_COUNT 30

/* 二十面体缩放，使顶点距与正方体角点距相同（18*√3 / √(1+φ²)） */
#define ICO_HALF_SIZE  (HALF_SIZE * 0.91058f)

static const Vec3f_t kIcoVertices[ICO_VTX_COUNT] =
{
	/*  0- 3: (0, ±1, ±φ) */
	{ 0.0f,           HALF_SIZE,      HALF_SIZE*PHI},
	{ 0.0f,           HALF_SIZE,     -HALF_SIZE*PHI},
	{ 0.0f,          -HALF_SIZE,      HALF_SIZE*PHI},
	{ 0.0f,          -HALF_SIZE,     -HALF_SIZE*PHI},
	/*  4- 7: (±1, ±φ, 0) */
	{ HALF_SIZE,      HALF_SIZE*PHI,  0.0f          },
	{ HALF_SIZE,     -HALF_SIZE*PHI,  0.0f          },
	{-HALF_SIZE,      HALF_SIZE*PHI,  0.0f          },
	{-HALF_SIZE,     -HALF_SIZE*PHI,  0.0f          },
	/*  8-11: (±φ, 0, ±1) */
	{ HALF_SIZE*PHI,  0.0f,           HALF_SIZE     },
	{ HALF_SIZE*PHI,  0.0f,          -HALF_SIZE     },
	{-HALF_SIZE*PHI,  0.0f,           HALF_SIZE     },
	{-HALF_SIZE*PHI,  0.0f,          -HALF_SIZE     }
};

static const uint8_t kIcoEdges[ICO_EDGE_COUNT][2] =
{
	{0,2}, {0,4}, {0,6}, {0,8}, {0,10},
	{1,3}, {1,4}, {1,6}, {1,9}, {1,11},
	{2,5}, {2,7}, {2,8}, {2,10},
	{3,5}, {3,7}, {3,9}, {3,11},
	{4,6}, {4,8}, {4,9},
	{5,7}, {5,8}, {5,9},
	{6,10}, {6,11},
	{7,10}, {7,11},
	{8,9},
	{10,11}
};

/* ========== 立方八面体：12顶点、24边 ========== */
#define CO_VTX_COUNT  12
#define CO_EDGE_COUNT 24

/* 缩放使顶点距与正方体角点相同（18*√3 / √2） */
#define CO_HALF_SIZE  22.05f

static const Vec3f_t kCoVertices[CO_VTX_COUNT] =
{
	/* (±1, ±1, 0) */
	{ CO_HALF_SIZE,  CO_HALF_SIZE,  0.0f          },
	{ CO_HALF_SIZE, -CO_HALF_SIZE,  0.0f          },
	{-CO_HALF_SIZE,  CO_HALF_SIZE,  0.0f          },
	{-CO_HALF_SIZE, -CO_HALF_SIZE,  0.0f          },
	/* (±1, 0, ±1) */
	{ CO_HALF_SIZE,  0.0f,           CO_HALF_SIZE },
	{ CO_HALF_SIZE,  0.0f,          -CO_HALF_SIZE },
	{-CO_HALF_SIZE,  0.0f,           CO_HALF_SIZE },
	{-CO_HALF_SIZE,  0.0f,          -CO_HALF_SIZE },
	/* (0, ±1, ±1) */
	{ 0.0f,           CO_HALF_SIZE,  CO_HALF_SIZE },
	{ 0.0f,           CO_HALF_SIZE, -CO_HALF_SIZE },
	{ 0.0f,          -CO_HALF_SIZE,  CO_HALF_SIZE },
	{ 0.0f,          -CO_HALF_SIZE, -CO_HALF_SIZE }
};

static const uint8_t kCoEdges[CO_EDGE_COUNT][2] =
{
	{0,4}, {0,5}, {0,8}, {0,9},
	{1,4}, {1,5}, {1,10}, {1,11},
	{2,6}, {2,7}, {2,8}, {2,9},
	{3,6}, {3,7}, {3,10}, {3,11},
	{4,8}, {4,10},
	{5,9}, {5,11},
	{6,8}, {6,10},
	{7,9}, {7,11}
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

void Cube3D_Render(float pitchDeg, float rollDeg, float yawDeg, uint8_t shape)
{
	uint8_t i;
	uint8_t vtxCount, edgeCount;
	const Vec3f_t *vertices;
	const uint8_t (*edges)[2];
	Vec3f_t rotated[20];    /* 最大顶点数（十二面体20个） */
	Vec2i_t projected[20];
	uint8_t idx0, idx1;
	int16_t sumX = 0;
	int16_t sumY = 0;
	int16_t offsetX;
	int16_t offsetY;

	if (shape == SHAPE_OCTAHEDRON)
	{
		vertices = kOctVertices;
		edges    = kOctEdges;
		vtxCount = OCT_VTX_COUNT;
		edgeCount = OCT_EDGE_COUNT;
	}
	else if (shape == SHAPE_TETRAHEDRON)
	{
		vertices = kTetVertices;
		edges    = kTetEdges;
		vtxCount = TET_VTX_COUNT;
		edgeCount = TET_EDGE_COUNT;
	}
	else if (shape == SHAPE_DODECAHEDRON)
	{
		vertices = kDodVertices;
		edges    = kDodEdges;
		vtxCount = DOD_VTX_COUNT;
		edgeCount = DOD_EDGE_COUNT;
	}
	else if (shape == SHAPE_ICOSAHEDRON)
	{
		vertices = kIcoVertices;
		edges    = kIcoEdges;
		vtxCount = ICO_VTX_COUNT;
		edgeCount = ICO_EDGE_COUNT;
	}
	else if (shape == SHAPE_CUBOCTAHEDRON)
	{
		vertices = kCoVertices;
		edges    = kCoEdges;
		vtxCount = CO_VTX_COUNT;
		edgeCount = CO_EDGE_COUNT;
	}
	else
	{
		vertices = kCubeVertices;
		edges    = kCubeEdges;
		vtxCount = CUBE_VTX_COUNT;
		edgeCount = CUBE_EDGE_COUNT;
	}

	/* 每帧仅清缓冲，最后统一刷新，避免"先黑屏再绘图"的闪烁 */
	OLED_ClearBuffer();

	for (i = 0; i < vtxCount; i++)
	{
		rotated[i] = RotateXYZ(vertices[i], pitchDeg, rollDeg, yawDeg);
		projected[i] = ProjectToScreen(rotated[i]);
		sumX += projected[i].x;
		sumY += projected[i].y;
	}

	/* 将投影重心锁定在屏幕中心，防止整体漂移出画面 */
	offsetX = (int16_t)SCREEN_CX - (sumX / (int16_t)vtxCount);
	offsetY = (int16_t)SCREEN_CY - (sumY / (int16_t)vtxCount);
	for (i = 0; i < vtxCount; i++)
	{
		projected[i].x += offsetX;
		projected[i].y += offsetY;
	}

	for (i = 0; i < edgeCount; i++)
	{
		idx0 = edges[i][0];
		idx1 = edges[i][1];
		OLED_DrawLine(projected[idx0].x, projected[idx0].y,
					  projected[idx1].x, projected[idx1].y, 1);
	}

	OLED_Refresh();
}
