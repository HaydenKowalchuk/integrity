#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define CUTE_C2_IMPLEMENTATION
#include <integrity/common/2d_physics/2d_physics.h>
#include <integrity/common/common.h>

#define MAX_RIGIDBODIES 256
static rigidbody_2d _bodies[MAX_RIGIDBODIES];
static int _numActive;

static void deactivateRigidbody2D(int index);
static void activateRigidbody2D(int index);
static void mark_inactive(int index);
static void consolidate_actives(void);
static void PHYS_2D_generate_pairs(void);
static void resolve_collision(rigidbody_2d* A, rigidbody_2d* B, c2Manifold* manifold);

int sort_pairs(const void* /*Pair*/ _lhs, const void* /*Pair*/ _rhs);

static PhysicsOptions_2D PhysicsDefaults = {
    .gravity = {0, -10, 0},
    .minBounceImpulse = 0.5f,
    .airDrag = 0.1f,
    .fluidDrag = 0.4f,
    .fluidDensity = 2.0f,
};

static PhysicsOptions_2D _currentOptions;

// Our collision pairs and unique collisions after filtering
static Pair _initial_pairs[256];
static Pair* _unique_pairs[256];
static int _numpairs;

typedef struct Box {
  vec3 pos;
  c2AABB shape;
} Box_t;

void PHYS_2D_CreateEx(PhysicsOptions_2D opts) {
  memcpy(&_currentOptions, &opts, sizeof(PhysicsOptions_2D));
  _numpairs = 0;
  _numActive = 0;
}
void PHYS_2D_Create(void) {
  PHYS_2D_CreateEx(PhysicsDefaults);
}

void PHYS_2D_Destroy(void) {
  memset(_initial_pairs, 0, sizeof(_initial_pairs));
  memset(_unique_pairs, 0, sizeof(_unique_pairs));
  memset(_bodies, 0, sizeof(_bodies));
  _numpairs = 0;
  _numActive = 0;
}

/* Notes:

lower is higher
top is 0
bottom is 480

Inverted

*/

void PHYS_2D_Tick(float time) {
  static const float friction = 1.0f - 0.02f;
  for (int i = 0; i < _numActive; i++) {
    // add New forces and Current gravity to existing forces
    _bodies[i]._force[0] += _bodies[i].forces[0] + _bodies[i]._impulses[0] + (_currentOptions.gravity[0] * -1 * _bodies[i].gravityMultiplier);
    _bodies[i]._force[1] += _bodies[i].forces[1] + _bodies[i]._impulses[1] + (_currentOptions.gravity[1] * -1 * _bodies[i].gravityMultiplier);
    glm_vec2_zero(_bodies[i]._impulses);
  }
  PHYS_2D_generate_pairs();

  for (int j = 0; j < _numpairs; j++) {
    if (_unique_pairs[j]->A && _unique_pairs[j]->B) {
      // Check for collision
      c2Manifold manifold;

      c2AABBtoAABBManifold(_unique_pairs[j]->A->shape, _unique_pairs[j]->B->shape, &manifold);
      if (manifold.count > 0) {
        resolve_collision(_unique_pairs[j]->A, _unique_pairs[j]->B, &manifold);
        if (_unique_pairs[j]->A->inv_mass > 0) {
          AABB_2D_Translate(&_unique_pairs[j]->A->shape, (vec2){_unique_pairs[j]->A->_force[0] * time, _unique_pairs[j]->A->_force[1] * time});
          //_unique_pairs[j]->A->_force[0] = _unique_pairs[j]->A->_force[1] = 0.0f;
        }

        if (_unique_pairs[j]->B->inv_mass > 0) {
          AABB_2D_Translate(&_unique_pairs[j]->B->shape, (vec2){_unique_pairs[j]->B->_force[0] * time, _unique_pairs[j]->B->_force[1] * time});
          //_unique_pairs[j]->B->_force[0] = _unique_pairs[j]->B->_force[1] = 0.0f;
        }
      }
    }
  }

  for (int i = 0; i < _numActive; i++) {
    if (_bodies[i].inv_mass > 0) {
      AABB_2D_Translate(&_bodies[i].shape, (vec2){_bodies[i]._force[0] * time, _bodies[i]._force[1] * time});
      _bodies[i]._force[0] *= friction;
      // glm_vec2_scale(_bodies[i]._force, friction, _bodies[i]._force);
    }
  }

  // consolidate_actives();
}

rigidbody_2d* PHYS_2D_AddRigidbody(AABB_2D* _aabb) {
  return PHYS_2D_AddRigidbodyEx(_aabb, 1, 1, 1, 1);
}

rigidbody_2d* PHYS_2D_AddRigidbodyEx(AABB_2D* _aabb, float mass, float friction, float restitution, float gravMult /*, void (*onCollide)(rigidbody_2d, vec3) */) {
  activateRigidbody2D(_numActive);
  _bodies[_numActive - 1] = RBDY_2D_Create(_aabb, mass, friction, restitution, gravMult /*, onCollide, false */);
  _bodies[_numActive - 1].active = true;
  return &_bodies[_numActive - 1];
}

void PHYS_2D_RemoveRigidbody(rigidbody_2d* body) {
  mark_inactive(PHYS_2D_get_body_index(body));
}

/*
 *    ADDING AND REMOVING RIGID BODIES
 */
static void activateRigidbody2D(int index) {
  // printf("Active index: %d, _numActive: %d\n", index, _numActive);
  //  Shouldn't already be active!
  assert(index >= _numActive);

  // Swap it with the first inactive Rigidbody
  // right after the active ones.
  rigidbody_2d temp = _bodies[_numActive];
  _bodies[_numActive] = _bodies[index];
  _bodies[index] = temp;

  // Now there's one more.
  _numActive++;
}

static void deactivateRigidbody2D(int index) {
  // Shouldn't already be inactive!
  assert(index < _numActive);

  // There's one fewer.
  _numActive--;

  // Swap it with the last active Rigidbody
  // right before the inactive ones.
  rigidbody_2d temp = _bodies[_numActive];
  _bodies[_numActive] = _bodies[index];
  _bodies[index] = temp;
}

static void mark_inactive(int index) {
  _bodies[index].active = false;
}

static void consolidate_actives(void) __attribute((unused));
static void consolidate_actives(void) {
  for (int i = 0; i < _numActive; i++) {
    // printf("Consolidate check\n");
    if (!_bodies[i].active) {
      // printf("\tremove %d\n", i);
      deactivateRigidbody2D(i);
    }
  }
}

rigidbody_2d* PHYS_2D_get_bodies(void) {
  return _bodies;
}

int PHYS_2D_get_body_index(rigidbody_2d* body) {
  return (body - &_bodies[0]);
}

int PHYS_2D_get_active(void) {
  return _numActive;
}

static void resolve_collision(rigidbody_2d* A, rigidbody_2d* B, c2Manifold* manifold) {
  /* onCollide Callbacks */
  if (A->onCollide != NULL) {
    (*A->onCollide)((void*)A, (void*)B);
    // A->layer = 0;
  }
  if (B->onCollide != NULL) {
    (*B->onCollide)((void*)B, (void*)A);
    // B->layer = 0;
  }

  // Calculate relative velocity
  vec2 rv;
  glm_vec2_sub(B->_force, A->_force, rv);

  // printf("Force A: {%f, %f}, Force B: {%f, %f}\n", A->_force[0], A->_force[1], B->_force[0], B->_force[1]);

  // Calculate relative velocity in terms of the normal direction
  vec2 normal;
  memcpy(&normal, &manifold->n, sizeof(vec2));
  // printf("Normal: {%f, %f}\n", normal[0], normal[1]);
  float velAlongNormal = glm_vec2_dot(rv, normal);

  // Do not resolve if velocities are separating
  if (velAlongNormal > 0)
    return;

  // printf("RV: {%f, %f} vel: %f\n", rv[0], rv[1], velAlongNormal);

  // Calculate restitution
  float e = MIN(A->restitution, B->restitution);

  // Calculate impulse scalar
  float j = -(1 + e) * velAlongNormal;
  j /= A->inv_mass + B->inv_mass;
  j /= 4;

  // Apply impulse
  vec2 impulse;
  glm_vec2_scale(normal, j, impulse);

  // printf("Impulse: {%f, %f} vel: %f\n", impulse[0], impulse[1], j);

  const float percent = 20.0f / 100.0f;  // usually 20% to 80%
  const float slop = 0.01f;              // usually 0.01 to 0.1

#if 0
    /* Old Examples */
    //Vec2 correction = MAX( penetration - k_slop, 0.0f ) / (A->inv_mass + B->inv_mass)) * percent * n;

    //correction[0] = normal[0] * MAX(manifold->depths[0] - slop, 0.0f) / (A->inv_mass + B->inv_mass) * percent;
    //correction[1] = normal[1] * MAX(manifold->depths[0] - slop, 0.0f) / (A->inv_mass + B->inv_mass) * percent;
    //correction[0] = normal[0] * (manifold->depths[0] - slop) / (A->inv_mass + B->inv_mass) * percent;

    //glm_vec2_scale(normal, MAX(manifold->depths[0] - slop, 0.0f) / (A->inv_mass + B->inv_mass) * percent, correction);
    //printf("depth: %f\ncorrect: %2.5f, correct: %2.5f\n", manifold->depths[0], correction[0], correction[1]);
    //printf("Impulse: %f, %f J: %f\n", impulse[0], impulse[1], j);
#endif

  vec2 correction;
  correction[0] = 0;
  correction[1] = normal[1] * (manifold->depths[0] - slop) / (A->inv_mass + B->inv_mass) * percent;
  /*
  printf("Impulse: %f, %f Normal: %f %f\n", impulse[1]*A->inv_mass, impulse[1]*B->inv_mass, normal[0], normal[1]);

  if (((fabs(impulse[1])*B->inv_mass) < 1) || (fabs(impulse[1])*A->inv_mass) < 1)
  {
      //impulse[1] = 0;
  }
  */

  // glm_vec2_sub(correction, impulse, A->_force);
  // glm_vec2_scale(A->_force, A->inv_mass, A->_force);

  /* Original */
  /*
  glm_vec2_add(correction, impulse, B->_force);
  glm_vec2_scale(B->_force, B->inv_mass, B->_force);
  */

  /* New */
  vec2 temp;
  glm_vec2_add(correction, impulse, temp);
  glm_vec2_scale(temp, B->inv_mass, temp);
  if (impulse[0] != 0) {
    B->_force[0] = temp[0];
    if (B->_force[0] > -1.0f && B->_force[0] < 1.0f) {
      B->_force[0] = 0.0f;
    }
  } else {
    B->_force[1] = temp[1];
    A->_force[0] *= B->friction;
    if (B->_force[1] > -1.0f && B->_force[1] < 1.0f) {
      B->_force[1] = 0.0f;
    }
  }
  extern rigidbody_2d* PLYR_2D_Get(void);
  extern void PLYR_2D_ResetJumpSimple(void);

  if (A == PLYR_2D_Get()) {
    PLYR_2D_ResetJumpSimple();
  }

  // c2AABB new_pos = (c2AABB){.min = {.x = B->shape.min.x + temp[0], .y = B->shape.min.y + temp[1]}, .max = {.x = B->shape.max.x + temp[0], .y = B->shape.max.y + temp[1]}};

#if 0
    if (temp[1] != 0.0f)
    {

        for (int i = 0; i < _numActive; i++)
        {
            rigidbody_2d *other = &_bodies[i];
            // Skip check with self
            if (B == other || other == A)
                continue;

            c2Manifold manifold2;

            c2AABBtoAABBManifold(other->shape, new_pos, &manifold2);
            if (manifold2.count > 0)
            //if (c2AABBtoAABB(other->shape, new_pos))
            {
                if (other->shape.max.y >= new_pos.min.y)
                {

                    //B->_force[1] -= (manifold2.depths[0]+(other->shape.max.y-other->shape.min.y));
                    //other->_force[1] -= (manifold2.depths[0]+(other->shape.max.y-other->shape.min.y));
                    //B->_force[1] = 0;
                    //other->_force[1] -= (manifold->depths[0] + (other->shape.max.y - other->shape.min.y));

                    /* Good ish */
                    B->_force[1] = manifold->depths[0];
                    A->_force[1] = fabs(A->_force[1]);
                    //printf("%f Below us! depth:{%f, %f} height: %f, %f\n", other->mass, manifold->depths[0], manifold->depths[1], other->shape.max.y - other->shape.min.y, Sys_FloatTime());
                }
            }
        }
    }
#endif

  glm_vec2_add(correction, impulse, temp);
  glm_vec2_scale(temp, -A->inv_mass, temp);
  if (impulse[0] != 0) {
    A->_force[0] = temp[0];
    if (A->_force[0] > -1.0f && A->_force[0] < 1.0f) {
      A->_force[0] = 0.0f;
    }
  } else {
    A->_force[1] = temp[1];
    B->_force[0] *= A->friction;

    if (A->_force[1] > -1.0f && A->_force[1] < 1.0f) {
      A->_force[1] = 0.0f;
    }
  }

  // new_pos = (c2AABB){.min = {.x = A->shape.min.x + temp[0], .y = A->shape.min.y + temp[1]}, .max = {.x = A->shape.max.x + temp[0], .y = A->shape.max.y + temp[1]}};

#if 0
    for (int i = 0; i < _numActive; i++)
    {
        rigidbody_2d *other = &_bodies[i];
        // Skip check with self
        if (B == other || other == A)
            continue;

        /* Implement Later
        // Only matching layers will be considered
        if (!(A->layers & B->layers))
            continue;
        */

        if (c2AABBtoAABB(other->shape, new_pos))
        {
            if(other->shape.max.y >= new_pos.min.y){
                //other->_force[1] = 0;
                A->_force[1] += (manifold->depths[0]+(other->shape.max.y-other->shape.min.y));
                other->_force[1] = 0;
                extern rigidbody_2d *PLYR_2D_Get(void);
                extern void PLYR_2D_ResetJumpSimple();
                if(A == PLYR_2D_Get()){
                    PLYR_2D_ResetJumpSimple();
                }
                //printf("Below us! depth:{%f, %f} height: %f, %f\n", manifold->depths[0], manifold->depths[1], other->shape.max.y-other->shape.min.y, Sys_FloatTime());
            }
        }
    }
#endif

  // printf("Force A: {%f, %f}, Force B: {%f, %f}\n", A->_force[0], A->_force[1], B->_force[0], B->_force[1]);
}

int sort_pairs(const void* /*Pair*/ _lhs, const void* /*Pair*/ _rhs) {
  const Pair* lhs = _lhs;
  const Pair* rhs = _rhs;

  if (lhs->A < rhs->A)
    return -1;

  if (lhs->A == rhs->A)
    return lhs->B < rhs->B;

  return 1;
}

// Generates the pair list.
// All previous pairs are cleared when this function is called.
void PHYS_2D_generate_pairs(void) {
  memset(_initial_pairs, 0, sizeof(_initial_pairs));
  memset(_unique_pairs, 0, sizeof(_unique_pairs));
  _numpairs = 0;

  for (int i = 0; i < _numActive; i++) {
    rigidbody_2d* A = &_bodies[i];
    if (!A->active)
      continue;

    for (int j = 0; j < _numActive; j++) {
      rigidbody_2d* B = &_bodies[j];
      if (!B->active)
        continue;

      // Skip check with self
      if (A == B)
        continue;

      // Only matching layers will be considered
      if (!(A->layer & B->layer))
        continue;

      /* static check */
      if (A->mass + B->mass < 1)
        continue;

      if (c2AABBtoAABB(A->shape, B->shape)) {
        if (A->shape.max.y < B->shape.max.y)
          _initial_pairs[_numpairs++] = (Pair){A, B};
        else
          _initial_pairs[_numpairs++] = (Pair){B, A};
      }
    }
  }
  // Sort pairs to expose duplicates
  qsort(_initial_pairs, _numpairs, sizeof(Pair), sort_pairs);

  // Queue manifolds for solving
  {
    int i = 0;
    int num_unique = 0;
    while (i < _numpairs) {
      Pair* pair = &_initial_pairs[i];

      _unique_pairs[num_unique++] = pair;

      ++i;

      // Skip duplicate pairs by iterating i until we find a unique pair
      while (i < _numpairs) {
        Pair* potential_dup = &_initial_pairs[i];
        if (pair->A != potential_dup->B || pair->B != potential_dup->A)
          break;
        ++i;
      }
    }
    _numpairs = num_unique;
  }
}
