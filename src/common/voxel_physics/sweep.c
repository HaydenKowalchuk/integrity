#include <integrity/common/voxel_physics/sweep.h>

static vec3 tr;
static int ldi[3];
static int tri[3];
static int step[3];
static vec3 tDelta;
static vec3 tNext;
static vec3 vec;
static vec3 normed;
static vec3 base;
static vec3 sweep_max;
static vec3 left;

static float cumulative_t = 0.0;
static float par_t = 0.0;
static float max_t = 0.0;
static int sweep_axis = 0;
static float epsilon;
static blockTest getVoxel;
static void* __chunk;
static CollisionCallback callback;

#define ERROR_CORRECTION (float)(1e-5);
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static inline int leadEdgeToInt(float coord, int _step) {
  return (int)floor(coord - _step * epsilon);
}
static inline int trailEdgeToInt(float coord, int _step) {
  return (int)floor(coord + _step * epsilon);
}

// low-level implementations of each step:
static void initSweep(void) {
  // parametrization t along raycast
  par_t = 0.0f;
  max_t = glm_vec3_norm(vec);

  if (equals(max_t, 0.0f))
    return;
  for (int i = 0; i < 3; i++) {
    int dir = (vec[i] >= 0);
    step[i] = dir ? 1 : -1;
    // trailing / trailing edge coords
    float lead = dir ? sweep_max[i] : base[i];
    tr[i] = dir ? base[i] : sweep_max[i];
    // int values of lead/trail edges
    ldi[i] = leadEdgeToInt(lead, step[i]);
    tri[i] = trailEdgeToInt(tr[i], step[i]);
    // normed vector
    normed[i] = vec[i] / max_t;
    if (normed[i] >= HUGE_VALF || equals(normed[i], 0.0f)) {
      tDelta[i] = HUGE_VALF;
    } else {
      // distance along par_t required to move one voxel in each axis
      tDelta[i] = fabsf(1.0f / normed[i]);
    }
    // location of nearest voxel boundary, in units of par_t
    float dist = dir ? (ldi[i] + 1 - lead) : (lead - ldi[i]);
    tNext[i] = (tDelta[i] < HUGE_VALF) ? tDelta[i] * dist : HUGE_VALF;
  }
}
// check for collisions - iterate over the leading face on the advancing axis

static bool checkCollision(int i_axis) {
  int stepx = step[0];
  int x0 = (i_axis == 0) ? ldi[0] : tri[0];
  int x1 = ldi[0] + stepx;

  int stepy = step[1];
  int y0 = (i_axis == 1) ? ldi[1] : tri[1];
  int y1 = ldi[1] + stepy;

  int stepz = step[2];
  int z0 = (i_axis == 2) ? ldi[2] : tri[2];
  int z1 = ldi[2] + stepz;

  int x, y, z;
  for (x = x0; x != x1; x += stepx) {
    for (y = y0; y != y1; y += stepy) {
      for (z = z0; z != z1; z += stepz) {
        if (getVoxel(__chunk, x, y, z)) {
          return true;
        }
      }
    }
  }
  return false;
}

// on collision - call the callback and return or set up for the next sweep

static bool handleCollision(void) {
  // set up for callback
  cumulative_t += par_t;
  int dir = step[sweep_axis];

  // vector moved so far, and left to move
  float done = par_t / max_t;

  for (int i = 0; i < 3; i++) {
    float dv = vec[i] * done;
    base[i] += dv;
    sweep_max[i] += dv;
    left[i] = vec[i] - dv;
  }

  // set leading edge of stepped axis exactly to voxel boundary
  // else we'll sometimes rounding error beyond it

  if (dir > 0) {
    sweep_max[sweep_axis] = roundf(sweep_max[sweep_axis]) - ERROR_CORRECTION;
  } else {
    base[sweep_axis] = roundf(base[sweep_axis]) + ERROR_CORRECTION;
  }

  // call back to let client update the "left to go" vector
  bool res = (*callback)(cumulative_t, sweep_axis, dir, left);

  // bail out out on truthy response
  if (res)
    return true;

  // init for new sweep along vec
  glm_vec3_copy(left, vec);
  initSweep();
  if (equals(max_t, 0))
    return true;  // no vector left

  return false;
}

static int stepForward(void) {
  int _axis = (tNext[0] < tNext[1]) ? ((tNext[0] < tNext[2]) ? 0 : 2) : ((tNext[1] < tNext[2]) ? 1 : 2);
  float dt = tNext[_axis] - par_t;
  par_t = tNext[_axis];
  ldi[_axis] += step[_axis];
  tNext[_axis] += tDelta[_axis];
  for (int i = 0; i < 3; i++) {
    tr[i] += dt * normed[i];
    tri[i] = trailEdgeToInt(tr[i], step[i]);
  }

  return _axis;
}

// core implementation:

// static float sweep_impl(blockTest _getVoxel, void *_chunk, CollisionCallback _callback, vec3 _vec, vec3 _base, vec3 _max)
static float sweep_impl(blockTest _getVoxel, void* _chunk, CollisionCallback _callback) {
  // consider algo as a raycast along the AABB's leading corner
  // as raycast enters each new voxel, iterate in 2D over the AABB's
  // leading face in that axis looking for collisions
  //
  // original raycast implementation: https://github.com/andyhall/fast-voxel-raycast
  // original raycast paper: http://www.cse.chalmers.se/edu/year/2010/course/TDA361/grid.pdf

  getVoxel = _getVoxel;
  __chunk = _chunk;
  callback = _callback;

  cumulative_t = 0.0;
  par_t = 0.0;
  max_t = 0.0;
  sweep_axis = 0;

  // init for the current sweep vector and take first step
  initSweep();
  if (equals(max_t, 0))
    return 0.0f;

  sweep_axis = stepForward();

  // loop along raycast vector
  while (par_t <= max_t) {
    // sweeps over leading face of AABB
    if (checkCollision(sweep_axis)) {
      // calls the callback and decides whether to continue
      bool done = handleCollision();
      if (done)
        return cumulative_t;
    }

    sweep_axis = stepForward();
  }

  // reached the end of the vector unobstructed, finish and exit
  cumulative_t += max_t;
  glm_vec3_add(base, vec, base);
  glm_vec3_add(sweep_max, vec, sweep_max);
  return cumulative_t;
}

float sweep(blockTest _getVoxel, void* _chunk, AABB* box, vec3 dir, CollisionCallback _callback, bool noTranslate) {
  // init parameter float arrays
  glm_vec3_copy(dir, vec);
  glm_vec3_copy(box->max, sweep_max);
  glm_vec3_copy(box->base, base);

  // if (equals(epsilon, 0))
  epsilon = 1e-6f;

  // run sweep implementation
  // float dist = sweep_impl(getVoxel, _chunk, _callback, vec, base, sweep_max);
  float dist = sweep_impl(_getVoxel, _chunk, _callback);

  // translate box by distance needed to updated base value
  if (!noTranslate) {
    vec3 result;
    int i = 0;
    result[i] = (dir[i] > 0) ? sweep_max[i] - box->max[i] : base[i] - box->base[i];
    i++;
    result[i] = (dir[i] > 0) ? sweep_max[i] - box->max[i] : base[i] - box->base[i];
    i++;
    result[i] = (dir[i] > 0) ? sweep_max[i] - box->max[i] : base[i] - box->base[i];
    glm_vec3_add(box->max, result, box->max);
    glm_vec3_add(box->base, result, box->base);
  }

  // return value is total distance moved (not necessarily magnitude of [end]-[start])
  return dist;
}
