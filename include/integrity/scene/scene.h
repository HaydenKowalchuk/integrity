#pragma once

#include <stdint.h>

typedef struct scene {
  void (*init)(void);
  void (*exit)(void);
  void (*render2D)(float);
  void (*render3D)(float);
  void (*update)(float);
  int flags;
  void* context;
} scene;

#define STARTUP_SCENE(_init, _exit, _render2, _render3, _update, _flags) struct scene scene_start = {_init, _exit, _render2, _render3, _update, _flags}
#define SCENE(_local, _init, _exit, _render2, _render3, _update, _flags) struct scene _local = {_init, _exit, _render2, _render3, _update, _flags}

extern scene scene_start;

#define SCENE_FALLTHROUGH_RENDER 1
#define SCENE_FALLTHROUGH_UPDATE 2
#define SCENE_BLOCK 4
#define SCENE_NO_REINIT_CHILD 8

scene* SCN_ChangeTo(scene next);
scene* SCN_Push(scene next);
scene* SCN_Pop(void);
scene* SCN_Current(void);
scene* SCN_Peek(void);

void SCN_SetContext(void* ctx);
void* SCN_GetContext(void);
void SCN_SetNextScene(scene* next);
scene* SCN_GetNextScene(void);
