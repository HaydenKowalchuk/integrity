#include <integrity/common/2d_physics/aabb_2d.h>

AABB_2D AABB_2D_Create(vec2 min, vec2 max)
{
    AABB_2D temp;
    glm_vec2_copy(min, temp.min);
    glm_vec2_copy(max, temp.max);

    return temp;
}

void AABB_2D_Translate(c2AABB *aabb, vec2 by)
{
    *aabb = (c2AABB){.min = {.x = aabb->min.x + by[0], .y = aabb->min.y + by[1]}, .max = {.x = aabb->max.x + by[0], .y = aabb->max.y + by[1]}};
    // glm_vec2_add(aabb->min, by, aabb->min);
    // glm_vec2_add(aabb->max, by, aabb->max);
    //return aabb;
}

bool AABB_2D_Intersects(AABB_2D *first, AABB_2D *second)
{
    /* Slow? */
    if (second->min[0] >= first->max[0])
        return false;
    if (second->min[1] >= first->max[1])
        return false;
    if (second->max[0] <= first->min[0])
        return false;
    if (second->max[1] <= first->min[1])
        return false;
    return true;
#if 0
    bool x = false, y = false;
    //bool x = abs(first->vec[0] - second->vec[0]) <= (first->vec[0] + second->vec[0]);
    //bool y = abs(first->vec[1] - second->vec[1]) <= (first->vec[1] + second->vec[1]);
    return (x && y);
#endif
}
