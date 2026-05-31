#include <integrity/common/sound_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if SOUND
#include <miniaudio.h>

#define MAX_SOUNDS 64

typedef struct {
  char filename[256];
  int used;
} sound_slot;

static sound_slot sound_table[MAX_SOUNDS];

static struct {
  ma_decoder decoder;
  int active;
} current_sound;

static ma_context g_context;
static ma_device g_device;
static int g_initialized = 0;

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
  (void)pInput;

  ma_format format = ma_device_get_format(pDevice, ma_device_type_playback);
  ma_uint32 channels = ma_device_get_channels(pDevice, ma_device_type_playback);
  ma_uint32 bpf = ma_get_bytes_per_frame(format, channels);

  if (current_sound.active) {
    ma_uint64 framesRead = ma_decoder_read_pcm_frames(&current_sound.decoder, pOutput, frameCount, NULL);
    if (framesRead < frameCount) {
      memset((ma_uint8*)pOutput + framesRead * bpf, 0, (size_t)(frameCount - framesRead) * bpf);
      if (framesRead == 0) {
        ma_decoder_uninit(&current_sound.decoder);
        current_sound.active = 0;
      }
    }
    ma_apply_volume_factor_pcm_frames(pOutput, frameCount, format, channels, 0.3f);
  } else {
    memset(pOutput, 0, (size_t)frameCount * bpf);
  }
}

int SYS_SND_Setup(void) {
  fprintf(stdout, "[audio] Dreamcast: miniaudio dev-0.12\n");

  memset(sound_table, 0, sizeof(sound_table));
  current_sound.active = 0;

  ma_context_config contextConfig = ma_context_config_init();

  ma_result result = ma_context_init(NULL, 0, &contextConfig, &g_context);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "[audio] ma_context_init failed (err=%d)\n", result);
    return -1;
  }

  ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.threadingMode = MA_THREADING_MODE_SINGLE_THREADED;
  deviceConfig.playback.format = ma_format_s16;
  deviceConfig.playback.channels = 2;
  deviceConfig.sampleRate = 44100;
  deviceConfig.periodSizeInFrames = 2048;
  deviceConfig.noFixedSizedCallback = MA_TRUE;
  deviceConfig.dataCallback = data_callback;
  deviceConfig.pUserData = NULL;

  result = ma_device_init(&g_context, &deviceConfig, &g_device);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "[audio] ma_device_init failed (err=%d)\n", result);
    ma_context_uninit(&g_context);
    return -1;
  }

  result = ma_device_start(&g_device);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "[audio] ma_device_start failed (err=%d)\n", result);
    ma_device_uninit(&g_device);
    ma_context_uninit(&g_context);
    return -1;
  }

  g_initialized = 1;
  fprintf(stdout, "[audio] Device started\n");
  return 0;
}

void SND_Update(void) {
  if (g_initialized) {
    ma_device_step(&g_device, MA_BLOCKING_MODE_NON_BLOCKING);
  }
}

int SYS_SND_Destroy(void) {
  fprintf(stdout, "[audio] Shutting down\n");

  if (g_initialized) {
    if (current_sound.active) {
      ma_decoder_uninit(&current_sound.decoder);
      current_sound.active = 0;
    }
    ma_device_uninit(&g_device);
    g_initialized = 0;
  }

  return 0;
}

int create_sound(unsigned int* source, const char* filename) {
  if (!source || !filename) return -1;

  FILE* f = fopen(filename, "rb");
  if (!f) return -1;
  fclose(f);

  int slot = -1;
  for (int i = 0; i < MAX_SOUNDS; i++) {
    if (!sound_table[i].used) { slot = i; break; }
  }
  if (slot < 0) return -1;

  strncpy(sound_table[slot].filename, filename, sizeof(sound_table[slot].filename) - 1);
  sound_table[slot].filename[sizeof(sound_table[slot].filename) - 1] = '\0';
  sound_table[slot].used = 1;

  *source = (unsigned int)slot;
  return 1;
}

void SND_Play(unsigned int source) {
  if (source >= MAX_SOUNDS || !sound_table[source].used) return;

  if (current_sound.active) {
    ma_decoder_uninit(&current_sound.decoder);
    current_sound.active = 0;
  }

  ma_decoder_config cfg = ma_decoder_config_init(ma_format_s16, 2, 44100);
  if (ma_decoder_init_file(sound_table[source].filename, &cfg, &current_sound.decoder) == MA_SUCCESS) {
    current_sound.active = 1;
  }
}
#else
int SYS_SND_Setup(void) { return 0; }
void SND_Update(void) {}
int SYS_SND_Destroy(void) { return 0; }
int create_sound(unsigned int* source, const char* filename) { (void)source; (void)filename; return -1; }
void SND_Play(unsigned int source) { (void)source; }
#endif
