#include <integrity/common/voxel_physics/voxel_physics.h>

#include <assert.h>

#include <integrity/common/voxel_physics/sweep.h>

extern chunk *WLRD_GetChunkAt(int x, int y, int z);

#define MAX_RIGIDBODIES 256
static rigidbody _bodies[MAX_RIGIDBODIES];
static int _numActive;

static void deactivateRigidbody(int index);
static void activateRigidbody(int index);

static PhysicsOptions PhysicsDefaults = {
    .gravity = {0, -10, 0},
    .minBounceImpulse = 0.5f,
    .airDrag = 0.1f,
    .fluidDrag = 0.4f,
    .fluidDensity = 2.0f,
};

PhysicsSystem physics;

/*
 *     LOCALS
 */
static vec3 accel;
static vec3 dv;
static vec3 dx;
static vec3 impacts;
static vec3 oldResting;

static AABB tmpBox;
static vec3 tmpResting;
static vec3 targetPos;
static vec3 upvec;
static vec3 leftover;

static vec3 sleepVec;
static vec3 lateralVel;

static void cloneAABB(AABB *target, AABB *source);

void PHYS_Create(blockTest testSolid, blockTest testFluid)  //, chunk *_chunk)
{
  PHYS_CreateEx(PhysicsDefaults, testSolid, testFluid);  //, _chunk);
}

void PHYS_CreateEx(PhysicsOptions opts, blockTest testSolid, blockTest testFluid)  //, chunk *_chunk)
{
  glm_vec3_copy(opts.gravity, physics.gravity);
  physics.airDrag = opts.airDrag;
  physics.fluidDensity = opts.fluidDensity;
  physics.fluidDrag = opts.fluidDrag;
  physics.minBounceImpulse = opts.minBounceImpulse;
  physics.bodies = _bodies;

  // collision function - TODO: abstract this into a setter?
  physics.testSolid = testSolid;
  physics.testFluid = testFluid;
  //physics.chunk = _chunk;

  /* SO MANY LOCALS */
  glm_vec3_copy(GLM_VEC3_ZERO, accel);
  glm_vec3_copy(GLM_VEC3_ZERO, dv);
  glm_vec3_copy(GLM_VEC3_ZERO, dx);
  glm_vec3_copy(GLM_VEC3_ZERO, impacts);
  glm_vec3_copy(GLM_VEC3_ZERO, oldResting);
  glm_vec3_copy(GLM_VEC3_ZERO, lateralVel);

  tmpBox = AABB_Create(GLM_VEC3_ZERO, GLM_VEC3_ONE);
  glm_vec3_copy(GLM_VEC3_ZERO, tmpResting);
  glm_vec3_copy(GLM_VEC3_ZERO, targetPos);
  glm_vec3_copy(GLM_VEC3_ZERO, upvec);
  glm_vec3_copy(GLM_VEC3_ZERO, leftover);

  glm_vec3_copy(GLM_VEC3_ZERO, sleepVec);
}

/*
 *    ADDING AND REMOVING RIGID BODIES
*/
static void activateRigidbody(int index) {
  // Shouldn't already be active!
  assert(index >= _numActive);

  // Swap it with the first inactive Rigidbody
  // right after the active ones.
  rigidbody temp = _bodies[_numActive];
  _bodies[_numActive] = _bodies[index];
  _bodies[index] = temp;

  // Now there's one more.
  _numActive++;
}
static void deactivateRigidbody(int index) {
  // Shouldn't already be inactive!
  assert(index < _numActive);

  // There's one fewer.
  _numActive--;

  // Swap it with the last active Rigidbody
  // right before the inactive ones.
  rigidbody temp = _bodies[_numActive];
  _bodies[_numActive] = _bodies[index];
  _bodies[index] = temp;
}

rigidbody *PHYS_AddRigidbody(AABB *_aabb) {
  return PHYS_AddRigidbodyEx(_aabb, 1.0f, 0.8f, 0.0f, 1.0f, NULL);
}

rigidbody *PHYS_AddRigidbodyEx(AABB *_aabb, float mass, float friction,
                               float restitution, float gravMult, void (*onCollide)(rigidbody *, vec3)) {
  activateRigidbody(_numActive);
  _bodies[_numActive - 1] = RBDY_Create(_aabb, mass, friction, restitution, gravMult, onCollide, /*autoStep*/ false);
  return &_bodies[_numActive - 1];
}

void PHYS_RemoveRigidbody(rigidbody *body) {
  for (int i = 0; i < _numActive; i++) {
    if (body->timestamp == _bodies[i].timestamp) {
      deactivateRigidbody(i);
      return;
    }
  }
}

/*
 *    PHYSICS AND COLLISIONS
*/

/*
 *    TICK HANDLER
*/

void PHYS_Tick(float time) {
  bool noGravity = equals(0.0f, glm_vec3_norm2(physics.gravity));

  for (int i = 0; i < _numActive; i++) {
    PHYS_iterateBody(&physics, &_bodies[i], time, noGravity);
  }
}

void PHYS_iterateBody(PhysicsSystem *_physics, rigidbody *body, float time, bool noGravity) {
  glm_vec3_copy(body->resting, oldResting);

  // treat bodies with <= mass as static
  if (body->mass <= 0) {
    glm_vec3_copy(GLM_VEC3_ZERO, body->velocity);
    glm_vec3_copy(GLM_VEC3_ZERO, body->_forces);
    glm_vec3_copy(GLM_VEC3_ZERO, body->_impulses);
    return;
  }

  // skip bodies if static or no velocity/forces/impulses
  bool localNoGrav = noGravity || (equals(body->gravityMultiplier, 0));
  if (PHYS_bodyAsleep(_physics, body, time, localNoGrav)) {
    return;
  }
  body->_sleepFrameCount--;

  // check if under water, if so apply buoyancy and drag forces
  PHYS_applyFluidForces(_physics, body);

  // semi-implicit Euler integration

  // accel = f/m + gravity*gravityMultiplier
  glm_vec3_scale(body->_forces, body->inv_mass, accel);
  glm_vec3_muladds(_physics->gravity, body->gravityMultiplier, accel);

  // dv = i/m + accel*dt
  // v1 = v0 + dv
  glm_vec3_scale(body->_impulses, body->inv_mass, dv);
  glm_vec3_muladds(accel, time, dv);
  glm_vec3_add(body->velocity, dv, body->velocity);

  // apply friction based on change in velocity this frame
  if (!equals(body->friction, 0)) {
    PHYS_applyFrictionByAxis(0, body, dv);
    PHYS_applyFrictionByAxis(1, body, dv);
    PHYS_applyFrictionByAxis(2, body, dv);
  }

  // linear air or fluid friction - effectively v *= drag
  // body settings override global settings
  float drag = (body->airDrag >= 0) ? body->airDrag : _physics->airDrag;
  if (body->inFluid) {
    drag = (body->fluidDrag >= 0) ? body->fluidDrag : _physics->fluidDrag;
    drag *= 1 - ((1 - body->_ratioInFluid) * (1 - body->_ratioInFluid));
  }
  float mult = fmax(1 - drag * time / body->mass, 0);
  glm_vec3_scale(body->velocity, mult, body->velocity);

  // x1-x0 = v1*dt
  glm_vec3_scale(body->velocity, time, dx);

  // clear forces and impulses for next timestep
  glm_vec3_copy(GLM_VEC3_ZERO, body->_forces);
  glm_vec3_copy(GLM_VEC3_ZERO, body->_impulses);

  // cache old position for use in autostepping
  if (body->autoStep) {
    cloneAABB(&tmpBox, &body->aabb);
  }
  // sweeps aabb along dx and accounts for collisions
  PHYS_processCollisions(_physics, &body->aabb, dx, body->resting);

  // if autostep, and on ground, run collisions again with stepped up aabb
  if (body->autoStep) {
    PHYS_tryAutoStepping(_physics, body, &tmpBox, dx);
  }

  // Collision impacts. b.resting shows which axes had collisions:
  for (int i = 0; i < 3; i++) {
    impacts[i] = 0;
    if (body->resting[i]) {
      // count impact only if wasn't collided last frame
      if (oldResting[i])
        impacts[i] = -body->velocity[i];
      body->velocity[i] = 0;
    }
  }

  float mag = glm_vec3_norm(impacts);
  if (mag > .001f) {  // epsilon
    // send collision event - allows client to optionally change
    // body's restitution depending on what terrain it hit
    // event argument is impulse J = m * dv
    glm_vec3_scale(impacts, body->mass, impacts);
    if (body->onCollide)
      body->onCollide((void *)body, (float *)(impacts));

    // bounce depending on restitution and minBounceImpulse
    if (body->restitution > 0 && mag > _physics->minBounceImpulse) {
      glm_vec3_scale(impacts, body->restitution, impacts);
      RBDY_ApplyImpulse(body, impacts);
    }
  }

  // sleep check
  float vsq = glm_vec3_norm2(body->velocity);
  if (vsq > 0.00001f)
    RBDY_MarkActive(body);
}

/*
 *    FLUIDS
*/

void PHYS_applyFluidForces(PhysicsSystem *_physics __attribute((unused)), rigidbody *body __attribute((unused))) {
}

/*
 *    FRICTION
*/

void PHYS_applyFrictionByAxis(int axis, rigidbody *body, vec3 dvel) {
  // friction applies only if moving into a touched surface
  float restDir = body->resting[axis];
  float vNormal = dvel[axis];
  if (restDir == 0.0f)
    return;
  if (restDir * vNormal <= 0)
    return;

  // current vel lateral to friction axis
  glm_vec3_copy(body->velocity, lateralVel);
  lateralVel[axis] = 0;
  float vCurr = glm_vec3_norm(lateralVel);
  if (equals(vCurr, 0.0f))
    return;

  // treat current change in velocity as the result of a pseudoforce
  //        Fpseudo = m*dv/dt
  // Base friction force on normal component of the pseudoforce
  //        Ff = u * Fnormal
  //        Ff = u * m * dvnormal / dt
  // change in velocity due to friction force
  //        dvF = dt * Ff / m
  //            = dt * (u * m * dvnormal / dt) / m
  //            = u * dvnormal
  float dvMax = fabsf(body->friction * vNormal);

  // decrease lateral vel by dvMax (or clamp to zero)
  float scaler = (vCurr > dvMax) ? (vCurr - dvMax) / vCurr : 0;
  body->velocity[(axis + 1) % 3] *= scaler;
  body->velocity[(axis + 2) % 3] *= scaler;
}

/*
 *    COLLISION HANDLER
*/
static float *_resting;
static bool PHYS_basicCollisonCallback(float dist, int axis, int dir, float *vec) {
  _resting[axis] = dir;
  vec[axis] = 0;
  (void)dist;
  return false;
}

// sweep aabb along velocity vector and set resting vector
float PHYS_processCollisions(PhysicsSystem *_physics, AABB *box, float *velocity, float *resting) {
  glm_vec3_copy(GLM_VEC3_ZERO, resting);

  _resting = resting;
  chunk *current_chunk = WLRD_GetChunkAt((int)box->base[0], (int)box->base[1], (int)box->base[2]);
  return sweep(_physics->testSolid, current_chunk, box, velocity, &PHYS_basicCollisonCallback, false);
}

/*
 *    AUTO-STEPPING
*/

void PHYS_tryAutoStepping(PhysicsSystem *_physics __attribute((unused)), rigidbody *body __attribute((unused)), AABB *oldBox __attribute((unused)), vec3 _dx __attribute((unused))) {
#if 0
    if (b.resting[1] >= 0 && !b.inFluid) return

    // // direction movement was blocked before trying a step
    var xBlocked = (b.resting[0] !== 0)
    var zBlocked = (b.resting[2] !== 0)
    if (!(xBlocked || zBlocked)) return

    // continue autostepping only if headed sufficiently into obstruction
    var ratio = Math.abs(dx[0] / dx[2])
    var cutoff = 4
    if (!xBlocked && ratio > cutoff) return
    if (!zBlocked && ratio < 1 / cutoff) return

    // original target position before being obstructed
    vec3.add(targetPos, oldBox.base, dx)

    // move towards the target until the first X/Z collision
    var getVoxels = self.testSolid
    sweep(getVoxels, oldBox, dx, function (dist, axis, dir, vec) {
        if (axis === 1) vec[axis] = 0
        else return true
    })

    var y = b.aabb.base[1]
    var ydist = Math.floor(y + 1.001) - y
    vec3.set(upvec, 0, ydist, 0)
    var collided = false
    // sweep up, bailing on any obstruction
    sweep(getVoxels, oldBox, upvec, function (dist, axis, dir, vec) {
        collided = true
        return true
    })
    if (collided) return // could't move upwards

    // now move in X/Z however far was left over before hitting the obstruction
    vec3.subtract(leftover, targetPos, oldBox.base)
    leftover[1] = 0
    processCollisions(self, oldBox, leftover, tmpResting)

    // bail if no movement happened in the originally blocked direction
    if (xBlocked && !equals(oldBox.base[0], targetPos[0])) return
    if (zBlocked && !equals(oldBox.base[2], targetPos[2])) return

    // done - oldBox is now at the target autostepped position
    cloneAABB(b.aabb, oldBox)
    b.resting[0] = tmpResting[0]
    b.resting[2] = tmpResting[2]
    if (b.onStep) b.onStep()
#endif
}

/*
 *    SLEEP CHECK
*/
bool *_isResting;
static bool PHYS_sleepingCollisonCallback(float dist, int axis, int dir, float *vec) {
  (void)dist;
  (void)axis;
  (void)dir;
  (void)vec;
  *_isResting = true;
  return true;
}
bool PHYS_bodyAsleep(PhysicsSystem *_physics, rigidbody *body, float time, bool noGravity) {
  if (body->_sleepFrameCount > 0)
    return false;
  // without gravity bodies stay asleep until a force/impulse wakes them up
  if (noGravity)
    return true;
  // otherwise check body is resting against something
  // i.e. sweep along by distance d = 1/2 g*t^2
  // and check there's still a collision
  bool isResting = false;
  float gmult = 0.5f * time * time * body->gravityMultiplier;
  glm_vec3_scale(_physics->gravity, gmult, sleepVec);
  _isResting = &isResting;
  chunk *current_chunk = WLRD_GetChunkAt((int)body->aabb.base[0], (int)body->aabb.base[1], (int)body->aabb.base[2]);
  sweep(_physics->testSolid, current_chunk, &body->aabb, &(sleepVec[0]), &PHYS_sleepingCollisonCallback, true);
  return isResting;
}

static void cloneAABB(AABB *target, AABB *source) {
  glm_vec3_copy(source->base, target->base);
  glm_vec3_copy(source->max, target->max);
  glm_vec3_copy(source->vec, target->vec);
}
