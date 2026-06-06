#include <integrity/common/common.h>
#include <integrity/common/image_loader.h>
#include <integrity/common/input.h>
#include <integrity/common/renderer.h>
#include <integrity/common/resource_manager.h>
#include <integrity/common/stack.h>
#include <integrity/scene/scene.h>
#include <integrity/ui/ui_backend.h>

#include "../private.h"

char error_str[64] = {0};
extern IntegrityScene null_scene;

void Game_Main(__attribute__((unused)) int argc, __attribute__((unused)) char** argv) {
  extern int main(int argc, char** argv);
  srand((unsigned)Sys_FloatTime());
  /* Start up Resource Manager */
  resource_test();

  RNDR_Init(SCR_WIDTH, SCR_HEIGHT);

  /* Can do setup here if you NEED/WANT it to run before anything else */
  UI_Init();

  // Setup our basic scene as bottom
  SCN_Push(null_scene);
}

void Game_Exit(void) {
  SCN_Pop();

  resource_objects_empty();
}

void Host_Input(__attribute__((unused)) float time) {
}

static inline void Host_Render3D(float time) {
  RNDR_Reset();
  IntegrityScene* scene_current = SCN_Current();

  if (scene_current->flags & SCENE_FALLTHROUGH_RENDER) {
    IntegrityScene* next_scene = SCN_Peek();
    if (next_scene->render3D)
      (*next_scene->render3D)(time);
  }
  if (scene_current->render3D)
    (*scene_current->render3D)(time);
}

static inline void Host_Render2D(float time) {
  UI_Set2D();
  UI_TextColorEx(1.0f, 1.0f, 1.0f, 1.0f);
  IntegrityScene* scene_current = SCN_Current();

  if (scene_current->flags & SCENE_FALLTHROUGH_RENDER) {
    IntegrityScene* next = SCN_Peek();
    if (next->render2D)
      (*next->render2D)(time);
  }
  if (scene_current->render2D)
    (*scene_current->render2D)(time);
}

void Host_Update(float time) {
  /* Process deferred scene changes so the new scene's update
     runs before any render on this frame */
  SCN_FlushPendingChange();
  SCN_FlushPendingChange();  /* Handle chains (e.g. A_Init → SCN_ChangeTo(B)) */

  IntegrityScene* scene_current = SCN_Current();
  bool update_fallthrough = scene_current->flags & SCENE_FALLTHROUGH_UPDATE;

  if (scene_current->update)
    (*scene_current->update)(time);

  if (update_fallthrough) {
    IntegrityScene* next = SCN_Peek();
    if (next->update)
      (*next->update)(time);
  }
}

void Host_Frame(float time) {
  /* Render Both parts */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Clear The Screen And The Depth Buffer
  glLoadIdentity();                                    // Reset The View
  Host_Render3D(time);
  Host_Render2D(time);
}
