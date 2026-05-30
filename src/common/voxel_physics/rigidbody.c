#include <integrity/common/voxel_physics/rigidbody.h>

rigidbody RBDY_Create(AABB *aabb, float mass, float friction, float restitution, float gravMult, void (*onCollide)(rigidbody*, vec3), bool autoStep)
{
    rigidbody temp =
        {
            .aabb = AABB_Create(aabb->base, aabb->vec),
            .mass = mass,
            .inv_mass = (!mass) ? 0 : 1 / mass,
            .friction = friction,
            .restitution = restitution,
            .gravityMultiplier = gravMult,
            .onCollide = onCollide,
            .autoStep = !!autoStep,
            .airDrag = -1,   // overrides global airDrag when >= 0
            .fluidDrag = -1, // overrides global fluidDrag when >= 0
            .onStep = NULL,
            .inFluid = false,
            ._ratioInFluid = 0,
            ._sleepFrameCount = 10 | 0,
            .timestamp = Sys_FloatTime(),
        };

    //internals
    glm_vec3_copy(GLM_VEC3_ZERO, temp.velocity);
    glm_vec3_copy(GLM_VEC3_ZERO, temp.resting);
    glm_vec3_copy(GLM_VEC3_ZERO, temp._forces);
    glm_vec3_copy(GLM_VEC3_ZERO, temp._impulses);

    return temp;
}

void RBDY_SetPosition(rigidbody *body, vec3 pos)
{
    glm_vec3_sub(pos, body->aabb.base, pos);
    AABB_Translate(&body->aabb, pos);
    glm_vec3_copy(GLM_VEC3_ZERO, body->_forces);
    glm_vec3_copy(GLM_VEC3_ZERO, body->_impulses);
    glm_vec3_copy(GLM_VEC3_ZERO, body->velocity);
    RBDY_MarkActive(body);
}
void RBDY_GetPosition(rigidbody *body, vec3 out)
{
    glm_vec3_copy(body->aabb.base, out);
}
void RBDY_ApplyForce(rigidbody *body, vec3 force)
{
    glm_vec3_add(body->_forces, force, body->_forces);
    RBDY_MarkActive(body);
}
void RBDY_ApplyImpulse(rigidbody *body, vec3 impulse)
{
    glm_vec3_add(body->_impulses, impulse, body->_impulses);
    RBDY_MarkActive(body);
}
