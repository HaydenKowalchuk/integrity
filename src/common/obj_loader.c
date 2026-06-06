#include <integrity/common/file_access.h>
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <integrity/common/obj_loader.h>
#include <integrity/common/renderer.h>
#include <integrity/common/renderer_types.h>
#include <integrity/common/resource_manager.h>

#define GAMEJAM_LOG_GROUP "obj_loader"
#define GAMEJAM_LOG_LEVEL (2)
#include <gamejam/log.h>

const char* safe_dirname(const char* path) {
  GAMEJAM_LOG_DEBUG("%s input %s", __func__, path);
  static char dir_buffer[FS_MAX_PATH] = {0};
  static const char dot[] = ".";
  if (!path) {
    return dot;
  }

  memset(dir_buffer, 0, FS_MAX_PATH);
  int last_slash_pos = -1;
  for (size_t i = 0; path[i]; i++) {
    if (i >= (FS_MAX_PATH)) {
      return dot;
    }
    if (path[i] == '/') {
      last_slash_pos = i;
    }
  }
  if (last_slash_pos == -1) {
    return dot;
  }
  strncpy(dir_buffer, path, last_slash_pos);
  dir_buffer[last_slash_pos] = '\0';

  GAMEJAM_LOG_DEBUG("%s output %s", __func__, dir_buffer);
  return dir_buffer;
}

#include <tinyobj_loader_c.h>

extern char error_str[64];

static int LoadObjAndConvert(const char* filename, model_obj* obj);

static model_obj* OBJ_load_internal(const char* path) {
  GAMEJAM_LOG_DEBUG("%s input %s", __func__, path);
  if (path == NULL) {
    GAMEJAM_LOG_ERROR("Error %s given null path!", __func__);
  }

  GAMEJAM_LOG_DEBUG("Attempting load: %s", path);

  uint32_t crc = 0;
  int length = strlen(path);
  crc32(&crc, (uint8_t*)path, length);
  GAMEJAM_LOG_DEBUG("path: %d %lu %s", length, crc, path);

  if (resource_object_find(crc) == -1) {
    GAMEJAM_LOG_DEBUG("model_obj malloc(%d)", sizeof(model_obj));
    model_obj* obj = malloc(sizeof(model_obj));
    GAMEJAM_LOG_DEBUG("SUCCESS model_obj malloc(%d)", sizeof(model_obj));
    if (obj == NULL) {
      GAMEJAM_LOG_ERROR("malloc failed!");
      return NULL;
    }
    GAMEJAM_LOG_DEBUG("model_bj memset(%p, %d)", obj, sizeof(model_obj));
    memset(obj, 0, sizeof(model_obj));

    /* Check if file even exists */
    if (!FS_FileExists(path)) {
      GAMEJAM_LOG_ERROR("ERROR: File %s not found", path);
      exit(0);
      return obj;
    }

    obj->crc = crc;

    GAMEJAM_LOG_DEBUG("about loadobj");
    int res = LoadObjAndConvert(path, obj);
    if (res != 0) {
      GAMEJAM_LOG_ERROR("Error in loadobjandconvert!");
      free(obj);
      return NULL;
    }
    if (obj->tris == NULL) {
      GAMEJAM_LOG_ERROR("ERROR: for path(%s)", path);
      free(obj);
      return NULL;
    }

    GAMEJAM_LOG_DEBUG("Adding new OBJ! crc: %08X, %s", (unsigned int)crc, path);

    resource_object_add('o', crc, obj);
    return obj;
  } else {
    GAMEJAM_LOG_DEBUG("Found already loaded OBJ! crc: %08X, %s", (unsigned int)crc, path);

    resource_object_add('o', crc, NULL);
    return (model_obj*)(resource_object_pointer(crc));
  }
}

model_obj* OBJ_load(const char* path) {
  GAMEJAM_LOG_DEBUG("enter %s input %s", __func__, path);
  return OBJ_load_internal(FS_ResolvePathTemp(path));
}

model_obj* OBJ_load_boolean(const char* path, bool transform) {
  GAMEJAM_LOG_DEBUG("enter %s input %s", __func__, path);
  return OBJ_load_internal((transform) ? FS_ResolvePathTemp(path) : path);
}

void OBJ_destroy(model_obj** obj) {
  GAMEJAM_LOG_DEBUG("enter %s", __func__);
  if (*obj != NULL) {
    if ((*obj)->tris) {
      free((*obj)->tris);
    }

    if ((*obj)->is_ptr) {
      free(*obj);
    }
    memset(*obj, '\0', sizeof(model_obj));
    *obj = NULL;
  }
}

static void* obj_ptr = NULL;
static void get_file_data(void* ctx, const char* filename, const int is_mtl, const char* obj_filename, char** data, size_t* len) {
  // NOTE: If you allocate the buffer with malloc(),
  // You can define your own memory management struct and pass it through `ctx`
  // to store the pointer and free memories at clean up stage(when you quit an
  // app)
  // This example uses mmap(), so no free() required.

  if (!filename) {
    fprintf(stderr, "null filename");
    (*data) = NULL;
    (*len) = 0;
    return;
  }

  if (is_mtl) {
    (*data) = NULL;
    (*len) = 0;
    return;
  }

  FILE* fp = fopen(filename, "rb");
  if (!fp) {
    (*data) = NULL;
    (*len) = 0;
    return;
  }

  fseek(fp, 0, SEEK_END);
  size_t file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  void* bundle = malloc(file_size);
  if (!bundle) {
    fclose(fp);
    (*data) = NULL;
    (*len) = 0;
    return;
  }

  if (fread(bundle, file_size, 1, fp) != 1) {
    free(bundle);
    fclose(fp);
    (*data) = NULL;
    (*len) = 0;
    return;
  }

  obj_ptr = bundle;

  *data = bundle;
  (*len) = file_size;
}

static int LoadObjAndConvert(const char* path, model_obj* obj) {
  GAMEJAM_LOG_DEBUG("%s input %s", __func__, path);
  if (path == NULL) {
    GAMEJAM_LOG_ERROR("Error %s given null path!", __func__);
    return EXIT_FAILURE;
  }

  tinyobj_attrib_t attrib;
  tinyobj_shape_t* shapes = NULL;
  size_t num_shapes = 0;
  tinyobj_material_t* materials = NULL;
  size_t num_materials = 0;
  int read = 0;

  float bmin[3], bmax[3];

  obj->num_faces = 0;
  obj->num_tris = 0;

  {
    unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
    int ret = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials, &num_materials, path, get_file_data, NULL, flags);
    if (ret != TINYOBJ_SUCCESS) {
      return EXIT_FAILURE;
    }

    GAMEJAM_LOG_DEBUG("# of shapes    = %d", (int)num_shapes);
    GAMEJAM_LOG_DEBUG("# of materials = %d", (int)num_materials);

    {
      size_t i;
      for (i = 0; i < num_shapes; i++) {
        GAMEJAM_LOG_DEBUG("shape[%d] name = %s", i, shapes[i].name);
      }
    }
  }

  /* Print out materials */
  for (size_t i = 0; i < num_materials; i++) {
    // GAMEJAM_LOG_DEBUG("mat: %s\nc %3.2f %3.2f %3.2f\n", materials[i].name, materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
  }

  bmin[0] = bmin[1] = bmin[2] = FLT_MAX;
  bmax[0] = bmax[1] = bmax[2] = -FLT_MAX;

  int face_offset = 0;
  size_t i;

  /* Assume triangulated face. */
  int num_triangles = attrib.num_face_num_verts;
  GAMEJAM_LOG_DEBUG("Num verts: %d", attrib.num_face_num_verts - 1);

  VtxFmt* verts = malloc(sizeof(VtxFmt) * num_triangles * 3);

  for (i = 0; i < attrib.num_face_num_verts; i++) {
    assert(attrib.face_num_verts[i] % 3 == 0); /* assume all triangle faces. */

    for (int face = 0; face < (int)attrib.face_num_verts[i] / 3; face++) {
      int k;
      float v[3][5];
      float vt[3][2];
      // float uv[3][2];
      float c[3][3];
      // float len2;

      tinyobj_vertex_index_t idx0 = attrib.faces[face_offset + 3 * face + 0];
      tinyobj_vertex_index_t idx1 = attrib.faces[face_offset + 3 * face + 1];
      tinyobj_vertex_index_t idx2 = attrib.faces[face_offset + 3 * face + 2];

      for (k = 0; k < 3; k++) {
        int f0 = idx0.v_idx;
        int f1 = idx1.v_idx;
        int f2 = idx2.v_idx;
        // printf("making face: %d// %d// %d//\n", f0+1,f1+1,f2+1);

        assert(f0 >= 0);
        assert(f1 >= 0);
        assert(f2 >= 0);

        v[0][k] = attrib.vertices[3 * (unsigned int)f0 + k];
        v[1][k] = attrib.vertices[3 * (unsigned int)f1 + k];
        v[2][k] = attrib.vertices[3 * (unsigned int)f2 + k];
        c[0][k] = attrib.colors[3 * (unsigned int)f0 + k];
        c[1][k] = attrib.colors[3 * (unsigned int)f1 + k];
        c[2][k] = attrib.colors[3 * (unsigned int)f2 + k];
        if (k < 2) {
          vt[0][k] = attrib.texcoords[2 * idx0.vt_idx + k];
          vt[1][k] = attrib.texcoords[2 * idx1.vt_idx + k];
          vt[2][k] = attrib.texcoords[2 * idx2.vt_idx + k];
        }
        bmin[k] = (v[0][k] < bmin[k]) ? v[0][k] : bmin[k];
        bmin[k] = (v[1][k] < bmin[k]) ? v[1][k] : bmin[k];
        bmin[k] = (v[2][k] < bmin[k]) ? v[2][k] : bmin[k];
        bmax[k] = (v[0][k] > bmax[k]) ? v[0][k] : bmax[k];
        bmax[k] = (v[1][k] > bmax[k]) ? v[1][k] : bmax[k];
        bmax[k] = (v[2][k] > bmax[k]) ? v[2][k] : bmax[k];
      }
#if 0 /* Not using Normals */
        if (attrib.num_normals > 0)
        {
          int f0 = idx0.vn_idx;
          int f1 = idx1.vn_idx;
          int f2 = idx2.vn_idx;

          if (f0 >= 0 && f1 >= 0 && f2 >= 0)
          {
            assert(f0 < (int)attrib.num_normals);
            assert(f1 < (int)attrib.num_normals);
            assert(f2 < (int)attrib.num_normals);
            for (k = 0; k < 3; k++)
            {
              n[0][k] = attrib.normals[3 * (int)f0 + k];
              n[1][k] = attrib.normals[3 * (int)f1 + k];
              n[2][k] = attrib.normals[3 * (int)f2 + k];
            }
          }
          else
          { /* normal index is not defined for this face */
            /* compute geometric normal */
            CalcNormal(n[0], v[0], v[1], v[2]);
            n[1][0] = n[0][0];
            n[1][1] = n[0][1];
            n[1][2] = n[0][2];
            n[2][0] = n[0][0];
            n[2][1] = n[0][1];
            n[2][2] = n[0][2];
          }

        }
        else
        {
          /* compute geometric normal */
          CalcNormal(n[0], v[0], v[1], v[2]);
          n[1][0] = n[0][0];
          n[1][1] = n[0][1];
          n[1][2] = n[0][2];
          n[2][0] = n[0][0];
          n[2][1] = n[0][1];
          n[2][2] = n[0][2];
        }
#endif

      for (k = 0; k < 3; k++) {
        // printf("i = %d & k = %d [%d %d %d]\n", i, k, (3 * stride * i) + k *
        // stride + 0, (9 * i) + k * stride + 1, (3 * stride * i) + k * stride +
        // 2);
        //  Vertex Positions
        // vb[(3 * stride * i) + k * stride + 0] = v[k][0];
        // vb[(3 * stride * i) + k * stride + 1] = v[k][1];
        // vb[(3 * stride * i) + k * stride + 2] = v[k][2];
        verts[(3 * i) + k].vert.x = v[k][0];
        verts[(3 * i) + k].vert.y = v[k][1];
        verts[(3 * i) + k].vert.z = v[k][2];

        // Texture UVs
        // vb[(3 * stride * i) + k * stride + 3] = vt[k][0];
        // vb[(3 * stride * i) + k * stride + 4] = 1.0f - vt[k][1];
        verts[(3 * i) + k].texture.u = vt[k][0];
        verts[(3 * i) + k].texture.v = 1.0f - vt[k][1];

        // Vertex Color
        if (attrib.material_ids[i] == -1) {
          // vb[(3 * stride * i) + k * stride + 5] = c[k][0];
          // vb[(3 * stride * i) + k * stride + 6] = c[k][1];
          // vb[(3 * stride * i) + k * stride + 7] = c[k][2];
          uint8_t r = (c[k][0] * 255.0f);
          uint8_t g = (c[k][1] * 255.0f);
          uint8_t b = (c[k][2] * 255.0f);
          verts[(3 * i) + k].color.named.r = r;
          verts[(3 * i) + k].color.named.g = g;
          verts[(3 * i) + k].color.named.b = b;
          verts[(3 * i) + k].color.named.a = 255;
        } else {
          // vb[(3 * stride * i) + k * stride + 5] =
          //     materials[attrib.material_ids[i]].diffuse[0];
          // vb[(3 * stride * i) + k * stride + 6] =
          //     materials[attrib.material_ids[i]].diffuse[1];
          // vb[(3 * stride * i) + k * stride + 7] =
          //     materials[attrib.material_ids[i]].diffuse[2];
          verts[(3 * i) + k].color.named.r = (materials[attrib.material_ids[i]].diffuse[0] / 255.0f);
          verts[(3 * i) + k].color.named.g = (materials[attrib.material_ids[i]].diffuse[1] / 255.0f);
          verts[(3 * i) + k].color.named.b = (materials[attrib.material_ids[i]].diffuse[2] / 255.0f);
          verts[(3 * i) + k].color.named.a = 255;
        }

        // vb[(3 * i + k) * stride + 3] = n[k][0];
        // vb[(3 * i + k) * stride + 4] = n[k][1];
        // vb[(3 * i + k) * stride + 5] = n[k][2];
#if 0
          /* Use normal as color. */
          c[0] = n[k][0];
          c[1] = n[k][1];
          c[2] = n[k][2];
          len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
          if (len2 > 0.0f)
          {
            float len = (float)SQRT((double)len2);

            c[0] /= len;
            c[1] /= len;
            c[2] /= len;
          }


          vb[(3 * i + k) * stride + 6] = (c[0] * 0.5f + 0.5f);
          vb[(3 * i + k) * stride + 7] = (c[1] * 0.5f + 0.5f);
          vb[(3 * i + k) * stride + 8] = (c[2] * 0.5f + 0.5f);
#endif
      }
    }
    face_offset += (int)attrib.face_num_verts[i];
  }

  for (i = 1; i < (size_t)(num_triangles * 3); i += 3) {
    // printf("f  %d// %d// %d//\n",i, i+1, i+2);
  }

  if (num_triangles > 0) {
    obj->num_tris = (int)num_triangles * 3;
    obj->num_faces = (int)attrib.num_face_num_verts;
    obj->tris = verts;
    memcpy(obj->min, bmin, sizeof(bmin));
    memcpy(obj->max, bmax, sizeof(bmax));
  }

  // GAMEJAM_LOG_DEBUG("bmin = %f, %f, %f\n", (double)bmin[0], (double)bmin[1], (double)bmin[2]);
  // GAMEJAM_LOG_DEBUG("bmax = %f, %f, %f\n", (double)bmax[0], (double)bmax[1], (double)bmax[2]);

  (void)read;

  if (obj_ptr) {
    free(obj_ptr);
  }

  tinyobj_attrib_free(&attrib);
  tinyobj_shapes_free(shapes, num_shapes);
  tinyobj_materials_free(materials, num_materials);
  return EXIT_SUCCESS;
}

// Routine to load an tx_image as a texture.
GLuint RNDR_CreateTextureFromImageEx(tx_image* img, int wrap_s, int wrap_t, int mag_filter, int min_filter) {
  GAMEJAM_LOG_DEBUG("enter %s", __func__);
  if ((img->id == 0) && (img->data == NULL)) {
    return 0;
  } else if ((img->id != 0) && (img->data == NULL)) {
    return img->id;
  }
  glGenTextures(1, &(img->id));
  if (img->id) {
    glBindTexture(GL_TEXTURE_2D, img->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexImage2D(GL_TEXTURE_2D, 0, img->channels, img->width, img->height, 0, (img->channels == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, img->data);
    IMG_unload(img);
    return img->id;
  } else {
    return 0;
  }
}

GLuint RNDR_CreateTextureFromImage(tx_image* img) {
  return RNDR_CreateTextureFromImageEx(img, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);
}

void OBJ_bind(model_obj* obj) {
  glVertexPointer(3, GL_FLOAT, sizeof(VtxFmt), &obj->tris[0].vert);
  glTexCoordPointer(2, GL_FLOAT, sizeof(VtxFmt), &obj->tris[0].texture);
  glColorPointer(RGBA_FORMAT, GL_UNSIGNED_BYTE, sizeof(VtxFmt), &obj->tris[0].color);
}
