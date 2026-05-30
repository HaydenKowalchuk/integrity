#pragma once

#include <integrity/common/common.h>
#include <integrity/common/renderer.h>

#define MAX_BATCHED_SURFVERTEXES 512

#ifdef _arch_dreamcast
extern glvert_fast_t r_batchedfastvertexes_text[MAX_BATCHED_SURFVERTEXES * 2];
#else
extern psp_fast_t r_batchedfastvertexes_text[MAX_BATCHED_SURFVERTEXES * 2];
#endif
extern uint32_t text_color_packed;
extern int text_size;
extern int r_numsurfvertexes_text;

void R_BeginBatchingSurfacesQuad(void);
void R_EndBatchingSurfacesQuads(void);
// void R_BatchSurfaceQuadText(int x, int y, float frow, float fcol, float size);
static inline void R_BatchSurfaceQuadText(int x, int y, float frow, float fcol, float size) {
  if ((r_numsurfvertexes_text + 6) >= (MAX_BATCHED_SURFVERTEXES * 2))
    R_EndBatchingSurfacesQuads();
#ifdef _arch_dreamcast
  // Vertex 1
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 0] = (glvert_fast_t){.flags = VERTEX, .vert = {x, y, 0}, .texture = {fcol, frow}, .color = {.packed = text_color_packed}, .pad0 = {0}};
  // Vertex 2
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 1] = (glvert_fast_t){.flags = VERTEX, .vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {.packed = text_color_packed}, .pad0 = {0}};
  // Vertex 4
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 2] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {.packed = text_color_packed}, .pad0 = {0}};
  // Vertex 4
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 3] = (glvert_fast_t){.flags = VERTEX, .vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {.packed = text_color_packed}, .pad0 = {0}};
  // Vertex 2
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 4] = (glvert_fast_t){.flags = VERTEX, .vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {.packed = text_color_packed}, .pad0 = {0}};
  // Vertex 3
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 5] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {x + text_size, y + text_size, 0}, .texture = {fcol + size, frow + size}, .color = {.packed = text_color_packed}, .pad0 = {0}};
#else
  // Vertex 1
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 0] = (psp_fast_t){.vert = {x, y, 0}, .texture = {fcol, frow}, .color = {.packed = text_color_packed}};
  // Vertex 2
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 1] = (psp_fast_t){.vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {.packed = text_color_packed}};
  // Vertex 4
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 2] = (psp_fast_t){.vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {.packed = text_color_packed}};
  // Vertex 4
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 3] = (psp_fast_t){.vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {.packed = text_color_packed}};
  // Vertex 2
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 4] = (psp_fast_t){.vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {.packed = text_color_packed}};
  // Vertex 3
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 5] = (psp_fast_t){.vert = {x + text_size, y + text_size, 0}, .texture = {fcol + size, frow + size}, .color = {.packed = text_color_packed}};
#endif

  r_numsurfvertexes_text += 6;
}
