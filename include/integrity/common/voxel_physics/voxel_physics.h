#pragma once

#include <cglm/cglm.h>
#include <integrity/common/common.h>

#include "aabb_3d.h"
#include "rigidbody.h"

typedef struct chunk chunk;

typedef struct PhysicsOptions {
  vec3 gravity;
  float minBounceImpulse;  // lowest collision impulse that bounces
  float airDrag;
  float fluidDrag;
  float fluidDensity;
} PhysicsOptions;

typedef bool (*blockTest)(chunk*, int, int, int);

typedef struct PhysicsSystem {
  vec3 gravity;
  float airDrag;
  float fluidDensity;
  float fluidDrag;
  float minBounceImpulse;
  rigidbody* bodies;

  blockTest testSolid;
  blockTest testFluid;
} PhysicsSystem;

void PHYS_CreateEx(PhysicsOptions opts, blockTest testSolid, blockTest testFluid);
void PHYS_Create(blockTest testSolid, blockTest testFluid);

rigidbody* PHYS_AddRigidbody(AABB* _aabb);
rigidbody* PHYS_AddRigidbodyEx(AABB* _aabb, float mass, float friction, float restitution, float gravMult, void (*onCollide)(rigidbody*, vec3));
void PHYS_RemoveRigidbody(rigidbody* body);

void PHYS_Tick(float time);
void PHYS_iterateBody(PhysicsSystem* physics, rigidbody* body, float time, bool noGravity);
bool PHYS_bodyAsleep(PhysicsSystem* physics, rigidbody* body, float time, bool noGravity);
void PHYS_applyFluidForces(PhysicsSystem* physics, rigidbody* body);
void PHYS_applyFrictionByAxis(int axis, rigidbody* body, vec3 dvel);

float PHYS_processCollisions(PhysicsSystem* physics, AABB* box, vec3 velocity, vec3 resting);
void PHYS_tryAutoStepping(PhysicsSystem* physics, rigidbody* body, AABB* oldBox, vec3 dx);

static inline bool equals(float a, float b) {
  return fabsf(a - b) < 1e-5f;
}

// Sweep Definitions
typedef bool (*CollisionCallback)(float /* dist */, int /* axis */, int /* dir */, float* /* vec */);
