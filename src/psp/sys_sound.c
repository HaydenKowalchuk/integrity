#include <integrity/common/sound_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if SOUND
#include <miniaudio.h>
#include <pspaudio.h>
#include <pspkernel.h>
#include <pspthreadman.h>

#define MAX_SOUNDS 64
#define PSP_AUDIO_CHANNELS 2
#define PSP_AUDIO_FREQUENCY 44100
#define PSP_AUDIO_SAMPLES_DESIRED 1088
#define PSP_AUDIO_NUM_BUFFERS 2

typedef struct {
  ma_int16* pcmData;
  ma_uint64 frameCount;
  ma_uint32 channels;
  ma_uint32 sampleRate;
  int used;
} sound_slot;

static sound_slot sound_table[MAX_SOUNDS];

static int audio_chan = -1;
static int audio_thread_running = 0;
static int audio_thread_exit = 0;
static SceUInt32 audio_thread_id = 0;

static int cur_buf = 0;
static ma_int16 output_buffers[PSP_AUDIO_NUM_BUFFERS][PSP_AUDIO_SAMPLES_DESIRED * PSP_AUDIO_CHANNELS] __attribute__((aligned(64)));

static struct {
  int slot;
  ma_uint64 position;
  int active;
} play_queue[MAX_SOUNDS];
static int play_queue_count = 0;

static int audio_thread(SceSize args, void* argp) {
  (void)args;
  (void)argp;

  fprintf(stdout, "[audio thread] Started\n");

  while (!audio_thread_exit) {
    ma_int16* buf = output_buffers[cur_buf];
    int sampleCount = PSP_AUDIO_SAMPLES_DESIRED;

    memset(buf, 0, sampleCount * PSP_AUDIO_CHANNELS * sizeof(ma_int16));

    int active_count = 0;
    for (int i = 0; i < play_queue_count; i++) {
      if (!play_queue[i].active) continue;
      active_count++;

      sound_slot* slot = &sound_table[play_queue[i].slot];
      if (!slot->used) {
        fprintf(stdout, "[audio thread] Slot %d removed (sound freed)\n", play_queue[i].slot);
        play_queue[i].active = 0;
        continue;
      }

      ma_uint64 remaining = slot->frameCount - play_queue[i].position;
      ma_uint64 toMix = (remaining < (ma_uint64)sampleCount) ? remaining : (ma_uint64)sampleCount;

      if (slot->channels == 1) {
        for (ma_uint64 f = 0; f < toMix; f++) {
          ma_int16 s = slot->pcmData[play_queue[i].position + f];
          buf[f * 2 + 0] += s;
          buf[f * 2 + 1] += s;
        }
      } else {
        for (ma_uint64 f = 0; f < toMix; f++) {
          buf[f * 2 + 0] += slot->pcmData[(play_queue[i].position + f) * 2 + 0];
          buf[f * 2 + 1] += slot->pcmData[(play_queue[i].position + f) * 2 + 1];
        }
      }

      play_queue[i].position += toMix;

      if (play_queue[i].position >= slot->frameCount) {
        fprintf(stdout, "[audio thread] Sound slot %d finished\n", play_queue[i].slot);
        play_queue[i].active = 0;
      }
    }

    sceAudioOutput2OutputBlocking(PSP_AUDIO_VOLUME_MAX, buf);
    cur_buf ^= 1;
  }

  fprintf(stdout, "[audio thread] Exiting\n");
  return 0;
}

int SYS_SND_Setup(void) {
  fprintf(stdout, "[audio] Using PSP SDK audio with miniaudio decoder\n");

  memset(sound_table, 0, sizeof(sound_table));
  memset(play_queue, 0, sizeof(play_queue));
  play_queue_count = 0;

  fprintf(stdout, "[audio] Releasing any prior SRC channel\n");
  while (sceAudioOutput2GetRestSample() > 0)
    sceKernelDelayThread(1000);

  sceAudioSRCChRelease();

  audio_chan = sceAudioSRCChReserve(PSP_AUDIO_SAMPLES_DESIRED, PSP_AUDIO_FREQUENCY, PSP_AUDIO_CHANNELS);
  if (audio_chan < 0) {
    fprintf(stderr, "[audio] Failed to reserve PSP audio SRC channel (err=%d)\n", audio_chan);
    return -1;
  }
  fprintf(stdout, "[audio] Reserved SRC channel ch=%d (%d samples, %dHz, %dch)\n", audio_chan, PSP_AUDIO_SAMPLES_DESIRED, PSP_AUDIO_FREQUENCY, PSP_AUDIO_CHANNELS);

  audio_thread_exit = 0;

  audio_thread_id = sceKernelCreateThread("audio_thread", audio_thread, 0x10, 0x10000, 0, NULL);
  if (audio_thread_id < 0) {
    fprintf(stderr, "[audio] Failed to create audio thread (err=%08x)\n", audio_thread_id);
    sceAudioSRCChRelease();
    audio_chan = -1;
    return -1;
  }

  int ret = sceKernelStartThread(audio_thread_id, 0, NULL);
  if (ret < 0) {
    fprintf(stderr, "[audio] Failed to start audio thread (err=%08x)\n", ret);
    sceKernelDeleteThread(audio_thread_id);
    audio_thread_id = 0;
    sceAudioSRCChRelease();
    audio_chan = -1;
    return -1;
  }

  audio_thread_running = 1;
  fprintf(stdout, "[audio] Audio thread started (id=%d)\n", audio_thread_id);

  return 0;
}

int SYS_SND_Destroy(void) {
  fprintf(stdout, "[audio] Shutting down audio system\n");

  audio_thread_exit = 1;

  if (audio_thread_id > 0) {
    fprintf(stdout, "[audio] Waiting for audio thread to exit...\n");
    sceKernelWaitThreadEnd(audio_thread_id, NULL);
    sceKernelDeleteThread(audio_thread_id);
    audio_thread_id = 0;
  }

  if (audio_chan >= 0) {
    sceAudioSRCChRelease();
    fprintf(stdout, "[audio] Released SRC channel\n");
    audio_chan = -1;
  }

  int freed = 0;
  for (int i = 0; i < MAX_SOUNDS; i++) {
    if (sound_table[i].used) {
      ma_free(sound_table[i].pcmData, NULL);
      sound_table[i].used = 0;
      freed++;
    }
  }
  fprintf(stdout, "[audio] Freed %d sound slots\n", freed);

  audio_thread_running = 0;
  return 0;
}

int create_sound(unsigned int* source, const char* filename) {
  if (!source || !filename) return -1;

  fprintf(stdout, "[audio] Loading sound file: %s\n", filename);

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

  sound_table[slot].used = 0;

  ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_s16, PSP_AUDIO_CHANNELS, PSP_AUDIO_FREQUENCY);
  ma_decoder decoder;

  ma_result result = ma_decoder_init_file(filename, &decoderConfig, &decoder);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "[audio] Failed to open audio file: %s (err=%d)\n", filename, result);
    return -1;
  }

  ma_uint64 totalFrames;
  result = ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "[audio] Failed to get audio length for %s\n", filename);
    ma_decoder_uninit(&decoder);
    return -1;
  }
  fprintf(stdout, "[audio] Decoding %llu frames from %s\n", totalFrames, filename);

  sound_table[slot].pcmData = (ma_int16*)ma_malloc((size_t)(totalFrames * PSP_AUDIO_CHANNELS * sizeof(ma_int16)), NULL);
  if (sound_table[slot].pcmData == NULL) {
    fprintf(stderr, "[audio] Out of memory loading %s (%llu frames)\n", filename, totalFrames);
    ma_decoder_uninit(&decoder);
    return -1;
  }

  ma_uint64 framesRead;
  result = ma_decoder_read_pcm_frames(&decoder, sound_table[slot].pcmData, totalFrames, &framesRead);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "[audio] Failed to read audio frames from %s\n", filename);
    ma_free(sound_table[slot].pcmData, NULL);
    ma_decoder_uninit(&decoder);
    return -1;
  }

  ma_decoder_uninit(&decoder);

  sound_table[slot].frameCount = framesRead;
  sound_table[slot].channels = PSP_AUDIO_CHANNELS;
  sound_table[slot].sampleRate = PSP_AUDIO_FREQUENCY;
  sound_table[slot].used = 1;

  fprintf(stdout, "[audio] Loaded sound slot %d: %llu frames, %s\n", slot, framesRead, filename);

  *source = (unsigned int)slot;
  return 1;
}

void SND_Play(unsigned int source) {
  if (source >= MAX_SOUNDS) {
    fprintf(stderr, "[audio] SND_Play: source %u out of range\n", source);
    return;
  }
  if (!sound_table[source].used) {
    fprintf(stderr, "[audio] SND_Play: sound slot %u not loaded\n", source);
    return;
  }

  for (int i = 0; i < play_queue_count; i++) {
    if (!play_queue[i].active && play_queue[i].slot == (int)source) {
      play_queue[i].position = 0;
      play_queue[i].active = 1;
      fprintf(stdout, "[audio] Playing sound slot %u (reused entry %d)\n", source, i);
      return;
    }
  }

  for (int i = 0; i < play_queue_count; i++) {
    if (!play_queue[i].active) {
      play_queue[i].slot = (int)source;
      play_queue[i].position = 0;
      play_queue[i].active = 1;
      fprintf(stdout, "[audio] Playing sound slot %u (slot %d)\n", source, i);
      return;
    }
  }

  if (play_queue_count < MAX_SOUNDS) {
    play_queue[play_queue_count].slot = (int)source;
    play_queue[play_queue_count].position = 0;
    play_queue[play_queue_count].active = 1;
    fprintf(stdout, "[audio] Playing sound slot %u (new entry %d)\n", source, play_queue_count);
    play_queue_count++;
  } else {
    fprintf(stderr, "[audio] SND_Play: play queue full, dropping sound %u\n", source);
  }
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
