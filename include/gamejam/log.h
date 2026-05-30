#pragma once

#include <stdio.h>

#ifndef GAMEJAM_LOG_LEVEL
#define GAMEJAM_LOG_LEVEL 0
#endif

#ifndef GAMEJAM_LOG_GROUP
#define GAMEJAM_LOG_GROUP "debug"
#endif

enum {
  GAMEJAM_LOG_LEVEL_TRACE = 0,
  GAMEJAM_LOG_LEVEL_DEBUG = 1,
  GAMEJAM_LOG_LEVEL_INFO = 2,
  GAMEJAM_LOG_LEVEL_WARN = 3,
  GAMEJAM_LOG_LEVEL_ERROR = 4,
  GAMEJAM_LOG_LEVEL_NONE = 5,
};

#define standardout stdout
#define standarderr stderr
#if defined(__PSP__)
#undef standarderr
#define standarderr stdout
#endif

#if GAMEJAM_LOG_LEVEL <= GAMEJAM_LOG_LEVEL_TRACE
#define GAMEJAM_LOG_TRACE(fmt, ...) \
  fprintf(standardout, "[" GAMEJAM_LOG_GROUP "] TRACE: " fmt "\n", ##__VA_ARGS__)
#else
#define GAMEJAM_LOG_TRACE(...) ((void)0)
#endif

#if GAMEJAM_LOG_LEVEL <= GAMEJAM_LOG_LEVEL_DEBUG
#define GAMEJAM_LOG_DEBUG(fmt, ...) \
  fprintf(standardout, "[" GAMEJAM_LOG_GROUP "] DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define GAMEJAM_LOG_DEBUG(...) ((void)0)
#endif

#if GAMEJAM_LOG_LEVEL <= GAMEJAM_LOG_LEVEL_INFO
#define GAMEJAM_LOG_INFO(fmt, ...) \
  fprintf(standardout, "[" GAMEJAM_LOG_GROUP "] INFO: " fmt "\n", ##__VA_ARGS__)
#else
#define GAMEJAM_LOG_INFO(...) ((void)0)
#endif

#if GAMEJAM_LOG_LEVEL <= GAMEJAM_LOG_LEVEL_WARN
#define GAMEJAM_LOG_WARN(fmt, ...) \
  fprintf(standarderr, "[" GAMEJAM_LOG_GROUP "] WARNING: " fmt "\n", ##__VA_ARGS__)
#else
#define GAMEJAM_LOG_WARN(...) ((void)0)
#endif

#if GAMEJAM_LOG_LEVEL <= GAMEJAM_LOG_LEVEL_ERROR
#define GAMEJAM_LOG_ERROR(fmt, ...) \
  fprintf(standarderr, "[" GAMEJAM_LOG_GROUP "] ERROR: " fmt "\n", ##__VA_ARGS__)
#else
#define GAMEJAM_LOG_ERROR(...) ((void)0)
#endif
