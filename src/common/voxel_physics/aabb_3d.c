#include <integrity/common/voxel_physics/aabb_3d.h>

AABB AABB_Create(vec3 pos, vec3 vec)
{
    AABB temp;
    vec3 pos2;
    vec[0] -=.00001f;
    vec[1] -=.00001f;
    vec[2] -=.00001f;
    pos[0] +=.00001f;
    pos[1] +=.00001f;
    pos[2] +=.00001f;
    glm_vec3_add(pos, vec, pos2);

    glm_vec3_minv(pos, pos2, temp.base);
    glm_vec3_copy(vec, temp.vec);
    glm_vec3_maxv(pos, pos2, temp.max);

   //temp.length = glm_vec3_norm(temp.vec);
    return temp;
}

AABB *AABB_Translate(AABB *aabb, vec3 by)
{
    glm_vec3_add(aabb->max, by, aabb->max);
    glm_vec3_add(aabb->base, by, aabb->base);
    return aabb;
}

bool AABB_Intersects(AABB *first, AABB *second)
{
    #if 0
    /* Slow? */
    if (second->base[0] >= first->max[0])
        return false;
    if (second->base[1] >= first->max[1])
        return false;
    if (second->base[2] >= first->max[2])
        return false;
    if (second->max[0] <= first->base[0])
        return false;
    if (second->max[1] <= first->base[1])
        return false;
    if (second->max[2] <= first->base[2])
        return false;
    return true;
    #endif
    bool x = fabsf(first->vec[0] - second->vec[0]) <= (first->vec[0] + second->vec[0]);
    bool y = fabsf(first->vec[1] - second->vec[1]) <= (first->vec[1] + second->vec[1]);
    bool z = fabsf(first->vec[2] - second->vec[2]) <= (first->vec[2] + second->vec[2]);
    return (x && y && z);
}
