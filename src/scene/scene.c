#include <integrity/common/common.h>
#include <integrity/common/stack.h>
#include <integrity/common/resource_manager.h>

struct StackNode *scene_root;
scene scene_current;

static void* scene_context = NULL;
static scene* scene_next = NULL;

scene *SCN_Push(scene next)
{
  push(&scene_root, next);
  return &scene_root->data;
}

scene *SCN_ChangeTo(scene next)
{
  pop(&scene_root);
  push(&scene_root, next);

  resource_objects_flush();
  return peek(&scene_root->next);
}

scene *SCN_Pop(void)
{
  pop(&scene_root);

  scene_context = NULL;
  resource_objects_flush();
  return SCN_Current();
}

scene *SCN_Current(void)
{
  return peek(&scene_root);
}

scene *SCN_Peek(void)
{
  return peek(&scene_root->next);
}

void SCN_SetContext(void* ctx)
{
  scene_context = ctx;
}

void* SCN_GetContext(void)
{
  return scene_context;
}

void SCN_SetNextScene(scene* next)
{
  scene_next = next;
}

scene* SCN_GetNextScene(void)
{
  return scene_next;
}
