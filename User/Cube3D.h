#ifndef __CUBE3D_H
#define __CUBE3D_H

#include "stm32f10x.h"

#define SHAPE_CUBE      0
#define SHAPE_OCTAHEDRON 1
#define SHAPE_TETRAHEDRON 2
#define SHAPE_DODECAHEDRON 3
#define SHAPE_ICOSAHEDRON 4
#define SHAPE_CUBOCTAHEDRON 5

void Cube3D_Render(float pitchDeg, float rollDeg, float yawDeg, uint8_t shape);

#endif
