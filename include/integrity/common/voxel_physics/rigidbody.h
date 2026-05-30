#pragma once

#include <integrity/common/common.h>
#include <cglm/cglm.h>
#include "aabb_3d.h"

typedef struct rigidbody
{
    AABB aabb;
    float mass;
    float inv_mass;
    float friction;
    float restitution;
    float gravityMultiplier;
    void (*onCollide)(struct rigidbody *, float *);
    bool autoStep;
    float airDrag;   // overrides global airDrag when >= 0
    float fluidDrag; // overrides global fluidDrag when >= 0
    void (*onStep)(void);
    vec3 velocity;
    vec3 resting;
    bool inFluid;
    float _ratioInFluid;
    vec3 _forces;
    vec3 _impulses;
    int _sleepFrameCount;
    float timestamp;
} rigidbody;

rigidbody RBDY_Create(AABB *aabb, float mass, float friction, float restitution, float gravMult,  void (*onCollide)(rigidbody*, vec3), bool autoStep);
void RBDY_SetPosition(rigidbody *body, vec3 pos);
void RBDY_GetPosition(rigidbody *body, vec3 out);
void RBDY_ApplyForce(rigidbody *body, vec3 force);
void RBDY_ApplyImpulse(rigidbody *body, vec3 impulse);

static inline void RBDY_MarkActive(rigidbody *body) { body->_sleepFrameCount = 10 | 0; }
static inline float RBDY_atRestX(rigidbody *body) { return body->resting[0]; }
static inline float RBDY_atRestY(rigidbody *body) { return body->resting[1]; }
static inline float RBDY_atRestZ(rigidbody *body) { return body->resting[2]; }
