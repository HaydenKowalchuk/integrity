#include <integrity/common/file_access.h>
#include <integrity/common/math_headers.h>
#include <integrity/common/resource_manager.h>
#include <integrity/ui/gl_batcher.h>
#include <integrity/ui/ui_backend.h>

#include "char_basic.h"

tx_image* char_texture;

float text_color[3];
uint32_t text_color_packed;

float text_alpha;
const dimen_FRECT rect_tex_0TO1 = {0, 0, 1.0f, 1.0f};
static dimen_FRECT _rect_pos_fullscreen = {0, 0, 640.0f, 480.0f};
dimen_RECT rect_pos_fullscreen = {0, 0, 640, 480};
static bool ui_scaling = false;

void UI_Init(void) {
  if (FS_FileExists("char.png")) {
  } else {
  }
  char_texture = IMG_load("char.png");
  if (!char_texture->data) {
    free(char_texture);
    char_texture = IMG_load_from_memory(char_basic, char_basic_length);
  }
  RNDR_CreateTextureFromImage(char_texture);
  glm_vec3_copy(GLM_VEC3_ONE, text_color);

  UI_TextColorEx(1, 1, 1, 1);
  UI_TextSize(16);
}

void UI_Destroy(void) {
  if (char_texture) {
    resource_object_remove(char_texture->crc);
  }
}

bool UI_GetScalingEnabled(void) {
  return ui_scaling;
}

void UI_SetScalingEnabled(bool enable) {
  ui_scaling = enable;
}

void UI_SetVirtualToReal(void) {
  _rect_pos_fullscreen.w = SCR_WIDTH;
  _rect_pos_fullscreen.h = SCR_HEIGHT;

  rect_pos_fullscreen.w = SCR_WIDTH;
  rect_pos_fullscreen.h = SCR_HEIGHT;
}

void UI_SetVirtualResolution(int16_t width, int16_t height) {
  _rect_pos_fullscreen.w = width;
  _rect_pos_fullscreen.h = height;

  rect_pos_fullscreen.w = width;
  rect_pos_fullscreen.h = height;
}

void UI_ResetVirtualResolution(void) {
  _rect_pos_fullscreen.w = 640;
  _rect_pos_fullscreen.h = 480;

  rect_pos_fullscreen.w = 640;
  rect_pos_fullscreen.h = 480;
}

dimen_RECT UI_ScaleToVirtual(const dimen_RECT* const input, const dimen_FRECT* const screen) {
  dimen_RECT out = {0};
  out.x = (input->x / screen->w) * 640;
  out.y = (input->y / screen->h) * 480;

  out.w = (input->w / screen->w) * 640;
  out.h = (input->h / screen->h) * 480;
  return out;
}

dimen_RECT UI_RECTFToRECT(dimen_FRECT* input) {
  dimen_RECT out = {input->x, input->y, input->w, input->h};
  return out;
}

void UI_DrawCharacterCentered(int x, int y, unsigned char num) {
  UI_DrawCharacter(x - (1 * text_size / 2), y - (text_size / 2), num);
}

void UI_DrawCharacter(int x, int y, unsigned char num) {
  int row, col;
  float frow, fcol, size;

  if (num == 32) {
    return;  // space
  }

  num &= 255;

  if (y <= -8) {
    return;  // totally off screen
  }

  row = num >> 4;
  col = num & 15;

  frow = row * 0.0625;
  fcol = col * 0.0625;
  size = 0.0625;

  GL_Bind(char_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  dimen_RECT pos = {x, y, text_size, text_size};
  dimen_FRECT tex = {fcol, frow, size, size};

  DrawQuad(&pos, &tex);
}

void UI_DrawString(int x, int y, const char* str) {
  GL_Bind(char_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  // R_BeginBatchingSurfacesQuad();
  const float size = 0.0625f;
  while (*str) {
    unsigned char num = (unsigned char)*str & 255;
    /*if (num == 32)
        {
            x += text_size;
            str++;
            continue;
        }*/

    unsigned char row = num >> 4;
    unsigned char col = num & 15;
    float frow = row * 0.0625f;
    float fcol = col * 0.0625f;

    // R_BatchSurfaceQuadText(x, y, frow, fcol, size);
    dimen_RECT pos = {x, y, text_size, text_size};
    dimen_FRECT tex = {fcol, frow, size, size};
    DrawQuad(&pos, &tex);
    x += text_size;
    str++;
  }
  // R_EndBatchingSurfacesQuads();
}

void UI_DrawString_Linebreak(int x, int y, const char* str, int width) {
  GL_Bind(char_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  // R_BeginBatchingSurfacesQuad();
  const float size = 0.0625f;
  int x_orig = x;
  int line_width = 0;
  while (*str) {
    unsigned char num = (unsigned char)*str & 255;

    if (num == '\n') {
      x += text_size;
      str++;
      continue;
    }

    if (num == 32) {
      line_width += text_size;
      const char* current_word = str + 1;
      int word_length = 0;
      while ((*current_word) && (*current_word != 32)) {
        word_length += text_size;
        current_word++;
      }
      // printf("current len: %d, word '%s' len %d\n", line_width, str + 1, word_length / text_size);
      if (line_width + word_length > width) {
        x = x_orig;
        y += 2 + text_size;
        line_width = 0;
      } else {
        x += text_size;
      }
      str++;
      continue;
    }
    unsigned char row = num >> 4;
    unsigned char col = num & 15;
    float frow = row * 0.0625f;
    float fcol = col * 0.0625f;

    // R_BatchSurfaceQuadText(x, y, frow, fcol, size);
    dimen_RECT pos = {x, y, text_size, text_size};
    dimen_FRECT tex = {fcol, frow, size, size};
    DrawQuad(&pos, &tex);
    x += text_size;
    line_width += text_size;
    str++;
  }
  // R_EndBatchingSurfacesQuads();
}

void UI_DrawStringCentered_Rot(int x, int y, const char* str, float rad) {
  GL_Bind(char_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  const float size = 0.0625f;
  float _x = ((float)x) - (text_size * (strlen(str) / 2.0f - 0.5f) * COS(rad));
  float _y = ((float)y) - (text_size * (strlen(str) / 2.0f - 0.5f) * SIN(rad));
  while (*str) {
    unsigned char num = ((unsigned char)*str) & 255;

    unsigned char row = num >> 4;
    unsigned char col = num & 15;
    float frow = row * 0.0625f;
    float fcol = col * 0.0625f;

    // R_BatchSurfaceQuadText(x, y, frow, fcol, size);
    dimen_RECT pos = {_x, _y, text_size, text_size};
    dimen_FRECT tex = {fcol, frow, size, size};
    DrawQuad_RotEx(&pos, &tex, rad);
    _x += (text_size * COS(rad));
    //_x += text_size;
    _y += (text_size * SIN(rad));
    str++;
  }
  // R_EndBatchingSurfacesQuads();
}

void UI_DrawString_Rot(int x, int y, const char* str, float rad) {
  GL_Bind(char_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  const float size = 0.0625f;
  float _x = x;
  float _y = y;
  while (*str) {
    unsigned char num = ((unsigned char)*str) & 255;

    unsigned char row = num >> 4;
    unsigned char col = num & 15;
    float frow = row * 0.0625f;
    float fcol = col * 0.0625f;

    // R_BatchSurfaceQuadText(x, y, frow, fcol, size);
    dimen_RECT pos = {_x, _y, text_size, text_size};
    dimen_FRECT tex = {fcol, frow, size, size};
    DrawQuad_RotEx(&pos, &tex, rad);
    _x += (text_size * COS(rad));
    //_x += text_size;
    _y += (text_size * SIN(rad));
    str++;
  }
  // R_EndBatchingSurfacesQuads();
}

void UI_DrawStringCentered(int x, int y, const char* str) {
  UI_DrawString(x - (strlen(str) * text_size / 2), y - (text_size / 2), str);
}
void UI_TextAlpha(float a) {
  text_alpha = a;

  text_color_packed = PACK_COLOR(255 * text_color[0], 255 * text_color[1], 255 * text_color[2], 255 * text_alpha);
}
void UI_TextColor(float r, float g, float b) {
  UI_TextColorEx(r, g, b, 1.0f);
}
void UI_TextColorEx(float r, float g, float b, float a) {
  text_color[0] = MIN(r, 1.0f);
  text_color[1] = MIN(g, 1.0f);
  text_color[2] = MIN(b, 1.0f);
  text_alpha = a;

  text_color_packed = PACK_COLOR(255 * text_color[0], 255 * text_color[1], 255 * text_color[2], 255 * text_alpha);
}

static inline void Draw_AlphaPic(int x, int y, tx_image* pic) {
  UI_DrawPic(x, y, pic);
}

void UI_DrawPic(int x, int y, tx_image* pic) {
  glColor4f(1, 1, 1, 1);
  GL_Bind(pic);
  dimen_RECT pos = {x, y, pic->width, pic->height};
  DrawQuad(&pos, &rect_tex_0TO1);
}

void UI_DrawPicDimensions(const dimen_RECT* const rect, tx_image* pic) {
  glColor4f(1, 1, 1, 1);
  GL_Bind(pic);
  DrawQuad(rect, &rect_tex_0TO1);
}

void UI_DrawTransSprite(const dimen_RECT* const rect, float alpha, sprite* spr) {
  float old_alpha = text_alpha;
  text_alpha = alpha;
  GL_Bind(spr->parent);
  dimen_FRECT tex = {spr->u, spr->v, spr->width, spr->height};
  DrawQuad(rect, &tex);
  text_alpha = old_alpha;
}

void UI_DrawSprite(const dimen_RECT* const rect, sprite* spr) {
  UI_TextColorEx(1, 1, 1, 1);
  GL_Bind(spr->parent);
  dimen_FRECT tex = {spr->u, spr->v, spr->width, spr->height};
  DrawQuad(rect, &tex);
}

void UI_DrawTransSprite_Rot(const dimen_RECT* const rect, float alpha, sprite* spr, float rad) {
  float old_alpha = text_alpha;
  text_alpha = alpha;
  GL_Bind(spr->parent);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  dimen_FRECT tex = {spr->u, spr->v, spr->width, spr->height};
  DrawQuad_RotEx(rect, &tex, rad);
  text_alpha = old_alpha;
}

void UI_DrawSprite_Rot(const dimen_RECT* const rect, sprite* spr, float rad) {
  UI_TextColorEx(1, 1, 1, 1);
  GL_Bind(spr->parent);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  dimen_FRECT tex = {spr->u, spr->v, spr->width, spr->height};
  DrawQuad_RotEx(rect, &tex, rad);
}

void UI_DrawTransPic(int x, int y, tx_image* pic) {
  Draw_AlphaPic(x, y, pic);
}

void UI_DrawFill(const dimen_RECT* rect, int r, int g, int b) {
  float _r = text_color[0];
  float _g = text_color[1];
  float _b = text_color[2];
  UI_TextColor(r / 255.0f, g / 255.0f, b / 255.0f);
  DrawQuad_NoTex(rect);
  UI_TextColor(_r, _g, _b);
}

void UI_DrawTexturedQuad(const dimen_RECT* const rect, tx_image* pic) {
  GL_Bind(pic);
  DrawQuad(rect, &rect_tex_0TO1);
}

void UI_DrawFadeScreen(void) {
  glEnable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
#define OPACITY 0.6f

  float vertex[(3 + 4) * 4] = {// xyz,rgba
                               0,
                               0,
                               0,
                               0.0f,
                               0.0f,
                               0.0f,
                               OPACITY,
                               0,
                               480.0f,
                               0,
                               0.0f,
                               0.0f,
                               0.0f,
                               OPACITY,
                               640.0f,
                               0,
                               0,
                               0.0f,
                               0.0f,
                               0.0f,
                               OPACITY,
                               640.0f,
                               480.0f,
                               0,
                               0.0f,
                               0.0f,
                               0.0f,
                               OPACITY};

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glVertexPointer(3, GL_FLOAT, 7 * sizeof(GLfloat), vertex);
  glColorPointer(4, GL_FLOAT, 7 * sizeof(GLfloat), vertex + 3);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  glDisable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
}

void UI_Set2D(void) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, 640, 480, 0, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void DrawQuad_NoTex(const dimen_RECT* _rect_pos) {
  const dimen_RECT* rect_pos = _rect_pos;
  dimen_RECT temp;
  if (__builtin_expect(ui_scaling, false)) {
    temp = UI_ScaleToVirtual(_rect_pos, &_rect_pos_fullscreen);
    rect_pos = &temp;
  }
  float vertex[6 * 3] = {
      rect_pos->x, rect_pos->y + rect_pos->h, 0, rect_pos->x + rect_pos->w, rect_pos->y + rect_pos->h, 0, rect_pos->x + rect_pos->w, rect_pos->y, 0, rect_pos->x + rect_pos->w, rect_pos->y, 0, rect_pos->x, rect_pos->y, 0, rect_pos->x, rect_pos->y + rect_pos->h, 0};

  unsigned int color_val = PACK_COLOR(255 * text_color[0], 255 * text_color[1], 255 * text_color[2], 255 * text_alpha);
  unsigned int color_arr[6] = {color_val, color_val, color_val, color_val, color_val, color_val};

  glEnable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, vertex);
  glColorPointer(RGBA_FORMAT, GL_UNSIGNED_BYTE, 0, (uint8_t*)&color_arr[0]);

  glDrawArrays(GL_TRIANGLES, 0, 6);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);

  glDisable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
}

void DrawQuad(const dimen_RECT* const _rect_pos, const dimen_FRECT* const rect_tex) {
  const dimen_RECT* rect_pos = _rect_pos;
  dimen_RECT temp;
  if (__builtin_expect(ui_scaling, false)) {
    temp = UI_ScaleToVirtual(_rect_pos, &_rect_pos_fullscreen);
    rect_pos = &temp;
  }
  float vertex[6 * 3] = {
      rect_pos->x, rect_pos->y + rect_pos->h, 0, rect_pos->x + rect_pos->w, rect_pos->y + rect_pos->h, 0, rect_pos->x + rect_pos->w, rect_pos->y, 0, rect_pos->x + rect_pos->w, rect_pos->y, 0, rect_pos->x, rect_pos->y, 0, rect_pos->x, rect_pos->y + rect_pos->h, 0};
  float texcoord[6 * 2] = {rect_tex->x, rect_tex->y + rect_tex->h, rect_tex->x + rect_tex->w, rect_tex->y + rect_tex->h, rect_tex->x + rect_tex->w, rect_tex->y, rect_tex->x + rect_tex->w, rect_tex->y, rect_tex->x, rect_tex->y, rect_tex->x, rect_tex->y + rect_tex->h};

  unsigned int color_val = PACK_COLOR(255 * text_color[0], 255 * text_color[1], 255 * text_color[2], 255 * text_alpha);
  unsigned int color_arr[6] = {color_val, color_val, color_val, color_val, color_val, color_val};

  glEnable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, vertex);
  glTexCoordPointer(2, GL_FLOAT, 0, texcoord);
  glColorPointer(RGBA_FORMAT, GL_UNSIGNED_BYTE, 0, (uint8_t*)&color_arr[0]);

  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisable(GL_BLEND);
}
void DrawQuad_RotEx(const dimen_RECT* const _rect_pos, const dimen_FRECT* const rect_tex, float rad) {
  float halfWidth = _rect_pos->w / 2;
  float halfHeight = _rect_pos->h / 2;
  float c = COS(rad);
  float s = SIN(rad);
  float r1x = -halfWidth * c - halfHeight * s;
  float r1y = -halfWidth * s + halfHeight * c;
  float r2x = halfWidth * c - halfHeight * s;
  float r2y = halfWidth * s + halfHeight * c;

  float vertex[6 * 3] = {_rect_pos->x + r1x, _rect_pos->y + r1y, 0, _rect_pos->x + r2x, _rect_pos->y + r2y, 0, _rect_pos->x - r1x, _rect_pos->y - r1y, 0, _rect_pos->x - r1x, _rect_pos->y - r1y, 0, _rect_pos->x - r2x, _rect_pos->y - r2y, 0, _rect_pos->x + r1x, _rect_pos->y + r1y, 0};

  float texcoord[6 * 2] = {rect_tex->x, rect_tex->y + rect_tex->h, rect_tex->x + rect_tex->w, rect_tex->y + rect_tex->h, rect_tex->x + rect_tex->w, rect_tex->y, rect_tex->x + rect_tex->w, rect_tex->y, rect_tex->x, rect_tex->y, rect_tex->x, rect_tex->y + rect_tex->h};

  unsigned int color_val = PACK_COLOR(255 * text_color[0], 255 * text_color[1], 255 * text_color[2], 255 * text_alpha);
  unsigned int color_arr[6] = {color_val, color_val, color_val, color_val, color_val, color_val};

  glEnable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_CULL_FACE);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, vertex);
  glTexCoordPointer(2, GL_FLOAT, 0, texcoord);
  glColorPointer(RGBA_FORMAT, GL_UNSIGNED_BYTE, 0, (uint8_t*)&color_arr[0]);

  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisable(GL_BLEND);
}

void DrawQuad_Rot(const dimen_RECT* const rect_pos, float rad) {
  DrawQuad_RotEx(rect_pos, &rect_tex_0TO1, rad);
}

void DrawQuad_NoTex_Rot(const dimen_RECT* const rect_pos, float rad) {
  glDisable(GL_TEXTURE_2D);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  DrawQuad_RotEx(rect_pos, &rect_tex_0TO1, rad);
  glDisable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
}
