#pragma once

#include <stdint.h>

#define VERTEX_EOL 0xf0000000
#define VERTEX 0xe0000000

#if defined(_arch_dreamcast) || defined(DESKTOP)
#define _RGBA_FORMAT (GL_BGRA)
#else
#define _RGBA_FORMAT (4)
#endif


#if defined(_arch_dreamcast) || defined(DESKTOP)
#define PACK_ARGB8888(r, g, b, a) ((uint32_t)(((uint8_t)(a) << 24) + ((uint8_t)(r) << 16) + ((uint8_t)(g) << 8) + (uint8_t)(b)))
#define PACK_BGRA8888(r, g, b, a) ((uint32_t)(((uint8_t)(b) << 24) + ((uint8_t)(g) << 16) + ((uint8_t)(r) << 8) + (uint8_t)(a)))
#define PACK_RGBA8888(r, g, b, a) ((uint32_t)(((uint8_t)(a) << 24) + ((uint8_t)(b) << 16) + ((uint8_t)(g) << 8) + (uint8_t)(r)))
#else
#define PACK_ARGB8888(r, g, b, a) ((uint32_t)(((uint8_t)(a) << 24) + ((uint8_t)(r) << 16) + ((uint8_t)(g) << 8) + (uint8_t)(b)))
#define PACK_BGRA8888(r, g, b, a) ((uint32_t)(((uint8_t)(b) << 24) + ((uint8_t)(g) << 16) + ((uint8_t)(r) << 8) + (uint8_t)(a)))
#define PACK_RGBA8888(r, g, b, a) ((uint32_t)(((uint8_t)(a) << 24) + ((uint8_t)(b) << 16) + ((uint8_t)(g) << 8) + (uint8_t)(r)))

#define PACK_ARGB8888_OBJ(r, g, b, a) ((uint32_t)(((uint8_t)(a) << 24) + ((uint8_t)(r) << 16) + ((uint8_t)(g) << 8) + (uint8_t)(b)))
#endif

#define VTX_COLOR_WHITE .color = {.packed = 0xFFFFFFFF}

typedef struct __attribute__((packed, aligned(4))) vec3f_gl {
  float x, y, z;
} vec3f;

typedef struct __attribute__((packed, aligned(4))) uv_float {
  float u, v;
} uv_float;

typedef struct color_4ub_bgra {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char a;
} color_4ub_bgra;

typedef struct color_4ub {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} color_4ub;

typedef union color_uc {
  unsigned char array[4];
  unsigned int packed;
  color_4ub named;
} color_uc;

typedef union color_uc_bgra {
  unsigned char array[4];
  unsigned int packed;
  color_4ub_bgra named;
} color_uc_bgra;

typedef struct __attribute__((packed, aligned(4))) glvert_fast_t {
  uint32_t flags;
  struct vec3f_gl vert;
  uv_float texture;
  color_uc_bgra color;  // bgra
  union {
    float pad;
    unsigned int vertindex;
  } pad0;
} glvert_fast_t;

/* must be in order:  [weights (0-8)] [texture uv] [color] [normal] [vertex]
  aligned to 32bits (4bytes)
  we are going to use (GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF  )
*/
typedef struct __attribute__((packed, aligned(4))) psp_fast_t {
  uv_float texture;
  color_uc color;  // bgra
  struct vec3f_gl vert;
} psp_fast_t;

#if defined(_arch_dreamcast)
typedef struct glvert_fast_t VtxFmt;
#else
typedef struct psp_fast_t VtxFmt;
#endif
