#include <common/common.h>
#include <common/obj_loader.h>
#include <common/renderer.h>
#include <common/resource_manager.h>
#include <stb_image.h>

WINDOW_TITLE("", 0)
scene scene_start;

#include "gl_fast_vert.h"

#define EMF_MESH_PRESENT (1 << 0)

#define EMF_TXR_VTXCOLOR (1 << 1)
#define EMF_TXR_TEXTURE (1 << 0)

#define EMF_VERT_DREAMCAST (1 << 0)
#define EMF_VERT_PSP (1 << 1)

#define FOURCC(a, b, c, d) ((uint32_t)(((d) << 24) | ((c) << 16) | ((b) << 8) | (a)))

#define FOURCC_EHDR FOURCC('E', 'H', 'D', 'R')
#define FOURCC_DCVT FOURCC('D', 'C', 'V', 'T')
#define FOURCC_PSVT FOURCC('P', 'S', 'V', 'T')
#define FOURCC_T24B FOURCC('T', '2', '4', 'B')
#define FOURCC_T32B FOURCC('T', '3', '2', 'B')
#define FOURCC_EEND FOURCC('E', 'E', 'N', 'D')

typedef struct EMF_CHUNK {
  uint32_t chunkType;    // FOURCC
  uint32_t chunkLength;  // Total length of data not including this header
} EMF_CHUNK;

typedef struct {
  uint32_t triCount;       // Total tris in vertex segment, for now only support tris
  uint32_t modelInfo;      // bitfield, unsure, 0 bit is mesh present
  uint32_t textureFormat;  // for included texture data: rgb, rgba, paletted?, (bit indexes) 0 = texture, 1 = vertex color,
                           //  unsigned byte vertexFormat; //UNUSED
} EMF_HEADER;

typedef struct {
  uint32_t channels;
  uint32_t width;
  uint32_t height;
} EMF_BITMAP;

// Similar to PNG Header, specifies EMF
static const unsigned char emfHeader[] = {0x89, 0x45, 0x4D, 0x46, 0x0D, 0x0A, 0x1A, 0x0A};

dc_fast_t* processOBJtoDCVT(model_obj* obj) {
  dc_fast_t* dc_verts = malloc(sizeof(dc_fast_t) * obj->num_tris);
  memset(dc_verts, '\0', sizeof(dc_fast_t) * obj->num_tris);
  printf("size: %d bytes\n", sizeof(dc_fast_t) * obj->num_tris);
  printf("tris: %d\n", obj->num_tris);
  int i;
  for (i = 0; i < obj->num_tris; i++) {
    dc_verts[i] = (dc_fast_t){.flags = (i == 2) ? VERTEX_EOL : VERTEX,
                              .vert = (vec3f){obj->tris[i * 8 + 0], obj->tris[i * 8 + 1], obj->tris[i * 8 + 2]},
                              .texture = (uv_float){obj->tris[i * 8 + 3], obj->tris[i * 8 + 4]},
                              .color = (color_uc){.packed = PACK_BGRA8888(obj->tris[i * 8 + 7] * 255, obj->tris[i * 8 + 6] * 255, obj->tris[i * 8 + 5] * 255, 255)},
                              .pad0 = {.vertindex = 0}};
  }
  return dc_verts;
}

psp_fast_t* processOBJtoPSVT(model_obj* obj) {
  psp_fast_t* psp_verts = malloc(sizeof(psp_fast_t) * obj->num_tris);
  memset(psp_verts, '\0', sizeof(psp_fast_t) * obj->num_tris);
  printf("size: %d bytes\n", sizeof(psp_fast_t) * obj->num_tris);
  printf("tris: %d\n", obj->num_tris);
  int i;
  for (i = 0; i < obj->num_tris; i++) {
    psp_verts[i] = (psp_fast_t){
        .texture = (uv_float){obj->tris[i * 8 + 3], obj->tris[i * 8 + 4]},
        .color = (color_uc){.packed = PACK_BGRA8888(obj->tris[i * 8 + 7] * 255, obj->tris[i * 8 + 6] * 255, obj->tris[i * 8 + 5] * 255, 255)},
        .vert = (vec3f){obj->tris[i * 8 + 0], obj->tris[i * 8 + 1], obj->tris[i * 8 + 2]}};
  }
  return psp_verts;
}

tx_image processIMGtoT24B(const char* filename) {
  tx_image img;

  img.data = stbi_load(filename, &img.width, &img.height, NULL, 3);
  if (img.data == NULL) {
    printf("ERROR: for image(%s)\n", filename);
  }
  return img;
}

tx_image processIMGtoT32B(const char* filename) {
  tx_image img;

  img.data = stbi_load(filename, &(img.width), &(img.height), NULL, 4);

  if (img.data == NULL) {
    printf("ERROR: for image(%s)\n", filename);
  }

  return img;
}

int main(int argc, char** argv) {
  /*
  tx_image raw24 = processIMGtoT24B("skin2.png");
  FILE *second = fopen("image_24.raw", "wb");
  fwrite(raw24.data, (3 * (raw24.width * raw24.height)), 1, second);
  fclose(second);

  tx_image raw32 = processIMGtoT32B("skin3.png");
  second = fopen("image_32.raw", "wb");
  fwrite(raw32.data, (4 * (raw32.width * raw32.height)), 1, second);
  fclose(second);
  */

  (void)argc;
  (void)argv;
  /*Need to init some stuff under the hood*/
  resource_test();

  /* Load an obj file */
  model_obj* obj_file = OBJ_load("basicCharacter.obj");
  if (obj_file == NULL) {
    exit(1);
  }

  dc_fast_t* dc_verts = processOBJtoDCVT(obj_file);
  psp_fast_t* psp_verts = processOBJtoPSVT(obj_file);
  tx_image image_24 = processIMGtoT24B("skin_orc.png");
  // tx_image image_32 = processIMGtoT32B("skin_man.png");

  unsigned int crc;
  EMF_CHUNK chunk;
  EMF_HEADER chunk_header;

  FILE* fp = NULL;

  fp = fopen("basicCharacter.emf", "wb");

  fwrite(emfHeader, sizeof(emfHeader), 1, fp);

  // build and write the model header chunk
  chunk.chunkType = FOURCC_EHDR;
  chunk.chunkLength = sizeof(EMF_HEADER);
  fwrite(&chunk, sizeof(EMF_CHUNK), 1, fp);
  chunk_header.triCount = obj_file->num_tris;
  chunk_header.modelInfo = EMF_MESH_PRESENT;
  chunk_header.textureFormat = EMF_TXR_TEXTURE;
  fwrite(&chunk_header, sizeof(EMF_HEADER), 1, fp);

  crc = 0x12345678;  // bogus placeholder value
  fwrite(&crc, sizeof(crc), 1, fp);

  // build and write the model vertex chunk
  chunk.chunkLength = sizeof(dc_fast_t) * obj_file->num_tris;
  chunk.chunkType = FOURCC_DCVT;
  fwrite(&chunk, sizeof(EMF_CHUNK), 1, fp);
  fwrite(dc_verts, chunk.chunkLength, 1, fp);
  crc = 0x12345678;  // bogus placeholder value
  fwrite(&crc, sizeof(crc), 1, fp);

  // build and write the model vertex chunk
  chunk.chunkLength = sizeof(psp_fast_t) * obj_file->num_tris;
  chunk.chunkType = FOURCC_PSVT;
  fwrite(&chunk, sizeof(EMF_CHUNK), 1, fp);
  fwrite(psp_verts, chunk.chunkLength, 1, fp);
  crc = 0x12345678;  // bogus placeholder value
  fwrite(&crc, sizeof(crc), 1, fp);

  // build and write the 24bit texture chunk
  chunk.chunkLength = sizeof(EMF_BITMAP) + (3 * (image_24.width * image_24.height));
  chunk.chunkType = FOURCC_T24B;
  fwrite(&chunk, sizeof(EMF_CHUNK), 1, fp);
  /* header needed since raw data */
  EMF_BITMAP bitmap24;
  bitmap24.width = image_24.width;
  bitmap24.height = image_24.height;
  bitmap24.channels = 3;
  fwrite(&bitmap24, sizeof(EMF_BITMAP), 1, fp);

  printf("tex [%dx%dx%d]\n", bitmap24.width, bitmap24.height, 3);
  printf("wrote %d + %d bytes\n", sizeof(EMF_BITMAP), (3 * (image_24.width * image_24.height)));

  /* write raw bitmap */
  fwrite(image_24.data, (3 * (image_24.width * image_24.height)), 1, fp);
  crc = 0x12345678;  // bogus placeholder value
  fwrite(&crc, sizeof(crc), 1, fp);

  // build and write the 32bit texture chunk
  /*
  chunk.chunkLength = image_32.channels*(image_32.width * image_32.height);
  chunk.chunkType = FOURCC_T24B;
  fwrite(&chunk, sizeof(EMF_CHUNK), 1, fp);
  fwrite(image_32.data, chunk.chunkLength, 1, fp);
  crc = 0x12345678;  //bogus placeholder value
  fwrite(&crc, sizeof(crc), 1, fp);
  */

  // build and write the model end chunk
  chunk.chunkLength = 0;
  chunk.chunkType = FOURCC_EEND;
  fwrite(&chunk, sizeof(EMF_CHUNK), 1, fp);
  crc = 0x12345678;  // bogus placeholder value
  fwrite(&crc, sizeof(crc), 1, fp);

  fclose(fp);

  return EXIT_SUCCESS;
}