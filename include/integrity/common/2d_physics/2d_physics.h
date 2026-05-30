#pragma once

#include <cute_c2.h>
#include "2d_rigidbody.h"
#include "aabb_2d.h"

typedef struct PhysicsOptions_2D
{
  vec3 gravity;
  float minBounceImpulse; // lowest collision impulse that bounces
  float airDrag;
  float fluidDrag;
  float fluidDensity;
} PhysicsOptions_2D;

typedef struct PhysicsSystem_2D
{
  vec3 gravity;
  float airDrag;
  float fluidDensity;
  float fluidDrag;
  float minBounceImpulse;
  rigidbody_2d *bodies;
} PhysicsSystem_2D;

typedef struct Pair
{
  rigidbody_2d *A;
  rigidbody_2d *B;
} Pair;

void PHYS_2D_CreateEx(PhysicsOptions_2D opts);
void PHYS_2D_Create(void);
void PHYS_2D_Destroy(void);

rigidbody_2d *PHYS_2D_AddRigidbody(AABB_2D *_aabb);
rigidbody_2d *PHYS_2D_AddRigidbodyEx(AABB_2D *_aabb, float mass, float friction, float restitution, float gravMult /*, void (*onCollide)(rigidbody_2d, vec3) */);
void PHYS_2D_RemoveRigidbody(rigidbody_2d *body);

void PHYS_2D_Tick(float time);

rigidbody_2d *PHYS_2D_get_bodies(void);
int PHYS_2D_get_body_index(rigidbody_2d *body);
int PHYS_2D_get_active(void);
