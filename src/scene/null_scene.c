#include <integrity/common/common.h>
#include <integrity/scene/scene.h>
#include <integrity/ui/ui_backend.h>

static void Null_Update(float time);
static int is_loaded = 0;

struct IntegrityScene null_scene = {
    .init = NULL,
    .render2D = NULL,
    .render3D = NULL,
    .update = &Null_Update,
    .flags = SCENE_BLOCK,
};

static void Null_Update(float time) {
  (void)time;
  if (!is_loaded) {
    is_loaded = 1;
    null_scene.update = NULL;
    SCN_ChangeTo(scene_start);
  }
}
