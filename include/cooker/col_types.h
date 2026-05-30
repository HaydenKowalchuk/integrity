#pragma once

#include <stdint.h>

#define COLLISION_MAGIC 0x434F4C21

typedef struct {
  uint32_t magic;
  uint32_t grid_x;
  uint32_t grid_y;
  uint32_t grid_z;
  float cell_size_x;
  float cell_size_y;
  float cell_size_z;
  float origin_x;
  float origin_y;
  float origin_z;
  int32_t origin_cx;
  int32_t origin_cy;
  int32_t origin_cz;
  uint32_t num_triangles;
  uint32_t num_active_cells;
  uint32_t hash_capacity;
  uint32_t num_triggers;
} ColHeader;
