#include <integrity/common/log/log.h>
#include <integrity/common/sound_system.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define AL_ALEXT_PROTOTYPES 1
#define ALC_EXT_EFX 1
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

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

ALCdevice *device;

ALCcontext *context;
ALuint buffer;
ALfloat listenerOri[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
ALboolean loop = AL_FALSE;
ALCenum error;
ALint source_state;

static void list_audio_devices(const ALCchar *devices) {
  const ALCchar *device_str = devices, *next = devices + 1;
  size_t len = 0;

  log_trace("Devices list:\n");
  log_trace("----------\n");
  while (device_str && *device_str != '\0' && next && *next != '\0') {
    log_trace("%s\n", device_str);
    len = strlen(device_str);
    device_str += (len + 1);
    next += (len + 2);
  }
  log_trace("----------\n");
}

#define TEST_ERROR(_msg)      \
  error = alGetError();       \
  if (error != AL_NO_ERROR) { \
    log_error(_msg);          \
    return -1;                \
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

/* This function gets called by pspaudiolib every time the
    audio buffer needs to be filled. The sample format is
    16-bit, stereo.
 */
#ifdef PSP
void audioCallback_OPENAL(void *buf, unsigned int length, void *userdata) {
  (void)userdata;
  // alcRenderSamplesSOFT(device, buf, length);

  // memcpy(destination, first, (last - first) * sizeof(Sample));
}
#endif

static void dc_sound_init(void) {
  // pspAudioInit();

  // pspAudioSetChannelCallback(0, audioCallback_OPENAL, NULL);
}

static void dc_sound_destroy(void) {
  // Clear the channel callback.
  // pspAudioSetChannelCallback(0, 0, 0);

  // Stop the audio system?
  // pspAudioEndPre();

  // Insert a false delay so the thread can be cleaned up.
  // sceKernelDelayThread(50 * 1000);

  // Shut down the audio system.
  // pspAudioEnd();
}

int create_sound(ALuint *source, const char *filename) {
  ALvoid *data;
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
    printf("failed to read wave file\n");
    return -1;
  }

  ret = WaveSeekFile(0, wave);
  if (ret) {
    printf("failed to seek wave file\n");
    return -1;
  }

  bufferData = malloc(wave->dataSize);
  if (!bufferData) {
    perror("malloc");
    return -1;
  }

  ret = WaveReadFile(bufferData, wave->dataSize, wave);
  if (ret != wave->dataSize) {
    printf("short read: %d, want: %d\n", ret, wave->dataSize);
    return -1;
  }

  alBufferData(buffer, to_al_format(wave->channels, wave->bitsPerSample), bufferData, wave->dataSize, wave->sampleRate);
  TEST_ERROR("failed to load buffer data");
#elif defined(WAVLOAD)
  if (!LoadWAVFile(FS_ResolvePathTemp((char *)filename), &format, &data, &size, &freq)) {
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

  return 1;
}

int SYS_SND_Setup(void) {
  ALboolean enumeration;
  const ALCchar *defaultDeviceName = '\0';

  enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
  if (enumeration == AL_FALSE)
    fprintf(stderr, "enumeration extension not available\n");

  list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

  if (!defaultDeviceName)
    defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

  device = alcOpenDevice(defaultDeviceName);
  if (!device) {
    fprintf(stderr, "unable to open default device\n");
    return -1;
  }

  log_trace("Device: %s\n", alcGetString(device, ALC_DEVICE_SPECIFIER));

  alGetError();
  int attribs[] = {ALC_FORMAT_CHANNELS_SOFT, ALC_STEREO_SOFT, ALC_FORMAT_TYPE_SOFT, ALC_SHORT_SOFT, ALC_FREQUENCY, 44100};
  context = alcCreateContext(device, attribs);
  if (!alcMakeContextCurrent(context)) {
    log_error("failed to make default context\n");
    return -1;
  }
  TEST_ERROR("make default context");

  /* setup our psp side of things */
  dc_sound_init();

  /* set orientation */
  alListener3f(AL_POSITION, 0, 0, 1.0f);
  TEST_ERROR("listener position");
  alListener3f(AL_VELOCITY, 0, 0, 0);
  TEST_ERROR("listener velocity");
  alListenerfv(AL_ORIENTATION, listenerOri);
  TEST_ERROR("listener orientation");

  /*
  alGenSources((ALuint)1, &s_jump);
  TEST_ERROR("source generation");

  alSourcef(s_jump, AL_PITCH, 1);
  TEST_ERROR("source pitch");
  alSourcef(s_jump, AL_GAIN, 0.5f);
  TEST_ERROR("source gain");
  alSource3f(s_jump, AL_POSITION, 0, 0, 0);
  TEST_ERROR("source position");
  alSource3f(s_jump, AL_VELOCITY, 0, 0, 0);
  TEST_ERROR("source velocity");
  alSourcei(s_jump, AL_LOOPING, AL_FALSE);
  TEST_ERROR("source looping");

  alGenBuffers(1, &buffer);
  TEST_ERROR("buffer generation");

  TEST_ERROR("loading wav file");

  alBufferData(buffer, format, data, size, freq);
  TEST_ERROR("buffer copy");

  alSourcei(source, AL_BUFFER, buffer);
  TEST_ERROR("buffer binding");


  alSourcei(s_jump, AL_BUFFER, buffer);
  TEST_ERROR("buffer binding");
  */

  return 0;
}

void SND_Play(ALuint source) {
  /* Not working at the moment */
#if 0
    alSourcePlay(source);
    thd_pass();
#endif
  (void)source;
}

int SYS_SND_Destroy(void) {
  /*OpenAL Teardown */

  /* exit context */
  alDeleteBuffers(1, &buffer);
  device = alcGetContextsDevice(context);
  alcMakeContextCurrent(NULL);
  alcDestroyContext(context);
  alcCloseDevice(device);

  /*DCs Specifics */
  dc_sound_destroy();
  return 0;
}
