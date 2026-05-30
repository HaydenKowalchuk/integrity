#pragma once

/* Setup current targets that support sound */
#if defined(__linux__) || defined(__APPLE__)
#define SOUND (1)
#endif
#if defined(_WIN32)
#define SOUND (1)
#endif
#if defined(PSP)
/* Not yet */
#define SOUND (0)
#endif
#if defined(_arch_dreamcast)
/* Not yet */
#endif

int SYS_SND_Setup(void);
int SYS_SND_Destroy(void);

void SND_Play(unsigned int source);
int create_sound(unsigned int* source, const char* filename);
