#pragma once

#include <integrity/common/common.h>
#include <cglm/cglm.h>

typedef struct AABB
{
    vec3 base;
    vec3 vec;
    vec3 max;
    //float length;
} AABB;

AABB AABB_Create(vec3 pos, vec3 vec);
AABB *AABB_Translate(AABB *aabb, vec3 by);
bool AABB_Intersects(AABB *first, AABB *second);
