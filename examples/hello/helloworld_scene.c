#include <integrity/common/common.h>
#include <integrity/common/input.h>
#include <integrity/scene/scene.h>
#include <integrity/ui/ui_backend.h>

WINDOW_TITLE("Helloworld | Integrity", WINDOWED);

static void Helloworld_Init(void);
static void Helloworld_Exit(void);
static void Helloworld_Update(float time);
static void Helloworld_Render2D(float time);
static void Helloworld_Render3D(float time);

/* Registers the Title scene and populates the struct */
STARTUP_SCENE(&Helloworld_Init, &Helloworld_Exit, &Helloworld_Render2D, &Helloworld_Render3D, &Helloworld_Update, SCENE_BLOCK);
// static SCENE(scene_helloworld, &Helloworld_Init, &Helloworld_Exit, &Helloworld_Render2D, &Helloworld_Render3D, &Helloworld_Update, SCENE_BLOCK);

static void Helloworld_Init(void) {
}

static void Helloworld_Exit(void) {
}

static void Helloworld_Update(float time) {
  (void)time;

  if (INPT_ButtonEx(BTN_START, BTN_RELEASE)) {
    Sys_Quit();
  }
}

static void Helloworld_Render2D(float time) {
  (void)time;

  UI_TextSize(32);
  UI_DrawStringCentered(320, 440, "Hello World!");
}

static void Helloworld_Render3D(float time) {
  (void)time;
}
