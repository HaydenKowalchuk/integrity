#pragma once

#include <math.h>

#ifdef PSP
#include <pspmath.h>
#define SIN(x) sinf(x)
#define COS(x) cosf(x)
#define SQRT(x) sqrtf(x)
#endif
#ifdef _arch_dreamcast
#include <dc/fmath.h>
#define SIN(x) fsin(x)
#define COS(x) fcos(x)
#define SQRT(x) fsqrt(x)
#endif
#if defined(WINDOWS) || defined(__linux__) || defined(__APPLE__)
#define SIN(x) sinf(x)
#define COS(x) cosf(x)
#define SQRT(x) sqrtf(x)
#endif

#include <cglm/cglm.h>
