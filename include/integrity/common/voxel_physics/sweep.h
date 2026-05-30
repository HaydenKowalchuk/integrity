#pragma once

#include <integrity/common/common.h>
#include <cglm/cglm.h>
#include "voxel_physics.h"

float sweep(blockTest getVoxel, void* _chunk, AABB *box, vec3 dir, CollisionCallback callback, bool noTranslate);
