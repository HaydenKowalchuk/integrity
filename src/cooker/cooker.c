#include "integrity/common/renderer_types.h"
#define CGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <cgltf.h>
#include <cooker/col_types.h>
#include <cooker/crclib.h>
#include <cooker/shared_defs.h>
#include <stb_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char name[64];
  uint32_t texture_crc;
  RenderPass pass;
  VtxFmt* vertices;
  size_t vertex_capacity;
  size_t vertex_count;
  uint32_t* indices;
  size_t index_capacity;
  size_t index_count;
  uint8_t* texture_data;
  size_t texture_size;
  int tex_width;
  int tex_height;
} CookerBucket;

typedef struct {
  float x, y, z;
} Vec3;

typedef struct {
  Vec3 verts[3];
} CollisionTri;

typedef struct {
  int32_t cell_index;
  uint32_t key;
  bool used;
} GridEntry;

typedef struct {
  uint16_t indices[256];
  uint8_t count;
} SpCell;

static Vec3 g_col_verts[10000];
static int g_col_vert_count = 0;

enum {
  VERBOSE_QUIET = 0,
  VERBOSE_BASIC = 1,
  VERBOSE_DEBUG = 2,
  VERBOSE_TRACE = 3,
};

static int g_verbose_level = VERBOSE_BASIC;
static int g_col_node_count;

typedef struct {
  const char* input_file;
  const char* out_bin;
  const char* out_col;
  float grid_res[3];
} CookerArgs;

static CookerArgs parse_arguments(int argc, char** argv) {
  CookerArgs args = {0};
  args.grid_res[0] = 16.0f;
  args.grid_res[1] = 8.0f;
  args.grid_res[2] = 16.0f;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0) {
      g_verbose_level = VERBOSE_DEBUG;
    } else if (strcmp(argv[i], "-vv") == 0) {
      g_verbose_level = VERBOSE_DEBUG;
    } else if (strcmp(argv[i], "-vvv") == 0) {
      g_verbose_level = VERBOSE_TRACE;
    } else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
      args.input_file = argv[++i];
    } else if (strcmp(argv[i], "--out_bin") == 0 && i + 1 < argc) {
      args.out_bin = argv[++i];
    } else if (strcmp(argv[i], "--out_col") == 0 && i + 1 < argc) {
      args.out_col = argv[++i];
    } else if (strcmp(argv[i], "--grid_res") == 0 && i + 3 < argc) {
      args.grid_res[0] = atof(argv[++i]);
      args.grid_res[1] = atof(argv[++i]);
      args.grid_res[2] = atof(argv[++i]);
    }
  }

  if (!args.input_file || !args.out_bin) {
    fprintf(stderr, "Usage: %s --input <file.gltf> --out_bin <file.bin> [--out_col <file.col>] [--grid_res X Y Z] [-v|-vv|-vvv]\n", argv[0]);
    args.input_file = NULL;
  }
  return args;
}

#define COOKER_ERROR(fmt, ...) fprintf(stderr, "[cooker] ERROR: " fmt "\n", ##__VA_ARGS__)
#define COOKER_WARN(fmt, ...) fprintf(stderr, "[cooker] WARNING: " fmt "\n", ##__VA_ARGS__)
#define COOKER_LOG(fmt, ...) \
  if (g_verbose_level >= VERBOSE_BASIC) fprintf(stdout, "[cooker]: " fmt "\n", ##__VA_ARGS__)
#define COOKER_DEBUG(fmt, ...) \
  if (g_verbose_level >= VERBOSE_DEBUG) fprintf(stdout, "[cooker] DEBUG: " fmt "\n", ##__VA_ARGS__)
#define COOKER_TRACE(fmt, ...) \
  if (g_verbose_level >= VERBOSE_TRACE) fprintf(stdout, "[cooker] TRACE: " fmt "\n", ##__VA_ARGS__)

static cgltf_data* load_gltf_file(const char* input_file) {
  COOKER_LOG("Loading glTF file: %s", input_file);
  cgltf_options opts = {0};
  cgltf_data* data = NULL;
  cgltf_result res = cgltf_parse_file(&opts, input_file, &data);
  if (res != cgltf_result_success) {
    COOKER_ERROR("Failed to parse glTF: %d", res);
    return NULL;
  }

  COOKER_DEBUG("glTF parsed: %zu scenes, %zu meshes, %zu materials, %zu nodes, %zu accessors, %zu buffers, %zu buffer_views",
               data->scenes_count,
               data->meshes_count,
               data->materials_count,
               data->nodes_count,
               data->accessors_count,
               data->buffers_count,
               data->buffer_views_count);

  for (cgltf_size i = 0; i < data->meshes_count; i++) {
    cgltf_mesh* mesh = &data->meshes[i];
    COOKER_DEBUG("Mesh %zu: %s, %zu primitives", i, mesh->name ? mesh->name : "(unnamed)", mesh->primitives_count);
    for (cgltf_size p = 0; p < mesh->primitives_count; p++) {
      cgltf_primitive* prim = &mesh->primitives[p];
      COOKER_DEBUG("  Primitive %zu: indices=%zu, attributes=%zu, mode=%d",
                   p,
                   prim->indices ? prim->indices->count : 0,
                   prim->attributes_count,
                   prim->type);
      for (cgltf_size a = 0; a < prim->attributes_count; a++) {
        cgltf_attribute* attr = &prim->attributes[a];
        COOKER_DEBUG("    Attr %zu: %s, type=%d, idx=%d", a, attr->name ? attr->name : "?", attr->type, attr->index);
      }
    }
  }

  for (cgltf_size i = 0; i < data->materials_count; i++) {
    cgltf_material* mat = &data->materials[i];
    COOKER_DEBUG("Material %zu: %s", i, mat->name ? mat->name : "(unnamed)");
    if (mat->has_pbr_metallic_roughness) {
      COOKER_DEBUG("  PBR: metallic=%f, roughness=%f", mat->pbr_metallic_roughness.metallic_factor, mat->pbr_metallic_roughness.roughness_factor);
      if (mat->pbr_metallic_roughness.base_color_texture.texture) {
        COOKER_DEBUG("  BaseColorTex: %s", mat->pbr_metallic_roughness.base_color_texture.texture->name ? mat->pbr_metallic_roughness.base_color_texture.texture->name : "(unnamed)");
      }
    }
  }

  for (cgltf_size i = 0; i < data->accessors_count; i++) {
    cgltf_accessor* acc = &data->accessors[i];
    COOKER_TRACE("Accessor %zu: type=%d, count=%zu, comp_type=%d", i, acc->type, acc->count, acc->component_type);
  }

  for (cgltf_size i = 0; i < data->buffer_views_count; i++) {
    cgltf_buffer_view* bv = &data->buffer_views[i];
    COOKER_TRACE("BufferView %zu: offset=%zu, size=%zu, stride=%lu", i, bv->offset, bv->size, bv->stride);
  }

  res = cgltf_load_buffers(&opts, data, input_file);
  if (res != cgltf_result_success) {
    COOKER_WARN("Failed to load buffers: %d", res);
    cgltf_free(data);
    return NULL;
  }

  COOKER_LOG("Loaded glTF: %zu nodes, %zu meshes, %zu materials", data->nodes_count, data->meshes_count, data->materials_count);
  return data;
}

typedef struct {
  cgltf_node* node;
  char name[64];
  int is_static;
  int is_col;
  float world_mat[16];
} NodeInfo;

typedef struct {
  char base_name[64];
  float world_mat[16];
  cgltf_mesh* mesh;
} ColNode;

static void add_col_node(const char* node_name, const float world_mat[16], cgltf_mesh* mesh);
static void get_node_world_matrix(const cgltf_node* node, float out[16]);
static int find_col_node(const char* base_name, ColNode* out_node);
static void process_primitive(CookerBucket* bucket, const cgltf_primitive* prim, const float world_mat[16]);
static void mat4_mult_vec3(const float m[16], Vec3* in, Vec3* out);
static uint8_t* convert_texture(uint8_t* rgba, int w, int h, RenderPass pass, size_t* out_size);
static RenderPass get_pass_from_material(const char* mat_name);

static void discover_nodes(cgltf_data* data, NodeInfo** out_nodes, size_t* out_count) {
  COOKER_LOG("Scanning %zu nodes for _static and _col meshes", data->nodes_count);

  for (cgltf_size n = 0; n < data->nodes_count; n++) {
    cgltf_node* node = &data->nodes[n];
    const char* node_name = node->name ? node->name : "";
    COOKER_TRACE("Node %zu: %s, has_mesh=%d", n, node_name, node->mesh != NULL);
  }

  NodeInfo* nodes = NULL;
  size_t count = 0;
  size_t capacity = 0;

  for (cgltf_size n = 0; n < data->nodes_count; n++) {
    cgltf_node* node = &data->nodes[n];
    if (!node->mesh) {
      continue;
    }

    const char* node_name = node->name ? node->name : "";
    int is_static = strstr(node_name, "_static") != NULL;
    int is_col = strstr(node_name, "_col") != NULL;

    if (!is_static && !is_col) {
      continue;
    }

    COOKER_DEBUG("Discovered: %s (static=%d, col=%d)", node_name, is_static, is_col);
    if (node->mesh) {
      COOKER_TRACE("  Mesh: %s, %zu primitives", node->mesh->name ? node->mesh->name : "(unnamed)", node->mesh->primitives_count);
    }

    if (count >= capacity) {
      capacity = capacity ? capacity * 2 : 4;
      nodes = realloc(nodes, capacity * sizeof(NodeInfo));
    }

    memset(&nodes[count], 0, sizeof(NodeInfo));
    strncpy(nodes[count].name, node_name, 63);
    nodes[count].is_static = is_static;
    nodes[count].is_col = is_col;
    nodes[count].node = node;
    get_node_world_matrix(node, nodes[count].world_mat);
    COOKER_TRACE("  World transform: [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f] [%.2f,%.2f,%.2f,%.2f]",
                 nodes[count].world_mat[0],
                 nodes[count].world_mat[1],
                 nodes[count].world_mat[2],
                 nodes[count].world_mat[3],
                 nodes[count].world_mat[4],
                 nodes[count].world_mat[5],
                 nodes[count].world_mat[6],
                 nodes[count].world_mat[7],
                 nodes[count].world_mat[8],
                 nodes[count].world_mat[9],
                 nodes[count].world_mat[10],
                 nodes[count].world_mat[11],
                 nodes[count].world_mat[12],
                 nodes[count].world_mat[13],
                 nodes[count].world_mat[14],
                 nodes[count].world_mat[15]);

    if (is_col) {
      add_col_node(node_name, nodes[count].world_mat, node->mesh);
    }

    count++;
  }

  COOKER_LOG("Found %zu static nodes, %d collision nodes", count, g_col_node_count);
  *out_nodes = nodes;
  *out_count = count;
}

static CookerBucket* process_graphics(NodeInfo* nodes, size_t node_count, const char* input_file, size_t* out_bucket_count) {
  COOKER_LOG("Processing %zu graphics nodes", node_count);
  CookerBucket* buckets = NULL;
  size_t bucket_count = 0;
  size_t bucket_capacity = 0;

  for (size_t ni = 0; ni < node_count; ni++) {
    NodeInfo* info = &nodes[ni];
    if (info->is_col) {
      COOKER_TRACE("Skipping collision node: %s", info->name);
      continue;
    }

    cgltf_node* node = info->node;
    const char* node_name = info->name;
    COOKER_DEBUG("Processing graphics node: %s, primitives=%zu", node_name, node->mesh->primitives_count);

    for (cgltf_size p = 0; p < node->mesh->primitives_count; p++) {
      cgltf_primitive* prim = &node->mesh->primitives[p];
      const char* mat_name = prim->material && prim->material->name ? prim->material->name : NULL;
      COOKER_TRACE("  Primitive %zu: material=%s, indices=%zu, attributes=%zu",
                   p,
                   mat_name ? mat_name : "(none)",
                   prim->indices ? prim->indices->count : 0,
                   prim->attributes_count);

      int has_valid_material = mat_name && (strstr(mat_name, "_op") || strstr(mat_name, "_pt") || strstr(mat_name, "_tr"));
      if (!has_valid_material) {
        COOKER_TRACE("  Skipping render: no valid material (need _op, _pt, or _tr suffix)");
      }

      RenderPass pass = has_valid_material ? get_pass_from_material(mat_name) : PASS_OPAQUE;
      uint32_t tex_crc = 0;

      if (has_valid_material && prim->material && prim->material->has_pbr_metallic_roughness) {
        cgltf_texture* tex = prim->material->pbr_metallic_roughness.base_color_texture.texture;
        if (tex && tex->image && tex->image->uri) {
          char tex_path[512];
          strcpy(tex_path, input_file);
          char* last_slash = strrchr(tex_path, '/');
          if (last_slash) {
            strcpy(last_slash + 1, tex->image->uri);
            COOKER_TRACE("    Loading texture: %s", tex->image->uri);
            int w, h, ch;
            uint8_t* rgba = stbi_load(tex_path, &w, &h, &ch, 4);
            if (rgba) {
              size_t tex_size;
              uint8_t* tex_data = convert_texture(rgba, w, h, pass, &tex_size);
              stbi_image_free(rgba);

              tex_crc = crc32_data(tex_data, tex_size);
              COOKER_TRACE("    Texture: %dx%d, CRC=0x%08x, size=%zu", w, h, tex_crc, tex_size);

              int found = -1;
              for (size_t b = 0; b < bucket_count; b++) {
                if (buckets[b].texture_crc == tex_crc && buckets[b].pass == pass) {
                  found = b;
                  break;
                }
              }

              if (found < 0) {
                if (bucket_count >= bucket_capacity) {
                  bucket_capacity = bucket_capacity ? bucket_capacity * 2 : 4;
                  buckets = realloc(buckets, bucket_capacity * sizeof(CookerBucket));
                }
                found = bucket_count++;
                memset(&buckets[found], 0, sizeof(CookerBucket));
                strncpy(buckets[found].name, mat_name, 63);
                buckets[found].pass = pass;
                buckets[found].texture_crc = tex_crc;
                buckets[found].texture_data = tex_data;
                buckets[found].texture_size = tex_size;
                buckets[found].tex_width = w;
                buckets[found].tex_height = h;
                COOKER_TRACE("    Created new bucket %d: %s, pass=%d", found, mat_name, pass);
              } else {
                COOKER_TRACE("    Reusing bucket %d", found);
                free(tex_data);
              }
            } else {
              COOKER_WARN("Failed to load texture: %s", tex_path);
            }
          }
        } else {
          COOKER_TRACE("    No base color texture");
        }
      }

      if (has_valid_material) {
        int bucket_idx = -1;
        for (size_t b = 0; b < bucket_count; b++) {
          if (buckets[b].texture_crc == tex_crc && buckets[b].pass == pass) {
            bucket_idx = b;
            break;
          }
        }
        if (bucket_idx < 0) {
          if (bucket_count >= bucket_capacity) {
            bucket_capacity = bucket_capacity ? bucket_capacity * 2 : 4;
            buckets = realloc(buckets, bucket_capacity * sizeof(CookerBucket));
          }
          bucket_idx = bucket_count++;
          memset(&buckets[bucket_idx], 0, sizeof(CookerBucket));
          strncpy(buckets[bucket_idx].name, mat_name, 63);
          buckets[bucket_idx].pass = pass;
          buckets[bucket_idx].texture_crc = tex_crc;
        }

        process_primitive(&buckets[bucket_idx], prim, info->world_mat);
      }

      if (info->is_static) {
        const char* suffix = strstr(node_name, "_static");
        size_t base_len = suffix ? (size_t)(suffix - node_name) : strlen(node_name);
        char base_name[64] = {0};
        if (base_len < sizeof(base_name)) {
          strncpy(base_name, node_name, base_len);
        }
        int has_matching_col = find_col_node(base_name, NULL);
        if (has_matching_col) {
          COOKER_DEBUG("Skipping fallback: dedicated collision mesh exists for: %s", base_name);
        }

        if (!has_matching_col) {
          cgltf_accessor* pos_acc = NULL;
          for (cgltf_size a = 0; a < prim->attributes_count; a++) {
            if (prim->attributes[a].type == cgltf_attribute_type_position) {
              pos_acc = prim->attributes[a].data;
            }
          }
          cgltf_accessor* idx_acc = prim->indices;
          if (pos_acc) {
            cgltf_size vert_count = pos_acc->count;
            cgltf_size idx_count = idx_acc ? idx_acc->count : vert_count;
            Vec3 positions[3];
            for (cgltf_size i = 0; i < idx_count; i++) {
              cgltf_size idx = idx_acc ? (cgltf_size)cgltf_accessor_read_index(idx_acc, i) : i;
              cgltf_accessor_read_float(pos_acc, idx, &positions[i % 3].x, 3);
              if ((i + 1) % 3 == 0 && g_col_vert_count + 3 < 10000) {
                Vec3 wp[3];
                for (int j = 0; j < 3; j++) {
                  mat4_mult_vec3(info->world_mat, &positions[j], &wp[j]);
                }
                float e1 = fabsf(wp[1].x - wp[0].x) + fabsf(wp[1].y - wp[0].y) + fabsf(wp[1].z - wp[0].z);
                float e2 = fabsf(wp[2].x - wp[0].x) + fabsf(wp[2].y - wp[0].y) + fabsf(wp[2].z - wp[0].z);
                float e3 = fabsf(wp[2].x - wp[1].x) + fabsf(wp[2].y - wp[1].y) + fabsf(wp[2].z - wp[1].z);
                if (e1 < 0.01f || e2 < 0.01f || e3 < 0.01f) {
                  continue;
                }
                for (int j = 0; j < 3; j++) {
                  g_col_verts[g_col_vert_count + j] = wp[j];
                }
                g_col_vert_count += 3;
              }
            }
          }
        }
      }
    }
  }

  *out_bucket_count = bucket_count;
  return buckets;
}

static int write_graphics_file(const char* out_bin, CookerBucket* buckets, size_t bucket_count) {
  FILE* fp = fopen(out_bin, "wb");
  if (!fp) {
    fprintf(stderr, "Failed to open output: %s", out_bin);
    return 1;
  }

  GraphicsHeader hdr = {0};
  hdr.magic = MAGIC_GRAPHICS;
  hdr.num_batches = (uint32_t)bucket_count;
  hdr.texture_blob_offset = sizeof(GraphicsHeader) + bucket_count * sizeof(MeshBatch);
  size_t tex_blob_size = 0;
  for (size_t i = 0; i < bucket_count; i++) {
    tex_blob_size += buckets[i].texture_size;
  }
  hdr.texture_blob_size = (uint32_t)tex_blob_size;

  fwrite(&hdr, sizeof(hdr), 1, fp);

  uint32_t vtx_offset = hdr.texture_blob_offset + tex_blob_size;
  uint32_t tri_offset = vtx_offset;
  for (size_t i = 0; i < bucket_count; i++) {
    tri_offset += (uint32_t)(buckets[i].vertex_count * sizeof(VtxFmt));
  }

  for (size_t i = 0; i < bucket_count; i++) {
    MeshBatch batch = {0};
    batch.texture_crc = buckets[i].texture_crc;
    batch.vtx_offset = vtx_offset;
    batch.vtx_count = (uint32_t)buckets[i].vertex_count;
    batch.tri_offset = buckets[i].index_count > 0 ? tri_offset : 0;
    batch.num_tris = (uint32_t)(buckets[i].index_count > 0 ? buckets[i].index_count : buckets[i].vertex_count) / 3;
    batch.pass = buckets[i].pass;
    fwrite(&batch, sizeof(batch), 1, fp);
    vtx_offset += (uint32_t)(buckets[i].vertex_count * sizeof(VtxFmt));
    if (buckets[i].index_count > 0) {
      tri_offset += (uint32_t)(buckets[i].index_count * sizeof(uint32_t));
    }
  }

  for (size_t i = 0; i < bucket_count; i++) {
    fwrite(buckets[i].texture_data, 1, buckets[i].texture_size, fp), free(buckets[i].texture_data);
  }

  for (size_t i = 0; i < bucket_count; i++) {
    fwrite(buckets[i].vertices, sizeof(VtxFmt), buckets[i].vertex_count, fp), free(buckets[i].vertices);
  }

  for (size_t i = 0; i < bucket_count; i++) {
    if (buckets[i].index_count > 0) {
      fwrite(buckets[i].indices, sizeof(uint32_t), buckets[i].index_count, fp);
      free(buckets[i].indices);
    }
  }

  fclose(fp);
  free(buckets);

  COOKER_LOG("Cooked: %zu batches, output: %s", bucket_count, out_bin);
  return 0;
}

static uint32_t grid_hash(int x, int y, int z);

static int process_collision(const char* out_col) {
  COOKER_LOG("Processing collision: %d vertices (%d triangles)", g_col_vert_count, g_col_vert_count / 3);

  for (int i = 0; i < g_col_vert_count && i < 30; i++) {
    COOKER_TRACE("Collision vert %d: (%.3f, %.3f, %.3f)", i, g_col_verts[i].x, g_col_verts[i].y, g_col_verts[i].z);
  }
  if (g_col_vert_count > 30) {
    COOKER_TRACE("... and %d more vertices", g_col_vert_count - 30);
  }
  if (!out_col || g_col_vert_count == 0) {
    if (out_col) {
      FILE* fp = fopen(out_col, "wb");
      if (fp) {
        ColHeader col_hdr = {0};
        col_hdr.magic = COLLISION_MAGIC;
        fwrite(&col_hdr, sizeof(col_hdr), 1, fp);
        fclose(fp);
        COOKER_LOG("Collision: (empty)");
      }
    }
    return 0;
  }

  int num_tris = g_col_vert_count / 3;
  float min_x = 1e9, min_y = 1e9, min_z = 1e9;
  float max_x = -1e9, max_y = -1e9, max_z = -1e9;
  for (int i = 0; i < g_col_vert_count; i++) {
    if (g_col_verts[i].x < min_x) {
      min_x = g_col_verts[i].x;
    }
    if (g_col_verts[i].y < min_y) {
      min_y = g_col_verts[i].y;
    }
    if (g_col_verts[i].z < min_z) {
      min_z = g_col_verts[i].z;
    }
    if (g_col_verts[i].x > max_x) {
      max_x = g_col_verts[i].x;
    }
    if (g_col_verts[i].y > max_y) {
      max_y = g_col_verts[i].y;
    }
    if (g_col_verts[i].z > max_z) {
      max_z = g_col_verts[i].z;
    }
  }
  float buffer = 5.0f;
  min_x -= buffer;
  min_y -= buffer;
  min_z -= buffer;
  max_x += buffer;
  max_y += buffer;
  max_z += buffer;
  float bounds_x = max_x - min_x;
  float bounds_y = max_y - min_y;
  float bounds_z = max_z - min_z;
  const int subdiv_x = 16;
  const int subdiv_y = 4;
  const int subdiv_z = 16;
  float cell_size_x = bounds_x / subdiv_x;
  float cell_size_y = bounds_y / subdiv_y;
  float cell_size_z = bounds_z / subdiv_z;
  if (cell_size_x < 1.0f) {
    cell_size_x = 1.0f;
  }
  if (cell_size_y < 1.0f) {
    cell_size_y = 1.0f;
  }
  if (cell_size_z < 1.0f) {
    cell_size_z = 1.0f;
  }
  COOKER_DEBUG("Bounds: (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)", min_x, min_y, min_z, max_x, max_y, max_z);
  COOKER_DEBUG("Cell sizes: x=%.2f, y=%.2f, z=%.2f (subdiv %dx%dx%d)", cell_size_x, cell_size_y, cell_size_z, subdiv_x, subdiv_y, subdiv_z);
  int global_cx_min = 0, global_cy_min = 0, global_cz_min = 0;
  int global_cx_max = 0, global_cy_max = 0, global_cz_max = 0;
  for (int t = 0; t < num_tris; t++) {
    float t_min_x = fminf(fminf(g_col_verts[t * 3 + 0].x, g_col_verts[t * 3 + 1].x), g_col_verts[t * 3 + 2].x);
    float t_min_y = fminf(fminf(g_col_verts[t * 3 + 0].y, g_col_verts[t * 3 + 1].y), g_col_verts[t * 3 + 2].y);
    float t_min_z = fminf(fminf(g_col_verts[t * 3 + 0].z, g_col_verts[t * 3 + 1].z), g_col_verts[t * 3 + 2].z);
    float t_max_x = fmaxf(fmaxf(g_col_verts[t * 3 + 0].x, g_col_verts[t * 3 + 1].x), g_col_verts[t * 3 + 2].x);
    float t_max_y = fmaxf(fmaxf(g_col_verts[t * 3 + 0].y, g_col_verts[t * 3 + 1].y), g_col_verts[t * 3 + 2].y);
    float t_max_z = fmaxf(fmaxf(g_col_verts[t * 3 + 0].z, g_col_verts[t * 3 + 1].z), g_col_verts[t * 3 + 2].z);
    int cx_min = (int)floorf(t_min_x / cell_size_x);
    int cy_min = (int)floorf(t_min_y / cell_size_y);
    int cz_min = (int)floorf(t_min_z / cell_size_z);
    int cx_max = (int)floorf(t_max_x / cell_size_x);
    int cy_max = (int)floorf(t_max_y / cell_size_y);
    int cz_max = (int)floorf(t_max_z / cell_size_z);
    if (t == 0) {
      global_cx_min = cx_min;
      global_cy_min = cy_min;
      global_cz_min = cz_min;
      global_cx_max = cx_max;
      global_cy_max = cy_max;
      global_cz_max = cz_max;
    } else {
      if (cx_min < global_cx_min) {
        global_cx_min = cx_min;
      }
      if (cy_min < global_cy_min) {
        global_cy_min = cy_min;
      }
      if (cz_min < global_cz_min) {
        global_cz_min = cz_min;
      }
      if (cx_max > global_cx_max) {
        global_cx_max = cx_max;
      }
      if (cy_max > global_cy_max) {
        global_cy_max = cy_max;
      }
      if (cz_max > global_cz_max) {
        global_cz_max = cz_max;
      }
    }
  }
  int grid_x = global_cx_max - global_cx_min + 1;
  int grid_y = global_cy_max - global_cy_min + 1;
  int grid_z = global_cz_max - global_cz_min + 1;
  int num_dense_cells = grid_x * grid_y * grid_z;

  COOKER_DEBUG("Collision bounds: (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)", min_x, min_y, min_z, max_x, max_y, max_z);
  COOKER_DEBUG("Grid: %dx%dx%d cells, origin cell (%d, %d, %d)", grid_x, grid_y, grid_z, global_cx_min, global_cy_min, global_cz_min);

  FILE* fp = fopen(out_col, "wb");
  if (!fp) {
    return 1;
  }

  SpCell* dense_cells = calloc(num_dense_cells, sizeof(SpCell));
  int* cell_to_idx = malloc(num_dense_cells * sizeof(int));
  for (int i = 0; i < num_dense_cells; i++) {
    cell_to_idx[i] = -1;
  }

  for (int t = 0; t < num_tris; t++) {
    float tri_min_x = fminf(fminf(g_col_verts[t * 3 + 0].x, g_col_verts[t * 3 + 1].x), g_col_verts[t * 3 + 2].x);
    float tri_min_y = fminf(fminf(g_col_verts[t * 3 + 0].y, g_col_verts[t * 3 + 1].y), g_col_verts[t * 3 + 2].y);
    float tri_min_z = fminf(fminf(g_col_verts[t * 3 + 0].z, g_col_verts[t * 3 + 1].z), g_col_verts[t * 3 + 2].z);
    float tri_max_x = fmaxf(fmaxf(g_col_verts[t * 3 + 0].x, g_col_verts[t * 3 + 1].x), g_col_verts[t * 3 + 2].x);
    float tri_max_y = fmaxf(fmaxf(g_col_verts[t * 3 + 0].y, g_col_verts[t * 3 + 1].y), g_col_verts[t * 3 + 2].y);
    float tri_max_z = fmaxf(fmaxf(g_col_verts[t * 3 + 0].z, g_col_verts[t * 3 + 1].z), g_col_verts[t * 3 + 2].z);
    int cx_min = (int)floorf(tri_min_x / cell_size_x);
    int cy_min = (int)floorf(tri_min_y / cell_size_y);
    int cz_min = (int)floorf(tri_min_z / cell_size_z);
    int cx_max = (int)floorf(tri_max_x / cell_size_x);
    int cy_max = (int)floorf(tri_max_y / cell_size_y);
    int cz_max = (int)floorf(tri_max_z / cell_size_z);
    for (int cx = cx_min; cx <= cx_max; cx++) {
      for (int cy = cy_min; cy <= cy_max; cy++) {
        for (int cz = cz_min; cz <= cz_max; cz++) {
          int dense_idx = (cx - global_cx_min) + (cy - global_cy_min) * grid_x + (cz - global_cz_min) * grid_x * grid_y;
          if (dense_idx < 0 || dense_idx >= num_dense_cells) {
            continue;
          }
          if (cell_to_idx[dense_idx] < 0) {
            cell_to_idx[dense_idx] = -2;
          }
          if (dense_cells[dense_idx].count < 255) {
            dense_cells[dense_idx].indices[dense_cells[dense_idx].count++] = (uint16_t)t;
          }
        }
      }
    }
  }

  int num_active_cells = 0;
  for (int i = 0; i < num_dense_cells; i++) {
    if (cell_to_idx[i] == -2) {
      cell_to_idx[i] = num_active_cells++;
    }
  }

  int hash_capacity = 1;
  while (hash_capacity < num_active_cells * 2) {
    hash_capacity <<= 1;
  }
  if (hash_capacity < 256) {
    hash_capacity = 256;
  }

  SpCell* active_cells = malloc(num_active_cells * sizeof(SpCell));
  GridEntry* entries = calloc(hash_capacity, sizeof(GridEntry));

  for (int i = 0; i < num_dense_cells; i++) {
    if (cell_to_idx[i] >= 0) {
      int cx = global_cx_min + (i % grid_x);
      int cy = global_cy_min + ((i / grid_x) % grid_y);
      int cz = global_cz_min + (i / (grid_x * grid_y));
      uint32_t key = grid_hash(cx, cy, cz);
      uint32_t idx = key & (hash_capacity - 1);
      while (entries[idx].used) {
        idx = (idx + 1) & (hash_capacity - 1);
      }
      entries[idx].used = 1;
      entries[idx].key = key;
      entries[idx].cell_index = cell_to_idx[i];
      active_cells[cell_to_idx[i]] = dense_cells[i];
    }
  }

  COOKER_LOG("Collision: %d triangles, %d active cells, hash capacity %d, grid %dx%dx%d origin=(%d,%d,%d)", num_tris, num_active_cells, hash_capacity, grid_x, grid_y, grid_z, global_cx_min, global_cy_min, global_cz_min);

  ColHeader col_hdr = {0};
  col_hdr.magic = COLLISION_MAGIC;
  col_hdr.grid_x = grid_x;
  col_hdr.grid_y = grid_y;
  col_hdr.grid_z = grid_z;
  col_hdr.cell_size_x = cell_size_x;
  col_hdr.cell_size_y = cell_size_y;
  col_hdr.cell_size_z = cell_size_z;
  col_hdr.origin_x = min_x;
  col_hdr.origin_y = min_y;
  col_hdr.origin_z = min_z;
  col_hdr.origin_cx = global_cx_min;
  col_hdr.origin_cy = global_cy_min;
  col_hdr.origin_cz = global_cz_min;
  col_hdr.num_triangles = num_tris;
  col_hdr.num_active_cells = num_active_cells;
  col_hdr.hash_capacity = hash_capacity;
  col_hdr.num_triggers = 0;
  fwrite(&col_hdr, sizeof(col_hdr), 1, fp);
  fwrite(g_col_verts, sizeof(Vec3), g_col_vert_count, fp);
  fwrite(entries, sizeof(GridEntry), hash_capacity, fp);
  fwrite(active_cells, sizeof(SpCell), num_active_cells, fp);

  free(dense_cells);
  free(cell_to_idx);
  free(entries);
  free(active_cells);
  fclose(fp);
  return 0;
}

static uint32_t grid_hash(int x, int y, int z) {
  uint32_t ix = (uint32_t)(x + 0x40000000);
  uint32_t iy = (uint32_t)(y + 0x40000000);
  uint32_t iz = (uint32_t)(z + 0x40000000);
  uint32_t h = ix;
  h ^= iy + 0x9e3779b9 + (h << 6) + (h >> 2);
  h ^= iz + 0x9e3779b9 + (h << 6) + (h >> 2);
  return h;
}

static ColNode g_col_nodes[100];
static int g_col_node_count = 0;

static void add_col_node(const char* node_name, const float world_mat[16], cgltf_mesh* mesh) {
  if (g_col_node_count >= 100) {
    return;
  }
  const char* suffix = strstr(node_name, "_col");
  if (!suffix) {
    return;
  }
  size_t len = suffix - node_name;
  if (len > 63) {
    len = 63;
  }
  strncpy(g_col_nodes[g_col_node_count].base_name, node_name, len);
  g_col_nodes[g_col_node_count].base_name[len] = '\0';
  memcpy(g_col_nodes[g_col_node_count].world_mat, world_mat, 16 * sizeof(float));
  g_col_nodes[g_col_node_count].mesh = mesh;
  g_col_node_count++;
}

static int find_col_node(const char* base_name, ColNode* out_node) {
  for (int i = 0; i < g_col_node_count; i++) {
    if (strcmp(g_col_nodes[i].base_name, base_name) == 0) {
      if (out_node) {
        *out_node = g_col_nodes[i];
      }
      return 1;
    }
  }
  return 0;
}

static void mat4_mult_vec3(const float m[16], Vec3* in, Vec3* out) {
  out->x = m[0] * in->x + m[4] * in->y + m[8] * in->z + m[12];
  out->y = m[1] * in->x + m[5] * in->y + m[9] * in->z + m[13];
  out->z = m[2] * in->x + m[6] * in->y + m[10] * in->z + m[14];
}

static RenderPass get_pass_from_material(const char* mat_name) {
  if (strstr(mat_name, "_op")) {
    return PASS_OPAQUE;
  }
  if (strstr(mat_name, "_pt")) {
    return PASS_PUNCH_THRU;
  }
  if (strstr(mat_name, "_tr")) {
    return PASS_TRANSLUCENT;
  }
  return PASS_OPAQUE;
}

static uint8_t* convert_texture(uint8_t* rgba, int w, int h, RenderPass pass, size_t* out_size) {
  size_t size = w * h * 2;
  uint8_t* out = malloc(size);
  for (int i = 0; i < w * h; i++) {
    uint8_t r = rgba[i * 4 + 0];
    uint8_t g = rgba[i * 4 + 1];
    uint8_t b = rgba[i * 4 + 2];
    uint8_t a = rgba[i * 4 + 3];
    uint16_t pixel;
    if (pass == PASS_OPAQUE) {
      pixel = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    } else if (pass == PASS_PUNCH_THRU) {
      pixel = ((a & 0x80) << 8) | ((r & 0xF8) << 7) | ((g & 0xF8) << 2) | (b >> 3);
    } else {
      pixel = ((a & 0xF0) << 8) | ((r & 0xF0) << 4) | (g & 0xF0) | (b >> 4);
    }
    out[i * 2 + 0] = pixel & 0xFF;
    out[i * 2 + 1] = (pixel >> 8) & 0xFF;
  }
  *out_size = size;
  return out;
}

static void add_vertex_to_bucket(CookerBucket* bucket, Vec3 pos, Vec3 normal, float u, float v, uint32_t color) {
  if (bucket->vertex_capacity == 0) {
    bucket->vertex_capacity = 256;
    bucket->vertices = malloc(bucket->vertex_capacity * sizeof(VtxFmt));
  } else if (bucket->vertex_count >= bucket->vertex_capacity) {
    bucket->vertex_capacity *= 2;
    bucket->vertices = realloc(bucket->vertices, bucket->vertex_capacity * sizeof(VtxFmt));
  }
  VtxFmt* vtx = &bucket->vertices[bucket->vertex_count++];
#if defined(_arch_dreamcast) || defined(COOKER_TARGET_DREAMCAST)
  vtx->flags = 0;
#endif
  vtx->vert.x = pos.x;
  vtx->vert.y = pos.y;
  vtx->vert.z = pos.z;
  vtx->texture.u = u;
  vtx->texture.v = v;
  vtx->color.packed = color;
}

static void add_index_to_bucket(CookerBucket* bucket, uint32_t idx) {
  if (bucket->index_capacity == 0) {
    bucket->index_capacity = 256;
    bucket->indices = malloc(bucket->index_capacity * sizeof(uint32_t));
  } else if (bucket->index_count >= bucket->index_capacity) {
    bucket->index_capacity *= 2;
    bucket->indices = realloc(bucket->indices, bucket->index_capacity * sizeof(uint32_t));
  }
  bucket->indices[bucket->index_count++] = idx;
}

static void process_primitive(CookerBucket* bucket, const cgltf_primitive* prim, const float world_mat[16]) {
  COOKER_TRACE("process_primitive: bucket=%s, indices=%zu, vertices=%zu",
               bucket->name,
               prim->indices ? prim->indices->count : 0,
               prim->attributes_count);

  cgltf_accessor* pos_acc = NULL;
  cgltf_accessor* uv_acc = NULL;
  cgltf_accessor* color_acc = NULL;
  cgltf_accessor* idx_acc = prim->indices;

  for (cgltf_size i = 0; i < prim->attributes_count; i++) {
    cgltf_attribute* attr = &prim->attributes[i];
    if (attr->type == cgltf_attribute_type_position) {
      pos_acc = attr->data;
    } else if (attr->type == cgltf_attribute_type_texcoord) {
      uv_acc = attr->data;
    } else if (attr->type == cgltf_attribute_type_color) {
      color_acc = attr->data;
    }
  }

  if (!pos_acc) {
    COOKER_WARN("No position attribute in primitive");
    return;
  }

  cgltf_size vert_count = pos_acc->count;
  cgltf_size idx_count = idx_acc ? idx_acc->count : vert_count;

  COOKER_TRACE("  Processing %zu vertices, %zu indices", vert_count, idx_count);

  Vec3 positions[3];
  float uvs[3][2];
  uint32_t colors[3];

  cgltf_size tri_vert = 0;
  for (cgltf_size i = 0; i < idx_count; i++) {
    cgltf_size idx = idx_acc ? (cgltf_size)cgltf_accessor_read_index(idx_acc, i) : i;

    cgltf_accessor_read_float(pos_acc, idx, &positions[tri_vert % 3].x, 3);
    if (uv_acc) {
      cgltf_accessor_read_float(uv_acc, idx, uvs[tri_vert % 3], 2);
    } else {
      uvs[tri_vert % 3][0] = 0;
      uvs[tri_vert % 3][1] = 0;
    }

    if (color_acc) {
      float c[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // Default alpha to 1.0
      cgltf_accessor_read_float(color_acc, idx, c, cgltf_num_components(color_acc->type));

      // ensure we arent super color
      if (c[0] > 1.0f) {
        c[0] = 1.0f;
      }
      if (c[1] > 1.0f) {
        c[1] = 1.0f;
      }
      if (c[2] > 1.0f) {
        c[2] = 1.0f;
      }
      if (c[3] > 1.0f) {
        c[3] = 1.0f;
      }

      // Instead of: uint8_t r = (uint8_t)(c[0] * 255);
      // Use a Gamma 2.2 correction:
      uint8_t r = (uint8_t)(powf(c[0], 1.0f / 2.2f) * 255.0f);
      uint8_t g = (uint8_t)(powf(c[1], 1.0f / 2.2f) * 255.0f);
      uint8_t b = (uint8_t)(powf(c[2], 1.0f / 2.2f) * 255.0f);
      uint8_t a = (uint8_t)(c[3] * 255.0f);

      colors[tri_vert % 3] = PACK_COLOR(r, g, b, a);
    } else {
      colors[tri_vert % 3] = 0xFFFFFFFF;
    }

    if (idx_acc && prim->type != cgltf_primitive_type_triangles) {
      continue;
    }

    if (idx_acc) {
      add_index_to_bucket(bucket, (uint32_t)idx);
    }

    tri_vert++;

    if (tri_vert % 3 == 0) {
      Vec3 world_pos[3];
      for (int j = 0; j < 3; j++) {
        mat4_mult_vec3(world_mat, &positions[j], &world_pos[j]);
      }

      uint32_t flags = (tri_vert == idx_count) ? 0x40000000 : 0;
      for (int j = 0; j < 3; j++) {
        add_vertex_to_bucket(bucket, world_pos[j], (Vec3){0, 0, 0}, uvs[j][0], uvs[j][1], colors[j]);
#if defined(_arch_dreamcast) || defined(COOKER_TARGET_DREAMCAST)
        bucket->vertices[bucket->vertex_count - 1].flags = (j == 2) ? flags : 0;
#endif
      }
    }
  }
}

static void get_node_world_matrix(const cgltf_node* node, float out[16]) {
  if (node->has_matrix) {
    memcpy(out, node->matrix, 16 * sizeof(float));
  } else {
    float t[3] = {0, 0, 0}, r[4] = {0, 0, 0, 1}, s[3] = {1, 1, 1};
    if (node->has_translation) {
      memcpy(t, node->translation, 3 * sizeof(float));
    }
    if (node->has_rotation) {
      memcpy(r, node->rotation, 4 * sizeof(float));
    }
    if (node->has_scale) {
      memcpy(s, node->scale, 3 * sizeof(float));
    }

    float tx = t[0], ty = t[1], tz = t[2];
    float qx = r[0], qy = r[1], qz = r[2], qw = r[3];
    float x2 = qx + qx, y2 = qy + qy, z2 = qz + qz;
    float xy = qx * qy, xz = qx * qz, yz = qy * qz;
    float wx = qw * x2, wy = qw * y2, wz = qw * z2;
    float sx = s[0], sy = s[1], sz = s[2];

    out[0] = (1 - (y2 + z2)) * sx;
    out[1] = (xy + wz) * sx;
    out[2] = (xz - wy) * sx;
    out[3] = 0;
    out[4] = (xy - wz) * sy;
    out[5] = (1 - (x2 + z2)) * sy;
    out[6] = (yz + wx) * sy;
    out[7] = 0;
    out[8] = (xz + wy) * sz;
    out[9] = (yz - wx) * sz;
    out[10] = (1 - (x2 + y2)) * sz;
    out[11] = 0;
    out[12] = tx;
    out[13] = ty;
    out[14] = tz;
    out[15] = 1;
  }

  if (node->parent) {
    float parent[16];
    get_node_world_matrix(node->parent, parent);
    float result[16];
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        result[i * 4 + j] = 0;
        for (int k = 0; k < 4; k++) {
          result[i * 4 + j] += parent[i * 4 + k] * out[k * 4 + j];
        }
      }
    }
    memcpy(out, result, 16 * sizeof(float));
  }
}

int main(int argc, char** argv) {
  CookerArgs args = parse_arguments(argc, argv);
  if (!args.input_file || !args.out_bin) {
    return 1;
  }

  cgltf_data* data = load_gltf_file(args.input_file);
  if (!data) {
    return 1;
  }

  NodeInfo* nodes = NULL;
  size_t node_count = 0;
  discover_nodes(data, &nodes, &node_count);

  COOKER_LOG("Found %zu static/col nodes, %d collision nodes", node_count, g_col_node_count);
  COOKER_LOG("=====================");
  COOKER_LOG("About to start graphics loop, nodes_count=%zu", node_count);
  fflush(stdout);

  CookerBucket* buckets = NULL;
  size_t bucket_count = 0;
  buckets = process_graphics(nodes, node_count, args.input_file, &bucket_count);

  free(nodes);

  if (write_graphics_file(args.out_bin, buckets, bucket_count) != 0) {
    free(nodes);
    return 1;
  }

  process_collision(args.out_col);

  cgltf_free(data);
  return 0;
}
