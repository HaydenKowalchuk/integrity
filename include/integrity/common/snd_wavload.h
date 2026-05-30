#pragma once

#include <integrity/common/common.h>

#if SOUND
#define AL_ALEXT_PROTOTYPES 1
#define ALC_EXT_EFX 1
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

ALboolean LoadWAVFile(const char *filename, ALenum *format, ALvoid **data, ALsizei *size, ALsizei *freq);
#endif
