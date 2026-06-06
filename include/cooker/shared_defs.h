#pragma once

#include <integrity/common/renderer_types.h>
#include <stdint.h>

#define MAGIC_GRAPHICS 0x4D434421

typedef enum {
  PASS_OPAQUE = 0,
  PASS_PUNCH_THRU = 1,
  PASS_TRANSLUCENT = 2
} RenderPass;

typedef struct {
  uint32_t texture_crc;
  uint32_t vtx_offset;
  int32_t vtx_count;
  uint32_t tri_offset;
  uint32_t num_tris;
  uint8_t pass;
  uint8_t padding[3];
} MeshBatch;

typedef struct {
  uint32_t magic;
  uint32_t num_batches;
  uint32_t texture_blob_offset;
  uint32_t texture_blob_size;
} GraphicsHeader;

typedef struct {
  float min_x, min_y, min_z;
  float max_x, max_y, max_z;
  uint32_t target_crc;
} TriggerPortal;
