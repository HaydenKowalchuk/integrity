#pragma once

/* Setup current targets that support sound with OpenAL */
#if defined(__linux__) || defined(__APPLE__)
#define SOUND (1)
#endif
#if defined(WINDOWS)
#define SOUND (1)
#endif
#if defined(PSP)
/* Not yet */
#endif
#if defined(_arch_dreamcast)
/* Not yet */
#endif

#if SOUND
#include <AL/al.h>
#include <AL/alc.h>
#else
typedef unsigned int ALuint;
#endif

int SYS_SND_Setup(void);
int SYS_SND_Destroy(void);

void SND_Play(ALuint source);
int create_sound(ALuint *source, const char *filename);
