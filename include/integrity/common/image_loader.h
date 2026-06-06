#pragma once

#include <integrity/common/common.h>

typedef struct tx_image {
  char name[16];
  int width;
  int height;
  int channels;
  unsigned char* data;
  unsigned int id;
  unsigned int crc;
} tx_image;

typedef struct sprite {
  tx_image* parent;
  float u, v;
  float width, height;
  int i_width, i_height;
} sprite;

tx_image* IMG_load(const char* path);
tx_image* IMG_load_boolean(const char* path, bool transform);
tx_image* IMG_load_from_memory(const unsigned char* buffer, int len);
void IMG_unload(tx_image* img);
void IMG_destroy(tx_image** img);

sprite IMG_create_sprite(tx_image* img, int x, int y, int x2, int y2);
sprite IMG_create_sprite_scaled(tx_image* img, int x, int y, int x2, int y2, float scale);

static inline sprite IMG_create_sprite_scaled_alt(tx_image* img, int x, int y, int w, int h, float scale) {
  return IMG_create_sprite(img, (int)((float)x * scale), (int)((float)y * scale), (int)((float)(x + w) * scale), (int)((float)(y + h) * scale));
}

static inline sprite IMG_create_sprite_alt(tx_image* img, int x, int y, int w, int h) {
  return IMG_create_sprite(img, x, y, x + w, y + h);
}
