#pragma once

#include <integrity/common/common.h>
#include <integrity/common/renderer_types.h>

// #ifdef _arch_dreamcast
//   glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &_particle_verts[0].vert);
//   glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &_particle_verts[0].texture);
//   glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &_particle_verts[0].color);
// #else
//   // glInterleavedArrays(GL_T2F_C4UB_V3F, 0, &_particle_verts[0]);
//   glVertexPointer(3, GL_FLOAT, sizeof(psp_fast_t), &_particle_verts[0].vert);
//   glTexCoordPointer(2, GL_FLOAT, sizeof(psp_fast_t), &_particle_verts[0].texture);
//   glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(psp_fast_t), &_particle_verts[0].color);
// #endif
//
typedef struct psp_fast_t psp_fast_t;

typedef struct model_obj {
#if defined(_arch_dreamcast) || defined(DESKTOP)
  glvert_fast_t *tris;
#else
  psp_fast_t *tris;
#endif
  // float *tris; /* pos 3f, uv 2f, color 3f */
  float min[3];
  float max[3];
  int32_t num_tris;
  int32_t num_faces;
  uint32_t texture;
  uint32_t crc;
  int32_t is_ptr;
} model_obj;

model_obj *OBJ_load(const char *path);
model_obj *OBJ_load_boolean(const char *path, bool transform);
void OBJ_destroy(model_obj **obj);
void OBJ_bind(model_obj *obj);
