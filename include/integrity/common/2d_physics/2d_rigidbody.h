#pragma once

#include <integrity/common/common.h>
#include <cglm/cglm.h>
#include "aabb_2d.h"
#include <cute_c2.h>

typedef struct rigidbody_2d
{
  c2AABB shape;
  vec2 tx;
  float mass;
  float inv_mass;
  float friction;
  float restitution;
  float gravityMultiplier;
  vec2 velocity;
  vec2 _force;
  vec2 _impulses;
  vec2 forces;
  float timestamp;
  int _sleepFrameCount;
  bool sleeping;
  bool active;
  char _uuid;
  char layer;
  void (*onCollide)(void*, void*);
} rigidbody_2d;

rigidbody_2d RBDY_2D_Create(AABB_2D *aabb, float mass, float friction, float restitution, float gravMult /*, void *onCollide, bool autoStep*/);
void RBDY_2D_SetPosition(rigidbody_2d *body, vec2 pos);
void RBDY_2D_GetPosition(rigidbody_2d *body, vec2 out);
void RBDY_2D_GetPosition_OGL(rigidbody_2d *body, vec3 out);
float RBDY_2D_GetHeight(rigidbody_2d *body);
float RBDY_2D_GetWidth(rigidbody_2d *body);

void RBDY_2D_ApplyForce(rigidbody_2d *body, vec2 force);
void RBDY_2D_ApplyImpulse(rigidbody_2d *body, vec2 impulse);

static inline void RBDY_2D_MarkActive(rigidbody_2d *body) { body->_sleepFrameCount = 10 | 0; }
