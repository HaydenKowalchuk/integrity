/*
 * OpenAL example
 *
 * Copyright(C) Florian Fainelli <f.fainelli@gmail.com>
 */

#include <integrity/common/sound_system.h>

#if SOUND
#include <AL/al.h>
#include <AL/alc.h>
#else
typedef unsigned int ALuint;
#endif

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if SOUND
#define WAVLOAD

#ifdef LIBAUDIO
#include <audio/wave.h>
#define BACKEND "libaudio"
#elif defined(WAVLOAD)
#include <integrity/common/snd_wavload.h>
#define BACKEND "wavload"
#else
#include <AL/alut.h>
#define BACKEND "alut"
#endif

static void list_audio_devices(const ALCchar* devices) {
  const ALCchar *device = devices, *next = devices + 1;
  size_t len = 0;

  fprintf(stdout, "Devices list:\n");
  fprintf(stdout, "----------\n");
  while (device && *device != '\0' && next && *next != '\0') {
    fprintf(stdout, "%s\n", device);
    len = strlen(device);
    device += (len + 1);
    next += (len + 2);
  }
  fprintf(stdout, "----------\n");
}

#define TEST_ERROR(_msg)        \
  error = alGetError();         \
  if (error != AL_NO_ERROR) {   \
    fprintf(stderr, _msg "\n"); \
    return -1;                  \
  }

static inline ALenum to_al_format(short channels, short samples) {
  bool stereo = (channels > 1);

  switch (samples) {
    case 16:
      if (stereo)
        return AL_FORMAT_STEREO16;
      else
        return AL_FORMAT_MONO16;
    case 8:
      if (stereo)
        return AL_FORMAT_STEREO8;
      else
        return AL_FORMAT_MONO8;
    default:
      return -1;
  }
}
#endif

#if SOUND
ALCdevice* device;

ALCcontext* context;
ALuint buffer;
ALfloat listenerOri[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
ALboolean loop = AL_FALSE;
ALCenum error;
ALint source_state;
#endif

int create_sound(ALuint* source, const char* filename) {
#if SOUND
  ALvoid* data;
  ALsizei size, freq;
  ALenum format;

  alGenSources((ALuint)1, source);
  TEST_ERROR("source generation");

  alSourcef(*source, AL_PITCH, 1);
  TEST_ERROR("source pitch");
  alSourcef(*source, AL_GAIN, 1);
  TEST_ERROR("source gain");
  alSource3f(*source, AL_POSITION, 0, 0, 0);
  TEST_ERROR("source position");
  alSource3f(*source, AL_VELOCITY, 0, 0, 0);
  TEST_ERROR("source velocity");
  alSourcei(*source, AL_LOOPING, AL_FALSE);
  TEST_ERROR("source looping");

  alGenBuffers(1, &buffer);
  TEST_ERROR("buffer generation");
#ifdef LIBAUDIO
  /* load data */
  wave = WaveOpenFileForReading("test.wav");
  if (!wave) {
    fprintf(stderr, "failed to read wave file\n");
    return -1;
  }

  ret = WaveSeekFile(0, wave);
  if (ret) {
    fprintf(stderr, "failed to seek wave file\n");
    return -1;
  }

  bufferData = malloc(wave->dataSize);
  if (!bufferData) {
    perror("malloc");
    return -1;
  }

  ret = WaveReadFile(bufferData, wave->dataSize, wave);
  if (ret != wave->dataSize) {
    fprintf(stderr, "short read: %d, want: %d\n", ret, wave->dataSize);
    return -1;
  }

  alBufferData(buffer, to_al_format(wave->channels, wave->bitsPerSample), bufferData, wave->dataSize, wave->sampleRate);
  TEST_ERROR("failed to load buffer data");
#elif defined(WAVLOAD)
  if (!LoadWAVFile(FS_ResolvePathTemp(filename), &format, &data, &size, &freq)) {
    return -1;
  }
#else
  alutLoadWAVFile(filename, &format, &data, &size, &freq, &loop);
  TEST_ERROR("loading wav file");
#endif

  alBufferData(buffer, format, data, size, freq);
  TEST_ERROR("buffer copy");

  alSourcei(*source, AL_BUFFER, buffer);
  TEST_ERROR("buffer binding");
#else
  (void)source;
  (void)filename;
#endif
  return 1;
}

int SYS_SND_Setup(void) {
#if SOUND
  ALboolean enumeration;
  // const ALCchar *devices;
#ifdef LIBAUDIO
  int ret;
  WaveInfo* wave;
  char* bufferData;
#endif

  fprintf(stdout, "Using " BACKEND " as audio backend\n");

  enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
  if (enumeration == AL_FALSE)
    fprintf(stderr, "enumeration extension not available\n");

  list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

  const ALCchar* defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

  device = alcOpenDevice(defaultDeviceName);
  if (!device) {
    fprintf(stderr, "unable to open default device\n");
    return -1;
  }

  fprintf(stdout, "Device: %s\n", alcGetString(device, ALC_DEVICE_SPECIFIER));

  alGetError();

  context = alcCreateContext(device, NULL);
  if (!alcMakeContextCurrent(context)) {
    fprintf(stderr, "failed to make default context\n");
    return -1;
  }
  TEST_ERROR("make default context");

  /* set orientation */
  alListener3f(AL_POSITION, 0, 0, 1.0f);
  TEST_ERROR("listener position");
  alListener3f(AL_VELOCITY, 0, 0, 0);
  TEST_ERROR("listener velocity");
  alListenerfv(AL_ORIENTATION, listenerOri);
  TEST_ERROR("listener orientation");
#endif

  return 0;
}

void __attribute__((unused)) SND_Play(ALuint source) {
#ifdef SOUND
  alSourcePlay(source);
#else
  (void)source;
#endif
}

int SYS_SND_Destroy(void) {
#ifdef SOUND
  /* exit context */
  alDeleteBuffers(1, &buffer);
  device = alcGetContextsDevice(context);
  alcMakeContextCurrent(NULL);
  alcDestroyContext(context);
  alcCloseDevice(device);
#endif
  return 0;
}
