#pragma once

#include <stdint.h>

#define VERTEX_EOL 0xf0000000
#define VERTEX 0xe0000000

#if defined(_arch_dreamcast) || defined(DESKTOP)
#define RGBA_FORMAT (GL_BGRA)
#else
#define RGBA_FORMAT (4)
#endif

// On little-endian, these pack macros arrange bytes in memory to match each struct:
//   PACK_ARGB8888 -> LE bytes [b, g, r, a] -> color_uc_bgra {b,g,r,a}  (Dreamcast glvert_fast_t)
//   PACK_ABGR8888 -> LE bytes [r, g, b, a] -> color_uc       {r,g,b,a}  (desktop/PSP psp_fast_t)
#define PACK_ARGB8888(r, g, b, a) (((uint32_t)(uint8_t)(a) << 24) | ((uint32_t)(uint8_t)(r) << 16) | ((uint32_t)(uint8_t)(g) << 8) | (uint32_t)(uint8_t)(b))
#define PACK_RGBA8888(r, g, b, a) (((uint32_t)(uint8_t)(r) << 24) | ((uint32_t)(uint8_t)(g) << 16) | ((uint32_t)(uint8_t)(b) << 8) | (uint32_t)(uint8_t)(a))
#define PACK_BGRA8888(r, g, b, a) (((uint32_t)(uint8_t)(b) << 24) | ((uint32_t)(uint8_t)(g) << 16) | ((uint32_t)(uint8_t)(r) << 8) | (uint32_t)(uint8_t)(a))
#define PACK_ABGR8888(r, g, b, a) (((uint32_t)(uint8_t)(a) << 24) | ((uint32_t)(uint8_t)(b) << 16) | ((uint32_t)(uint8_t)(g) << 8) | (uint32_t)(uint8_t)(r))

#if defined(_arch_dreamcast)
#define PACK_COLOR(r, g, b, a) PACK_ARGB8888(r, g, b, a)
#else
#define PACK_COLOR(r, g, b, a) PACK_ABGR8888(r, g, b, a)
#endif

#define VTX_COLOR_WHITE .color = {.packed = 0xFFFFFFFF}

typedef struct __attribute__((packed, aligned(4))) vec3f_gl {
  float x, y, z;
} vec3f;

typedef struct __attribute__((packed, aligned(4))) uv_float {
  float u, v;
} uv_float;

typedef struct color_4ub_bgra {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;
} color_4ub_bgra;

typedef struct color_4ub {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} color_4ub;

typedef union color_uc {
  uint8_t array[4];
  uint32_t packed;
  color_4ub named;
} color_uc;

typedef union color_uc_bgra {
  uint8_t array[4];
  uint32_t packed;
  color_4ub_bgra named;
} color_uc_bgra;

typedef struct __attribute__((packed, aligned(4))) glvert_fast_t {
  uint32_t flags;
  struct vec3f_gl vert;
  uv_float texture;
  color_uc_bgra color;  // bgra
  union {
    float pad;
    uint32_t vertindex;
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
