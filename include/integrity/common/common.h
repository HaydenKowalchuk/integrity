#pragma once

#if defined(_arch_dreamcast)
#include <dc/fmath.h>
#include <integrity/dreamcast/cygprofile.h>
#include <kos.h>
#endif

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__
#include <strings.h>
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#ifdef NO_RESTRICT
#define __restrict
#else
#endif

#include "types.h"

/* Forward declaration to avoid circular dependency */
struct scene;

#include <integrity/common/file_access.h>
#include <integrity/common/math_headers.h>
#include <integrity/scene/scene.h>

#if defined(__linux__) || defined(_arch_dreamcast) || defined(__APPLE__)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

/* atof() breaks under dc if the string is 10 or more characters */
extern float dc_safe_atof(const char* str);
#define atof(x) _Pragma("GCC error \"atof is broken under dreamcast, use dc_safe_atof()\"")

#define FULLSCREEN 1
#define WINDOWED 0
#define WINDOW_TITLE(text, fullscreen) \
  const char* window_title = text;     \
  int _FULLSCREEN = fullscreen;

extern const char* window_title;
extern int _FULLSCREEN;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define Q_CIRCLE (M_PI / 2)
#define SX_CIRCLE (M_PI / 4)
#define DEG2RAD(x) ((x) * M_PI / 180)
#define RAD2DEG(x) ((x) * 180 / M_PI)

/* Test for GCC > 5.0.0 */
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if GCC_VERSION > 50000
#define FALLTHROUGH __attribute__((fallthrough));
#else
#define FALLTHROUGH ;
#endif

// Per system functions
extern float Sys_FloatTime(void);
extern unsigned int Sys_Frames(void);
extern void Sys_Quit(void);
extern bool Sys_IsFullscreen(void);
extern void Sys_SetFullscreen(bool fullscreen);

extern void Game_InputHandler(char c);
extern void Host_Input(float time);

int SYS_SND_Destroy(void);
int SYS_SND_Setup(void);

#if defined(PSP) && defined(DEBUG)
#endif
