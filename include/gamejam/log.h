#ifndef GAMEJAM_LOG_H
#define GAMEJAM_LOG_H

#include <stdio.h>

#define GAMEJAM_LOG_LEVEL_TRACE (0)
#define GAMEJAM_LOG_LEVEL_DEBUG (1)
#define GAMEJAM_LOG_LEVEL_INFO (2)
#define GAMEJAM_LOG_LEVEL_WARN (3)
#define GAMEJAM_LOG_LEVEL_ERROR (4)
#define GAMEJAM_LOG_LEVEL_NONE (5)

#ifndef GAMEJAM_LOG_LEVEL
#define GAMEJAM_LOG_LEVEL (GAMEJAM_LOG_LEVEL_NONE)
#endif

#ifndef GAMEJAM_LOG_GROUP
#define GAMEJAM_LOG_GROUP "debug"
#endif

#if GAMEJAM_LOG_LEVEL <= GAMEJAM_LOG_LEVEL_TRACE
#define GAMEJAM_LOG_TRACE(...)                                      \
  do {                                                              \
    fprintf(stdout, "[" GAMEJAM_LOG_GROUP "] TRACE: " __VA_ARGS__); \
    fprintf(stdout, "\n");                                          \
  } while (0)
#else
#define GAMEJAM_LOG_TRACE(...) ((void)0)
#endif

#if GAMEJAM_LOG_LEVEL <= GAMEJAM_LOG_LEVEL_DEBUG
#define GAMEJAM_LOG_DEBUG(...)                                      \
  do {                                                              \
    fprintf(stdout, "[" GAMEJAM_LOG_GROUP "] DEBUG: " __VA_ARGS__); \
    fprintf(stdout, "\n");                                          \
  } while (0)
#else
#define GAMEJAM_LOG_DEBUG(...) ((void)0)
#endif

#if GAMEJAM_LOG_LEVEL <= GAMEJAM_LOG_LEVEL_INFO
#define GAMEJAM_LOG_INFO(...)                                      \
  do {                                                             \
    fprintf(stdout, "[" GAMEJAM_LOG_GROUP "] INFO: " __VA_ARGS__); \
    fprintf(stdout, "\n");                                         \
  } while (0)
#else
#define GAMEJAM_LOG_INFO(...) ((void)0)
#endif

#if GAMEJAM_LOG_LEVEL <= GAMEJAM_LOG_LEVEL_WARN
#define GAMEJAM_LOG_WARN(...)                                         \
  do {                                                                \
    fprintf(stderr, "[" GAMEJAM_LOG_GROUP "] WARNING: " __VA_ARGS__); \
    fprintf(stderr, "\n");                                            \
  } while (0)
#else
#define GAMEJAM_LOG_WARN(...) ((void)0)
#endif

#if GAMEJAM_LOG_LEVEL <= GAMEJAM_LOG_LEVEL_ERROR
#define GAMEJAM_LOG_ERROR(...)                                      \
  do {                                                              \
    fprintf(stderr, "[" GAMEJAM_LOG_GROUP "] ERROR: " __VA_ARGS__); \
    fprintf(stderr, "\n");                                          \
  } while (0)
#else
#define GAMEJAM_LOG_ERROR(...) ((void)0)
#endif

#endif
