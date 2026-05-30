#include <cooker/gltf_loader.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC_GRAPHICS 0x4D434421

GltfBundle* gltf_load(const char* filename) {
  FILE* fp = fopen(filename, "rb");
  if (!fp) {
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  size_t file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  GltfBundle* bundle = calloc(1, sizeof(GltfBundle));
  if (!bundle) {
    fclose(fp);
    return NULL;
  }

  if (fread(&bundle->header, sizeof(GraphicsHeader), 1, fp) != 1) {
    free(bundle);
    fclose(fp);
    return NULL;
  }

  if (bundle->header.magic != MAGIC_GRAPHICS) {
    free(bundle);
    fclose(fp);
    return NULL;
  }

  bundle->file_size = file_size;
  bundle->batches = calloc(bundle->header.num_batches, sizeof(MeshBatch));
  if (fread(bundle->batches, sizeof(MeshBatch), bundle->header.num_batches, fp) != bundle->header.num_batches) {
    free(bundle->batches);
    free(bundle);
    fclose(fp);
    return NULL;
  }

  if (bundle->header.texture_blob_size > 0) {
    bundle->texture_blob = malloc(bundle->header.texture_blob_size);
    fseek(fp, bundle->header.texture_blob_offset, SEEK_SET);
    fread(bundle->texture_blob, 1, bundle->header.texture_blob_size, fp);
  }

  size_t vertex_data_size = file_size - bundle->header.texture_blob_offset - bundle->header.texture_blob_size;
  bundle->vertices = malloc(vertex_data_size);
  fread(bundle->vertices, 1, vertex_data_size, fp);

  fclose(fp);
  return bundle;
}

void gltf_free(GltfBundle* bundle) {
  if (!bundle) {
    return;
  }
  free(bundle->batches);
  free(bundle->texture_blob);
  free(bundle->vertices);
  free(bundle);
}
