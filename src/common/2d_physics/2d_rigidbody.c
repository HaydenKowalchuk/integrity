#include <integrity/common/2d_physics/2d_physics.h>

static int uuid_counter = 0;

rigidbody_2d RBDY_2D_Create(AABB_2D *aabb, float mass, float friction, float restitution, float gravMult /*, void *onCollide, bool autoStep*/)
{
    rigidbody_2d temp = {
        ._uuid = (char)++uuid_counter,
        .shape = (c2AABB){.min = {.x = aabb->min[0], .y = aabb->min[1]}, .max = {.x = aabb->max[0], .y = aabb->max[1]}},
        .mass = mass,
        .inv_mass = (!mass) ? 0 : 1 / mass,
        .friction = friction,
        .restitution = restitution,
        .gravityMultiplier = gravMult,
        .timestamp = Sys_FloatTime(),
        ._sleepFrameCount = 3,
        .sleeping = false,
        .active = true,
        .layer = 1,
        .onCollide = NULL,
    };

    glm_vec2_zero(temp.forces);
    glm_vec2_zero(temp._force);
    glm_vec2_zero(temp._impulses);
    glm_vec2_zero(temp.velocity);

    return temp;
}

void RBDY_2D_SetPosition(rigidbody_2d *body, vec2 pos)
{
    float halfwidth = (body->shape.max.x - body->shape.min.x) / 2;
    float halfheight = (body->shape.max.y - body->shape.min.y) / 2;

    body->shape = (c2AABB){.min = {.x = pos[0] - halfwidth, .y = pos[1] - halfheight}, .max = {.x = pos[0] + halfwidth, .y = pos[1] + halfheight}};
    glm_vec2_zero(body->forces);
    glm_vec2_zero(body->_force);

    RBDY_2D_MarkActive(body);
}

void RBDY_2D_GetPosition(rigidbody_2d *body, vec2 out)
{
    out[0] = (body->shape.min.x + body->shape.max.x) / 2;
    out[1] = (body->shape.min.y + body->shape.max.y) / 2;
}

void RBDY_2D_GetPosition_OGL(rigidbody_2d *body, vec3 out)
{
    vec2 temp;
    RBDY_2D_GetPosition(body, temp);

    out[0] = (body->shape.min.x) / 25;
    out[1] = (480 - body->shape.min.y) / 25;
    out[2] = 0.0f;
}

float RBDY_2D_GetHeight(rigidbody_2d *body)
{
    return (body->shape.max.y - body->shape.min.y);
}

float RBDY_2D_GetWidth(rigidbody_2d *body)
{
    return (body->shape.max.x - body->shape.min.x);
}

void RBDY_2D_ApplyForce(rigidbody_2d *body, vec2 force)
{
    glm_vec2_add(body->forces, force, body->forces);
    RBDY_2D_MarkActive(body);
}

void RBDY_2D_ApplyImpulse(rigidbody_2d *body, vec2 impulse)
{
    glm_vec2_add(body->_impulses, impulse, body->_impulses);
    RBDY_2D_MarkActive(body);
}
