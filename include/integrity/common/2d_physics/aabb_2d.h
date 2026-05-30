#pragma once

#include <cglm/cglm.h>
#include <cute_c2.h>
#include <integrity/common/common.h>

typedef struct AABB_2D {
  vec2 min;
  vec2 max;
} AABB_2D;

AABB_2D AABB_2D_Create(vec2 min, vec2 max);
void AABB_2D_Translate(c2AABB* aabb, vec2 by);
bool AABB_2D_Intersects(AABB_2D* first, AABB_2D* second);
