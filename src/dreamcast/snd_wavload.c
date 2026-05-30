
#include <integrity/common/snd_wavload.h>

static bool is_big_endian() {
  int a = 1;
  return !((char*)&a)[0];
}

static int convert_to_int(char* buffer, int len) {
  int i = 0;
  int a = 0;
  if (!is_big_endian())
    for (; i < len; i++)
      ((char*)&a)[i] = buffer[i];
  else
    for (; i < len; i++)
      ((char*)&a)[3 - i] = buffer[i];
  return a;
}

ALboolean LoadWAVFile(const char* filename, ALenum* format, ALvoid** data, ALsizei* size, ALsizei* freq) {
  char buffer[4];

  FILE* in = fopen(filename, "rb");
  if (!in) {
    fprintf(stderr, "Couldn't open file\n");
    return AL_FALSE;
  }

  fread(buffer, 4, sizeof(char), in);

  if (strncmp(buffer, "RIFF", 4) != 0) {
    fprintf(stderr, "Not a valid wave file\n");
    return AL_FALSE;
  }

  fread(buffer, 4, sizeof(char), in);
  fread(buffer, 4, sizeof(char), in);  // WAVE
  fread(buffer, 4, sizeof(char), in);  // fmt
  fread(buffer, 4, sizeof(char), in);  // 16
  fread(buffer, 2, sizeof(char), in);  // 1
  fread(buffer, 2, sizeof(char), in);

  int chan = convert_to_int(buffer, 2);
  fread(buffer, 4, sizeof(char), in);
  *freq = convert_to_int(buffer, 4);
  fread(buffer, 4, sizeof(char), in);
  fread(buffer, 2, sizeof(char), in);
  fread(buffer, 2, sizeof(char), in);
  int bps = convert_to_int(buffer, 2);
  fread(buffer, 4, sizeof(char), in);  // data
  fread(buffer, 4, sizeof(char), in);
  *size = (ALsizei)convert_to_int(buffer, 4);
  *data = (ALvoid*)malloc(*size * sizeof(char));
  fread(*data, *size, sizeof(char), in);

  if (chan == 1) {
    *format = (bps == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
  } else {
    *format = (bps == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
  }

  fclose(in);

  printf("Loaded WAV file!\n");

  return AL_TRUE;
}
