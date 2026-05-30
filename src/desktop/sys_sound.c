#include <integrity/common/sound_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if SOUND
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#define MAX_SOUNDS 64

static ma_engine engine;
static struct {
  ma_sound sound;
  int used;
} sound_table[MAX_SOUNDS];
static int engine_initialized = 0;

static void log_error(int result, const char* msg) {
  if (result != MA_SUCCESS) {
    fprintf(stderr, "%s: %s\n", msg, ma_result_description(result));
  }
}

int SYS_SND_Setup(void) {
  ma_engine_config config = ma_engine_config_init();

  fprintf(stdout, "Using miniaudio as audio backend\n");

  ma_result result = ma_engine_init(&config, &engine);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "Failed to initialize miniaudio engine: %s\n", ma_result_description(result));
    return -1;
  }

  engine_initialized = 1;
  memset(sound_table, 0, sizeof(sound_table));
  return 0;
}

int SYS_SND_Destroy(void) {
  if (!engine_initialized) return 0;

  for (int i = 0; i < MAX_SOUNDS; i++) {
    if (sound_table[i].used) {
      ma_sound_uninit(&sound_table[i].sound);
      sound_table[i].used = 0;
    }
  }

  ma_engine_uninit(&engine);
  engine_initialized = 0;
  return 0;
}

int create_sound(unsigned int* source, const char* filename) {
  if (!engine_initialized || !source || !filename) return -1;

  int slot = -1;
  for (int i = 0; i < MAX_SOUNDS; i++) {
    if (!sound_table[i].used) {
      slot = i;
      break;
    }
  }

  if (slot < 0) {
    fprintf(stderr, "create_sound: no free sound slots\n");
    return -1;
  }

  ma_result result = ma_sound_init_from_file(&engine, filename, 0, NULL, NULL, &sound_table[slot].sound);
  if (result != MA_SUCCESS) {
    log_error(result, "Failed to load sound file");
    return -1;
  }

  sound_table[slot].used = 1;
  *source = (unsigned int)slot;
  return 1;
}

void SND_Play(unsigned int source) {
  if (!engine_initialized) return;

  if (source >= MAX_SOUNDS || !sound_table[source].used) {
    fprintf(stderr, "SND_Play: invalid source handle %u\n", source);
    return;
  }

  ma_sound_start(&sound_table[source].sound);
}
#else
int SYS_SND_Setup(void) {
  return 0;
}
int SYS_SND_Destroy(void) {
  return 0;
}
int create_sound(unsigned int* source, const char* filename) {
  (void)source;
  (void)filename;
  return -1;
}
void SND_Play(unsigned int source) {
  (void)source;
}
#endif
