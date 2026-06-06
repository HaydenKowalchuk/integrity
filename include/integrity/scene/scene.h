#pragma once

#include <stdint.h>

typedef struct IntegrityScene {
  void (*init)(void);
  void (*exit)(void);
  void (*render2D)(float);
  void (*render3D)(float);
  void (*update)(float);
  int flags;
  void* context;
} IntegrityScene;

#define STARTUP_SCENE(_init, _exit, _render2, _render3, _update, _flags) IntegrityScene scene_start = {_init, _exit, _render2, _render3, _update, _flags, NULL}
#define SCENE(_local, _init, _exit, _render2, _render3, _update, _flags) IntegrityScene _local = {_init, _exit, _render2, _render3, _update, _flags, NULL}

extern IntegrityScene scene_start;

#define SCENE_FALLTHROUGH_RENDER 1
#define SCENE_FALLTHROUGH_UPDATE 2
#define SCENE_BLOCK 4
#define SCENE_NO_REINIT_CHILD 8

IntegrityScene* SCN_ChangeTo(IntegrityScene next);
IntegrityScene* SCN_Push(IntegrityScene next);
IntegrityScene* SCN_Pop(void);
IntegrityScene* SCN_Current(void);
IntegrityScene* SCN_Peek(void);

void SCN_SetContext(void* ctx);
void* SCN_GetContext(void);
void SCN_SetNextScene(IntegrityScene* next);
IntegrityScene* SCN_GetNextScene(void);
