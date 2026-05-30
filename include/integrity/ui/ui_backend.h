#pragma once

#include <integrity/common/common.h>
#include <integrity/common/renderer.h>

typedef struct dimen_RECT {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} dimen_RECT;

typedef struct dimen_FRECT {
  float x;
  float y;
  float w;
  float h;
} dimen_FRECT;

extern float text_color[3];
extern float text_alpha;
extern int text_size;

extern const dimen_FRECT rect_tex_0TO1;
extern dimen_RECT rect_pos_fullscreen;

void UI_Init(void);
void UI_Set2D(void);
bool UI_GetScalingEnabled(void);
void UI_SetScalingEnabled(bool enable);
void UI_SetVirtualResolution(int16_t width, int16_t height);
void UI_SetVirtualToReal(void);
void UI_ResetVirtualResolution(void);
dimen_RECT UI_ScaleToVirtual(const dimen_RECT* const input, const dimen_FRECT* const screen);
dimen_RECT UI_RECTFToRECT(dimen_FRECT* input);
void UI_DrawCharacter(int x, int y, unsigned char num);
void UI_DrawCharacterCentered(int x, int y, unsigned char num);
void UI_DrawString(int x, int y, const char* str);
void UI_DrawString_Linebreak(int x, int y, const char* str, int width);
void UI_DrawString_Rot(int x, int y, const char* str, float rad);
void UI_DrawStringCentered(int x, int y, const char* str);
void UI_DrawStringCentered_Rot(int x, int y, const char* str, float rad);
void UI_TextAlpha(float a);
void UI_TextColor(float r, float g, float b);
void UI_TextColorEx(float r, float g, float b, float a);
void UI_DrawPic(int x, int y, tx_image* pic);
void UI_DrawPicDimensions(const dimen_RECT* const rect_pos, tx_image* pic);
void UI_DrawTransPic(int x, int y, tx_image* pic);

void UI_DrawFill(const dimen_RECT* const rect_pos, int r, int g, int b);
void UI_DrawTexturedQuad(const dimen_RECT* const rect_pos, tx_image* pic);

void UI_DrawSprite(const dimen_RECT* const rect_pos, sprite* spr);
void UI_DrawTransSprite(const dimen_RECT* const rect_pos, float alpha, sprite* pic);

void DrawQuad_Rot(const dimen_RECT* const rect_pos, float rad);
void DrawQuad_RotEx(const dimen_RECT* const rect_pos, const dimen_FRECT* const rect_tex, float rad);
void DrawQuad_NoTex_Rot(const dimen_RECT* const rect_pos, float rad);
void DrawQuad_NoTex(const dimen_RECT* rect_pos);
void DrawQuad(const dimen_RECT* const rect_pos, const dimen_FRECT* const rect_tex);
void UI_DrawSprite_Rot(const dimen_RECT* const rect_pos, sprite* spr, float rad);
void UI_DrawTransSprite_Rot(const dimen_RECT* const rect_pos, float alpha, sprite* spr, float rad);

void UI_DrawFadeScreen(void);

static inline void UI_DrawTransPicCentered(int x, int y, tx_image* pic) {
  UI_DrawTransPic(x - (pic->width / 2), y - (pic->height / 2), pic);
}

inline static void UI_TextSize(int size) {
  text_size = size;
}

inline static int UI_TextSizeGet(void) {
  return text_size;
}
