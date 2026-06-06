#pragma once

#include <integrity/common/common.h>
#include <integrity/common/renderer.h>

#define MAX_BATCHED_SURFVERTEXES 512

extern VtxFmt r_batchedfastvertexes_text[MAX_BATCHED_SURFVERTEXES * 2];
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
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 0] = (VtxFmt){.flags = VERTEX, .vert = {x, y, 0}, .texture = {fcol, frow}, .color = {.packed = text_color_packed}, .pad0 = {0}};
  // Vertex 2
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 1] = (VtxFmt){.flags = VERTEX, .vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {.packed = text_color_packed}, .pad0 = {0}};
  // Vertex 4
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 2] = (VtxFmt){.flags = VERTEX_EOL, .vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {.packed = text_color_packed}, .pad0 = {0}};
  // Vertex 4
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 3] = (VtxFmt){.flags = VERTEX, .vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {.packed = text_color_packed}, .pad0 = {0}};
  // Vertex 2
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 4] = (VtxFmt){.flags = VERTEX, .vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {.packed = text_color_packed}, .pad0 = {0}};
  // Vertex 3
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 5] = (VtxFmt){.flags = VERTEX_EOL, .vert = {x + text_size, y + text_size, 0}, .texture = {fcol + size, frow + size}, .color = {.packed = text_color_packed}, .pad0 = {0}};
#else
  // Vertex 1
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 0] = (VtxFmt){.vert = {x, y, 0}, .texture = {fcol, frow}, .color = {.packed = text_color_packed}};
  // Vertex 2
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 1] = (VtxFmt){.vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {.packed = text_color_packed}};
  // Vertex 4
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 2] = (VtxFmt){.vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {.packed = text_color_packed}};
  // Vertex 4
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 3] = (VtxFmt){.vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {.packed = text_color_packed}};
  // Vertex 2
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 4] = (VtxFmt){.vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {.packed = text_color_packed}};
  // Vertex 3
  r_batchedfastvertexes_text[r_numsurfvertexes_text + 5] = (VtxFmt){.vert = {x + text_size, y + text_size, 0}, .texture = {fcol + size, frow + size}, .color = {.packed = text_color_packed}};
#endif

  r_numsurfvertexes_text += 6;
}
