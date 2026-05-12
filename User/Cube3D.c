#include "Cube3D.h"
#include "Cube3D_Hyperbolic.h"
#include "Cube3D_4D.h"
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

/* ========== 截角四面体：12顶点、18边 ========== */
#define TT_VTX_COUNT  12
#define TT_EDGE_COUNT 18

/*
 * 坐标：(±3, ±1, ±1) 的所有偶置换，要求符号变化次数为偶数
 * 缩放使最大顶点距与正方体角点距一致：31.18 / √11 ≈ 9.4
 */
#define TT_HALF_SIZE  9.4f

static const Vec3f_t kTTVertices[TT_VTX_COUNT] =
{
	{ 3.0f*TT_HALF_SIZE,  1.0f*TT_HALF_SIZE,  1.0f*TT_HALF_SIZE},
	{ 1.0f*TT_HALF_SIZE,  3.0f*TT_HALF_SIZE,  1.0f*TT_HALF_SIZE},
	{ 1.0f*TT_HALF_SIZE,  1.0f*TT_HALF_SIZE,  3.0f*TT_HALF_SIZE},
	{ 3.0f*TT_HALF_SIZE, -1.0f*TT_HALF_SIZE, -1.0f*TT_HALF_SIZE},
	{ 1.0f*TT_HALF_SIZE, -3.0f*TT_HALF_SIZE, -1.0f*TT_HALF_SIZE},
	{ 1.0f*TT_HALF_SIZE, -1.0f*TT_HALF_SIZE, -3.0f*TT_HALF_SIZE},
	{-3.0f*TT_HALF_SIZE,  1.0f*TT_HALF_SIZE, -1.0f*TT_HALF_SIZE},
	{-1.0f*TT_HALF_SIZE,  3.0f*TT_HALF_SIZE, -1.0f*TT_HALF_SIZE},
	{-1.0f*TT_HALF_SIZE,  1.0f*TT_HALF_SIZE, -3.0f*TT_HALF_SIZE},
	{-3.0f*TT_HALF_SIZE, -1.0f*TT_HALF_SIZE,  1.0f*TT_HALF_SIZE},
	{-1.0f*TT_HALF_SIZE, -3.0f*TT_HALF_SIZE,  1.0f*TT_HALF_SIZE},
	{-1.0f*TT_HALF_SIZE, -1.0f*TT_HALF_SIZE,  3.0f*TT_HALF_SIZE}
};

/* 相邻顶点恰好共享一个坐标值（同位置、同符号），其余相差 2 */
static const uint8_t kTTEdges[TT_EDGE_COUNT][2] =
{
	{0,1}, {0,2}, {0,3},
	{1,2}, {1,7},
	{2,11},
	{3,4}, {3,5},
	{4,5}, {4,10},
	{5,8},
	{6,7}, {6,8}, {6,9},
	{7,10},
	{8,11},
	{9,10}, {9,11}
};

/* ========== 小星星十二面体：12顶点、30边 ========== */
#define SS_VTX_COUNT  12
#define SS_EDGE_COUNT 30

/* 与二十面体共用顶点坐标，但边连接不同（互补于二十面体边集） */
#define SS_HALF_SIZE  (HALF_SIZE * 0.91058f)

static const Vec3f_t kSSVertices[SS_VTX_COUNT] =
{
	/*  0- 3: (0, ±1, ±φ) */
	{ 0.0f,           SS_HALF_SIZE,      SS_HALF_SIZE*PHI},
	{ 0.0f,           SS_HALF_SIZE,     -SS_HALF_SIZE*PHI},
	{ 0.0f,          -SS_HALF_SIZE,      SS_HALF_SIZE*PHI},
	{ 0.0f,          -SS_HALF_SIZE,     -SS_HALF_SIZE*PHI},
	/*  4- 7: (±1, ±φ, 0) */
	{ SS_HALF_SIZE,   SS_HALF_SIZE*PHI,  0.0f              },
	{ SS_HALF_SIZE,  -SS_HALF_SIZE*PHI,  0.0f              },
	{-SS_HALF_SIZE,   SS_HALF_SIZE*PHI,  0.0f              },
	{-SS_HALF_SIZE,  -SS_HALF_SIZE*PHI,  0.0f              },
	/*  8-11: (±φ, 0, ±1) */
	{ SS_HALF_SIZE*PHI,  0.0f,            SS_HALF_SIZE     },
	{ SS_HALF_SIZE*PHI,  0.0f,           -SS_HALF_SIZE     },
	{-SS_HALF_SIZE*PHI,  0.0f,            SS_HALF_SIZE     },
	{-SS_HALF_SIZE*PHI,  0.0f,           -SS_HALF_SIZE     }
};

/*
 * 边连接 = K₁₂ 完全图中除去二十面体的30条边和6对对径点，
 * 每条边对应五角星面的一个"跳跃"连接（隔一个顶点）。
 */
static const uint8_t kSSEdges[SS_EDGE_COUNT][2] =
{
	{0,1}, {0,5}, {0,7}, {0,9}, {0,11},
	{1,5}, {1,7}, {1,8}, {1,10},
	{2,3}, {2,4}, {2,6}, {2,9}, {2,11},
	{3,4}, {3,6}, {3,8}, {3,10},
	{4,5}, {4,10}, {4,11},
	{5,10}, {5,11},
	{6,7}, {6,8}, {6,9},
	{7,8}, {7,9},
	{8,10},
	{9,11}
};

/* ========== 大星形十二面体：20顶点、30边 ========== */
#define GSE_VTX_COUNT  20
#define GSE_EDGE_COUNT 30

/*
 * 大星形十二面体 {5/2, 3}，开普勒-庞索多面体。
 * 与正十二面体共用20个顶点，但边连接不同：
 * 每个面为五角星形，由12个五角星面构成，
 * 每顶点连接3条边。计算方式：对十二面体的12个
 * 五边形面平面取最近的5个异面顶点，以五角星模式连接。
 */
static const uint8_t kGSEEdges[GSE_EDGE_COUNT][2] =
{
	{0,11}, {0,15}, {0,19},
	{1,10}, {1,14}, {1,19},
	{2,9},  {2,15}, {2,18},
	{3,8},  {3,14}, {3,18},
	{4,11}, {4,13}, {4,17},
	{5,10}, {5,12}, {5,17},
	{6,9},  {6,13}, {6,16},
	{7,8},  {7,12}, {7,16},
	{8,9},  {10,11},
	{12,14},{13,15},
	{16,17},{18,19}
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

/* ========== 4D 旋转与投影辅助函数 ========== */

static Vec4f_t Rotate4D_XW(Vec4f_t v, float deg)
{
	float c = cosf(deg * DEG_TO_RAD);
	float s = sinf(deg * DEG_TO_RAD);
	Vec4f_t r;
	r.x = v.x * c - v.w * s;
	r.y = v.y;
	r.z = v.z;
	r.w = v.x * s + v.w * c;
	return r;
}

static Vec4f_t Rotate4D_YZ(Vec4f_t v, float deg)
{
	float c = cosf(deg * DEG_TO_RAD);
	float s = sinf(deg * DEG_TO_RAD);
	Vec4f_t r;
	r.x = v.x;
	r.y = v.y * c - v.z * s;
	r.z = v.y * s + v.z * c;
	r.w = v.w;
	return r;
}

static Vec3f_t Project4DTo3D(Vec4f_t v)
{
	/* 4D→3D 透视投影：观察距离沿 W 轴为 3.0 单位 */
	const float d = 3.0f;
	float scale = d / (d + v.w);
	Vec3f_t r;
	r.x = v.x * scale;
	r.y = v.y * scale;
	r.z = v.z * scale;
	return r;
}

void Cube3D_Render(float pitchDeg, float rollDeg, float yawDeg, uint8_t shape, float dtSec)
{
	uint8_t i;
	uint8_t vtxCount, edgeCount;
	const Vec3f_t *vertices;
	const uint8_t (*edges)[2];
	Vec2i_t projected[HYPERBOLIC_MAX_VTX];
	const Vec4f_t *verts4D = 0;
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
	else if (shape == SHAPE_TRUNCATED_TETRA)
	{
		vertices = kTTVertices;
		edges    = kTTEdges;
		vtxCount = TT_VTX_COUNT;
		edgeCount = TT_EDGE_COUNT;
	}
	else if (shape == SHAPE_SMALL_STELLATED)
	{
		vertices = kSSVertices;
		edges    = kSSEdges;
		vtxCount = SS_VTX_COUNT;
		edgeCount = SS_EDGE_COUNT;
	}
	else if (shape == SHAPE_GREAT_STELLATED)
	{
		vertices = kDodVertices;
		edges    = kGSEEdges;
		vtxCount = GSE_VTX_COUNT;
		edgeCount = GSE_EDGE_COUNT;
	}
	else if (shape == SHAPE_H3_7)
	{
		vertices = kH3_7Vertices;
		edges    = kH3_7Edges;
		vtxCount = H3_7_VTX_COUNT;
		edgeCount = H3_7_EDGE_COUNT;
	}
	else if (shape == SHAPE_H3_8)
	{
		vertices = kH3_8Vertices;
		edges    = kH3_8Edges;
		vtxCount = H3_8_VTX_COUNT;
		edgeCount = H3_8_EDGE_COUNT;
	}
	else if (shape == SHAPE_H4_5)
	{
		vertices = kH4_5Vertices;
		edges    = kH4_5Edges;
		vtxCount = H4_5_VTX_COUNT;
		edgeCount = H4_5_EDGE_COUNT;
	}
	else if (shape == SHAPE_H5_4)
	{
		vertices = kH5_4Vertices;
		edges    = kH5_4Edges;
		vtxCount = H5_4_VTX_COUNT;
		edgeCount = H5_4_EDGE_COUNT;
	}
	else if (shape == SHAPE_H4_6)
	{
		vertices = kH4_6Vertices;
		edges    = kH4_6Edges;
		vtxCount = H4_6_VTX_COUNT;
		edgeCount = H4_6_EDGE_COUNT;
	}
	else if (shape == SHAPE_H6_4)
	{
		vertices = kH6_4Vertices;
		edges    = kH6_4Edges;
		vtxCount = H6_4_VTX_COUNT;
		edgeCount = H6_4_EDGE_COUNT;
	}
	else if (shape == SHAPE_H5_5)
	{
		vertices = kH5_5Vertices;
		edges    = kH5_5Edges;
		vtxCount = H5_5_VTX_COUNT;
		edgeCount = H5_5_EDGE_COUNT;
	}
	else if (shape == SHAPE_H5_6)
	{
		vertices = kH5_6Vertices;
		edges    = kH5_6Edges;
		vtxCount = H5_6_VTX_COUNT;
		edgeCount = H5_6_EDGE_COUNT;
	}
	else if (shape == SHAPE_H6_5)
	{
		vertices = kH6_5Vertices;
		edges    = kH6_5Edges;
		vtxCount = H6_5_VTX_COUNT;
		edgeCount = H6_5_EDGE_COUNT;
	}
	else if (shape == SHAPE_H7_3)
	{
		vertices = kH7_3Vertices;
		edges    = kH7_3Edges;
		vtxCount = H7_3_VTX_COUNT;
		edgeCount = H7_3_EDGE_COUNT;
	}
	else if (shape == SHAPE_H8_3)
	{
		vertices = kH8_3Vertices;
		edges    = kH8_3Edges;
		vtxCount = H8_3_VTX_COUNT;
		edgeCount = H8_3_EDGE_COUNT;
	}
	else if (shape == SHAPE_H3_10)
	{
		vertices = kH3_10Vertices;
		edges    = kH3_10Edges;
		vtxCount = H3_10_VTX_COUNT;
		edgeCount = H3_10_EDGE_COUNT;
	}
	else if (shape == SHAPE_H10_3)
	{
		vertices = kH10_3Vertices;
		edges    = kH10_3Edges;
		vtxCount = H10_3_VTX_COUNT;
		edgeCount = H10_3_EDGE_COUNT;
	}
	else if (shape == SHAPE_5_CELL)
	{
		verts4D  = kP5_CELLVertices;
		edges    = kP5_CELLEdges;
		vtxCount = P5_CELL_VTX_COUNT;
		edgeCount = P5_CELL_EDGE_COUNT;
	}
	else if (shape == SHAPE_TESSERACT)
	{
		verts4D  = kTESSERACTVertices;
		edges    = kTESSERACTEdges;
		vtxCount = TESSERACT_VTX_COUNT;
		edgeCount = TESSERACT_EDGE_COUNT;
	}
	else if (shape == SHAPE_16_CELL)
	{
		verts4D  = kP16_CELLVertices;
		edges    = kP16_CELLEdges;
		vtxCount = P16_CELL_VTX_COUNT;
		edgeCount = P16_CELL_EDGE_COUNT;
	}
	else if (shape == SHAPE_24_CELL)
	{
		verts4D  = kP24_CELLVertices;
		edges    = kP24_CELLEdges;
		vtxCount = P24_CELL_VTX_COUNT;
		edgeCount = P24_CELL_EDGE_COUNT;
	}
	else if (shape == SHAPE_600_CELL)
	{
		verts4D  = kP600_CELLVertices;
		edges    = kP600_CELLEdges;
		vtxCount = P600_CELL_VTX_COUNT;
		edgeCount = P600_CELL_EDGE_COUNT;
	}
	else if (shape == SHAPE_P600_STAR1)
	{
		verts4D  = kP600_STAR1Vertices;
		edges    = kP600_STAR1Edges;
		vtxCount = P600_STAR1_VTX_COUNT;
		edgeCount = P600_STAR1_EDGE_COUNT;
	}
	else if (shape == SHAPE_P600_STAR2)
	{
		verts4D  = kP600_STAR2Vertices;
		edges    = kP600_STAR2Edges;
		vtxCount = P600_STAR2_VTX_COUNT;
		edgeCount = P600_STAR2_EDGE_COUNT;
	}
	else if (shape == SHAPE_P600_STAR3)
	{
		verts4D  = kP600_STAR3Vertices;
		edges    = kP600_STAR3Edges;
		vtxCount = P600_STAR3_VTX_COUNT;
		edgeCount = P600_STAR3_EDGE_COUNT;
	}
	else if (shape == SHAPE_P600_STAR4)
	{
		verts4D  = kP600_STAR4Vertices;
		edges    = kP600_STAR4Edges;
		vtxCount = P600_STAR4_VTX_COUNT;
		edgeCount = P600_STAR4_EDGE_COUNT;
	}
	else
	{
		vertices = kCubeVertices;
		edges    = kCubeEdges;
		vtxCount = CUBE_VTX_COUNT;
		edgeCount = CUBE_EDGE_COUNT;
	}
	OLED_ClearBuffer();

	/* 4D auto-rotation angles accumulate from real frame time */
	static float cellAngleXW = 0.0f;
	static float cellAngleYZ = 0.0f;
	if (verts4D)
	{
		cellAngleXW += dtSec * 15.0f;
		cellAngleYZ += dtSec * 10.0f;
		/* Pass 1: compute bounding sphere radius after 4D projection */
		float maxRadSq = 0.0f;
		for (i = 0; i < vtxCount; i++)
		{
			Vec4f_t r4 = Rotate4D_XW(verts4D[i], cellAngleXW);
			r4 = Rotate4D_YZ(r4, cellAngleYZ);
			Vec3f_t v3 = Project4DTo3D(r4);
			float r2 = v3.x*v3.x + v3.y*v3.y + v3.z*v3.z;
			if (r2 > maxRadSq) maxRadSq = r2;
		}
		float autoScale = (maxRadSq > 0.001f) ? (22.0f / sqrtf(maxRadSq)) : 22.0f;
		/* Pass 2: render with auto-scale to fit screen */
		for (i = 0; i < vtxCount; i++)
		{
			Vec4f_t r4 = Rotate4D_XW(verts4D[i], cellAngleXW);
			r4 = Rotate4D_YZ(r4, cellAngleYZ);
			Vec3f_t v3 = Project4DTo3D(r4);
			v3.x *= autoScale;
			v3.y *= autoScale;
			v3.z *= autoScale;
			v3 = RotateXYZ(v3, pitchDeg, rollDeg, yawDeg);
			projected[i] = ProjectToScreen(v3);
			sumX += projected[i].x;
			sumY += projected[i].y;
		}
	}
	else
	{
		for (i = 0; i < vtxCount; i++)
		{
			projected[i] = ProjectToScreen(RotateXYZ(vertices[i], pitchDeg, rollDeg, yawDeg));
			sumX += projected[i].x;
			sumY += projected[i].y;
		}
	}

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
