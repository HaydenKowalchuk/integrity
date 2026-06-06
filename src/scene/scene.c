#include <integrity/common/common.h>
#include <integrity/common/resource_manager.h>
#include <integrity/common/stack.h>

struct StackNode* scene_root;
IntegrityScene scene_current;

static void* scene_context = NULL;
static IntegrityScene* scene_next = NULL;

IntegrityScene* SCN_Push(IntegrityScene next) {
  push(&scene_root, next);
  return &scene_root->data;
}

IntegrityScene* SCN_ChangeTo(IntegrityScene next) {
  pop(&scene_root);
  push(&scene_root, next);

  resource_objects_flush();
  return peek(&scene_root->next);
}

IntegrityScene* SCN_Pop(void) {
  pop(&scene_root);

  scene_context = NULL;
  resource_objects_flush();
  return SCN_Current();
}

IntegrityScene* SCN_Current(void) {
  return peek(&scene_root);
}

IntegrityScene* SCN_Peek(void) {
  return peek(&scene_root->next);
}

void SCN_SetContext(void* ctx) {
  scene_context = ctx;
}

void* SCN_GetContext(void) {
  return scene_context;
}

void SCN_SetNextScene(IntegrityScene* next) {
  scene_next = next;
}

IntegrityScene* SCN_GetNextScene(void) {
  return scene_next;
}
