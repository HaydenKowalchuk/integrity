#pragma once

#include <cooker/shared_defs.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  GraphicsHeader header;
  MeshBatch* batches;
  uint8_t* texture_blob;
  Vertex32* vertices;
  size_t file_size;
} GltfBundle;

GltfBundle* gltf_load(const char* filename);
void gltf_free(GltfBundle* bundle);
