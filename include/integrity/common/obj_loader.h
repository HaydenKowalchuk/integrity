#pragma once

#include <integrity/common/common.h>
#include <integrity/common/renderer_types.h>

typedef struct model_obj {
  VtxFmt* tris;
  float min[3];
  float max[3];
  int32_t num_tris;
  int32_t num_faces;
  uint32_t texture;
  uint32_t crc;
  int32_t is_ptr;
} model_obj;

model_obj* OBJ_load(const char* path);
model_obj* OBJ_load_boolean(const char* path, bool transform);
void OBJ_destroy(model_obj** obj);
void OBJ_bind(model_obj* obj);
