#include <integrity/ui/gl_batcher.h>

VtxFmt r_batchedfastvertexes_text[MAX_BATCHED_SURFVERTEXES * 2];
int text_size = 16;
int r_numsurfvertexes_text;

void R_BeginBatchingSurfacesQuad(void) {
  r_numsurfvertexes_text = 0;
}

void R_EndBatchingSurfacesQuads(void) {
  if (r_numsurfvertexes_text) {
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
#if defined(_arch_dreamcast)
    glVertexPointer(3, GL_FLOAT, sizeof(VtxFmt), &r_batchedfastvertexes_text[0].vert);
    glTexCoordPointer(2, GL_FLOAT, sizeof(VtxFmt), &r_batchedfastvertexes_text[0].texture);
    glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(VtxFmt), &r_batchedfastvertexes_text[0].color);
#else
    glInterleavedArrays(GL_T2F_C4UB_V3F, 0, &r_batchedfastvertexes_text[0]);
#endif

    glDrawArrays(GL_TRIANGLES, 0, r_numsurfvertexes_text);
    glDisableClientState(GL_COLOR_ARRAY);

    glDisable(GL_BLEND);
    r_numsurfvertexes_text = 0;
  }
}

#if 0
void R_BatchSurfaceQuadText(int x, int y, float frow, float fcol, float size)
{

   if ((r_numsurfvertexes_text + 6) >= (MAX_BATCHED_SURFVERTEXES * 2))
      R_EndBatchingSurfacesQuads();

#ifdef _arch_dreamcast
   uint8_t color[4] = {(uint8_t)(255 * text_color[2]), (uint8_t)(255 * text_color[1]), (uint8_t)(255 * text_color[0]), (uint8_t)(255 * text_alpha)};
   //uint8_t color[4] = {(uint8_t)(255 * text_alpha), (uint8_t)(255 * text_color[0]), (uint8_t)(255 * text_color[1]), (uint8_t)(255 * text_color[2])};
#else
   uint8_t color[4] = PACK_COLOR() {(uint8_t)(255 * text_color[0]), (uint8_t)(255 * text_color[1]), (uint8_t)(255 * text_color[2]), (uint8_t)(255 * text_alpha)};
#endif

   //Vertex 1
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (VtxFmt){.flags = VERTEX, .vert = {x, y, 0}, .texture = {fcol, frow}, .color = {color[0], color[1], color[2], color[3]}, .pad0 = {0}};

   //Vertex 2
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (VtxFmt){.flags = VERTEX, .vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {color[0], color[1], color[2], color[3]}, .pad0 = {0}};

   //Vertex 4
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (VtxFmt){.flags = VERTEX_EOL, .vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {color[0], color[1], color[2], color[3]}, .pad0 = {0}};

   //Vertex 4
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (VtxFmt){.flags = VERTEX, .vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {color[0], color[1], color[2], color[3]}, .pad0 = {0}};

   //Vertex 2
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (VtxFmt){.flags = VERTEX, .vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {color[0], color[1], color[2], color[3]}, .pad0 = {0}};

   //Vertex 3
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (VtxFmt){.flags = VERTEX_EOL, .vert = {x + text_size, y + text_size, 0}, .texture = {fcol + size, frow + size}, .color = {color[0], color[1], color[2], color[3]}, .pad0 = {0}};
}
#endif
