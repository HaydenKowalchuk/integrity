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
static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
  (void)pDevice;
  (void)pInput;

  if (current_sound.active) {
    ma_uint64 framesRead;
    ma_result res = ma_decoder_read_pcm_frames(&current_sound.decoder, pOutput, frameCount, &framesRead);
    if (res != MA_SUCCESS || framesRead == 0) {
      ma_decoder_uninit(&current_sound.decoder);
      current_sound.active = 0;
    }
  }

  if (!current_sound.active) {
    memset(pOutput, 0, frameCount * 4);
  }
}

int SYS_SND_Setup(void)
{
  fprintf(stdout, "[audio] PSP: miniaudio test\n");

  memset(sound_table, 0, sizeof(sound_table));
  current_sound.active = 0;

  ma_context_config contextConfig = ma_context_config_init();

  ma_result result = ma_context_init(NULL, 0, &contextConfig, &g_context);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "[audio] ma_context_init failed (err=%d)\n", result);
    return -1;
  }

  ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_s16;
  deviceConfig.playback.channels = 2;
  deviceConfig.sampleRate = 44100;
  deviceConfig.periodSizeInFrames = 1470;
  deviceConfig.noFixedSizedCallback = MA_TRUE;
  deviceConfig.dataCallback = data_callback;

  result = ma_device_init(&g_context, &deviceConfig, &g_device);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "[audio] ma_device_init failed (err=%d)\n", result);
    ma_context_uninit(&g_context);
    return -1;
  }
  fprintf(stdout, "[audio] post-init: internalPS=%u format=%d ch=%u sr=%u\n",
    g_device.playback.internalPeriodSizeInFrames,
    g_device.playback.internalFormat,
    g_device.playback.internalChannels,
    g_device.playback.internalSampleRate);

  result = ma_device_start(&g_device);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "[audio] ma_device_start failed (err=%d)\n", result);
    ma_device_uninit(&g_device);
    ma_context_uninit(&g_context);
    return -1;
  }
  fprintf(stdout, "[audio] post-start: internalPS=%u\n",
    g_device.playback.internalPeriodSizeInFrames);

  g_initialized = 1;
  fprintf(stdout, "[audio] Device started\n");
  return 0;
}

void SND_Update(void)
{
  if (g_initialized)
    ma_device_step(&g_device, MA_BLOCKING_MODE_NON_BLOCKING);
}

int SYS_SND_Destroy(void)
{
  if (g_initialized) {
    g_initialized = 0;

    if (current_sound.active) {
      ma_decoder_uninit(&current_sound.decoder);
      current_sound.active = 0;
    }

    ma_device_uninit(&g_device);
    ma_context_uninit(&g_context);
  }
  return 0;
}

int create_sound(unsigned int* source, const char* filename)
{
  if (!source || !filename) return -1;
  int slot = -1;
  for (int i = 0; i < MAX_SOUNDS; i++) {
    if (!sound_table[i].used) { slot = i; break; }
  }
  if (slot < 0) return -1;

  ma_decoder_config cfg = ma_decoder_config_init(ma_format_s16, 2, 44100);
  ma_decoder decoder;
  if (ma_decoder_init_file(filename, &cfg, &decoder) != MA_SUCCESS) return -1;
  ma_decoder_uninit(&decoder);

  strncpy(sound_table[slot].filename, filename, sizeof(sound_table[slot].filename) - 1);
  sound_table[slot].filename[sizeof(sound_table[slot].filename) - 1] = '\0';
  sound_table[slot].used = 1;
  *source = (unsigned int)slot;
  return 1;
}

void SND_Play(unsigned int source)
{
  if (source >= MAX_SOUNDS || !sound_table[source].used) return;
  if (current_sound.active) {
    ma_decoder_uninit(&current_sound.decoder);
    current_sound.active = 0;
  }
  ma_decoder_config cfg = ma_decoder_config_init(ma_format_s16, 2, 44100);
  if (ma_decoder_init_file(sound_table[source].filename, &cfg, &current_sound.decoder) == MA_SUCCESS)
    current_sound.active = 1;
}
#else
int SYS_SND_Setup(void) { return 0; }
void SND_Update(void) { }
int SYS_SND_Destroy(void) { return 0; }
int create_sound(unsigned int* source, const char* filename) { (void)source; (void)filename; return -1; }
void SND_Play(unsigned int source) { (void)source; }
#endif
