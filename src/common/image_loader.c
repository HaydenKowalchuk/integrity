#include <integrity/common/image_loader.h>
#include <integrity/common/renderer.h>
#include <integrity/common/resource_manager.h>

#define GAMEJAM_LOG_GROUP "image_loader"
#define GAMEJAM_LOG_LEVEL (0)
#include <gamejam/log.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_JPEG

#include <stb_image.h>

static tx_image *IMG_load_internal(const char *path) {
  GAMEJAM_LOG_DEBUG("%s called with %s", __func__, path);
  tx_image *img = NULL;
  uint32_t crc = 0;
  if (path == NULL) {
    img = (tx_image *)calloc(1, sizeof(tx_image));
    GAMEJAM_LOG_ERROR("missing path");
    return img;
  }
  int length = strlen(path);
  crc32(&crc, (uint8_t *)path, length);

  if (resource_object_find(crc) == -1) {
    img = (tx_image *)calloc(1, sizeof(tx_image));

    /* Check if file even exists */
    if (!FS_FileExists(path)) {
      GAMEJAM_LOG_ERROR("path not exist %s", path);
      return img;
    }
    GAMEJAM_LOG_DEBUG("calling stbi load");

    size_t path_bytes = (length > 16) ? 16 : length;
    const char *src_start = path + (length - path_bytes);
    memset(img->name, 0, sizeof(img->name));
    memcpy(img->name, src_start, path_bytes);
    img->name[path_bytes] = '\0';
    img->crc = crc;
    img->id = 0;

    img->data = stbi_load(path,
                          &img->width,
                          &img->height,
                          &img->channels,
                          0);  // maybe should be rgb
    // const char *stbi_reason = stbi_failure_reason();
    if (img->data == NULL) {
      GAMEJAM_LOG_ERROR("ERROR: for path(%s)", path);
      return img;
    }

    GAMEJAM_LOG_DEBUG("Adding new Image! crc: %08X, %s", (unsigned int)crc, path);

    resource_object_add('i', crc, img);
    return img;
  } else {
    GAMEJAM_LOG_DEBUG("Found already loaded Image! crc: %08X, %s\n", (unsigned int)crc, path);
    resource_object_add('i', crc, img);
    return (tx_image *)(resource_object_pointer(crc));
  }
}

tx_image *IMG_load_from_memory(const unsigned char *buffer, int len) {
  tx_image *img = NULL;

  uint32_t crc = 0;
  crc32(&crc, (uint8_t *)&buffer, 4);

  if (resource_object_find(crc) == -1) {
    img = (tx_image *)malloc(sizeof(tx_image));
    memset(img, 0, sizeof(tx_image));
    snprintf(img->name, 16, "0x%08" PRIXPTR, (void *)buffer);

    GAMEJAM_LOG_DEBUG("%s", img->name);

    img->name[15] = '\0';
    img->crc = crc;
    // stbi_set_flip_vertically_on_load(false);  // maybe true?
    img->data = stbi_load_from_memory((const unsigned char *)buffer,
                                      len,
                                      &img->width,
                                      &img->height,
                                      &img->channels,
                                      0);  // maybe should be rgb, eg. (_32bit ? 4 : 3))
    // const char *stbi_reason = stbi_failure_reason();
    if (img->data == NULL) {
      GAMEJAM_LOG_ERROR("ERROR: for memory loaded image (0x%08" PRIXPTR ")", (uintptr_t)buffer);

      return img;
    }

    GAMEJAM_LOG_DEBUG("Adding new Image! crc: %08X, 0x%08" PRIXPTR "", (unsigned int)crc, (uintptr_t)buffer);

    resource_object_add('i', crc, img);
    return img;
  } else {
    GAMEJAM_LOG_DEBUG("Found already loaded Image! crc:  %08X, 0x%08" PRIXPTR "", (unsigned int)crc, (uintptr_t)buffer);

    resource_object_add('i', crc, NULL);
    return (tx_image *)(resource_object_pointer(crc));
  }
}

tx_image *IMG_load(const char *path) {
  return IMG_load_internal(FS_ResolvePathTemp(path));
}

tx_image *IMG_load_boolean(const char *path, bool transform) {
  return IMG_load_internal((transform) ? FS_ResolvePathTemp(path) : path);
}

void IMG_unload(tx_image *img) {
  if ((img != NULL) && (img->data != NULL)) {
    stbi_image_free(img->data);
    img->data = NULL;
  }
}

void IMG_destroy(tx_image **img) {
  GAMEJAM_LOG_DEBUG("%s called ", __func__);
  if ((*img) != NULL) {
    IMG_unload((*img));

    if ((*img) != NULL) {
      if (glIsTexture((*img)->id)) {
        glDeleteTextures(1, &(*img)->id);
      }

      memset((*img), 0, sizeof(tx_image));
      free(*img);
    }
  }
  *img = NULL;
}

sprite IMG_create_sprite(tx_image *img, int x, int y, int x2, int y2) {
  sprite spr = (sprite){.parent = img,
                        .u = (float)(x / (float)img->width),
                        .v = (float)(y / (float)img->height),
                        .width = (float)((x2 - x) / (float)img->width),
                        .height = (float)((y2 - y) / (float)img->height),
                        .i_width = x2 - x,
                        .i_height = y2 - y};

  return spr;
}

sprite IMG_create_sprite_scaled(tx_image *img, int x, int y, int x2, int y2, float scale) {
  return IMG_create_sprite(img, (int)(x * scale), (int)(y * scale), (int)(x2 * scale), (int)(y2 * scale));
}
