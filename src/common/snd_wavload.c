#include <integrity/common/snd_wavload.h>

#if SOUND
static bool is_big_endian(void) {
  int a = 1;
  return !((char *)&a)[0];
}

static inline unsigned long swap_32bit(unsigned long ul) {
  return (unsigned long)(((ul & 0xFF000000) >> 24) |
                         ((ul & 0x00FF0000) >> 8) |
                         ((ul & 0x0000FF00) << 8) |
                         ((ul & 0x000000FF) << 24));
}

static int convert_to_int(char *buffer, int len) {
  /*
  int i = 0;
  int a = 0;
  if (!is_big_endian())
    for (; i < len; i++)
      ((char *)&a)[i] = buffer[i];
  else
    for (; i < len; i++)
      ((char *)&a)[3 - i] = buffer[i];
  */
  (void)len;
  int a;

  memcpy(&a, buffer, sizeof(a));
  if (is_big_endian()) {
    a = swap_32bit(a);
  }
  return a;
}

ALboolean LoadWAVFile(const char *filename, ALenum *format, ALvoid **data, ALsizei *size, ALsizei *freq) {
  char buffer[4];
  int read = 0;

  FILE *in = fopen(filename, "rb");
  if (!in) {
#ifdef DEBUG
    fprintf(stderr, "ERROR: for path(%s)\n", filename);
#endif
    return AL_FALSE;
  }

  read += fread(buffer, 4, sizeof(char), in);

  if (strncmp(buffer, "RIFF", 4) != 0) {
#ifdef DEBUG
    fprintf(stderr, "ERROR: Not a valid wave file for path(%s)\n", filename);
#endif
    fclose(in);
    return AL_FALSE;
  }

  read += fread(buffer, 4, sizeof(char), in);
  read += fread(buffer, 4, sizeof(char), in);  //WAVE
  read += fread(buffer, 4, sizeof(char), in);  //fmt
  read += fread(buffer, 4, sizeof(char), in);  //16
  read += fread(buffer, 2, sizeof(char), in);  //1
  read += fread(buffer, 2, sizeof(char), in);

  int chan = convert_to_int(buffer, 2);
  read += fread(buffer, 4, sizeof(char), in);
  *freq = convert_to_int(buffer, 4);
  read += fread(buffer, 4, sizeof(char), in);
  read += fread(buffer, 2, sizeof(char), in);
  read += fread(buffer, 2, sizeof(char), in);
  int bps = convert_to_int(buffer, 2);
  read += fread(buffer, 4, sizeof(char), in);  //data
  read += fread(buffer, 4, sizeof(char), in);
  *size = (ALsizei)convert_to_int(buffer, 4);
  *data = (ALvoid *)malloc(*size * sizeof(char));
  read += fread(*data, *size, sizeof(char), in);

  if (chan == 1) {
    *format = (bps == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
  } else {
    *format = (bps == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
  }

  fclose(in);
  return AL_TRUE;
}
#endif
