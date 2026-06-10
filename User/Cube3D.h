#ifndef __CUBE3D_H
#define __CUBE3D_H

#include "stm32f10x.h"

typedef struct
{
	float x;
	float y;
	float z;
} Vec3f_t;

typedef struct
{
	float x;
	float y;
	float z;
	float w;
} Vec4f_t;

#define SHAPE_CUBE           0
#define SHAPE_OCTAHEDRON     1
#define SHAPE_TETRAHEDRON    2
#define SHAPE_DODECAHEDRON   3
#define SHAPE_ICOSAHEDRON    4
#define SHAPE_CUBOCTAHEDRON  5
#define SHAPE_TRUNCATED_TETRA  6
#define SHAPE_SMALL_STELLATED  7
#define SHAPE_GREAT_STELLATED  8
#define SHAPE_H3_7           9
#define SHAPE_H3_8          10
#define SHAPE_H4_5          11
#define SHAPE_H5_4          12
#define SHAPE_H4_6          13
#define SHAPE_H6_4          14
#define SHAPE_H5_5          15
#define SHAPE_H5_6          16
#define SHAPE_H6_5          17
#define SHAPE_H7_3          18
#define SHAPE_H8_3          19
#define SHAPE_H3_10         20
#define SHAPE_H10_3         21
#define SHAPE_5_CELL        22
#define SHAPE_TESSERACT     23
#define SHAPE_16_CELL       24
#define SHAPE_24_CELL       25
#define SHAPE_600_CELL      26
#define SHAPE_P600_STAR1    27
#define SHAPE_P600_STAR2    28
#define SHAPE_P600_STAR3    29
#define SHAPE_P600_STAR4    30

#define TOTAL_SHAPE_COUNT   31
#define HYPERBOLIC_MAX_VTX  150

void Cube3D_Render(float pitchDeg, float rollDeg, float yawDeg, uint8_t shape, float dtSec);

#endif
