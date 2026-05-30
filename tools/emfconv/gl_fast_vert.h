#pragma once

typedef struct __attribute__((packed, aligned(4))) dc_fast_t {
  uint32_t flags;
  struct vec3f_gl vert;
  uv_float texture;
  color_uc color;  // bgra
  union {
    float pad;
    unsigned int vertindex;
  } pad0;
} dc_fast_t;

/* must be in order:  [weights (0-8)] [texture uv] [color] [normal] [vertex]
  aligned to 32bits (4bytes)
  we are going to use (GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF  )
*/
typedef struct __attribute__((packed, aligned(4))) psp_fast_t {
  uv_float texture;
  color_uc color;  // bgra
  struct vec3f_gl vert;
} psp_fast_t;
