#include <integrity/common/common.h>
#include <integrity/common/input.h>
#include <integrity/common/sound_system.h>
#include <integrity/scene/scene.h>
#include <integrity/ui/ui_backend.h>

WINDOW_TITLE("Audio | Integrity", WINDOWED);

static unsigned int sfx;
static int sound_loaded;

static void Audio_Init(void);
static void Audio_Exit(void);
static void Audio_Update(float time);
static void Audio_Render2D(float time);
static void Audio_Render3D(float time);

STARTUP_SCENE(&Audio_Init, &Audio_Exit, &Audio_Render2D, &Audio_Render3D, &Audio_Update, SCENE_BLOCK);

static void Audio_Init(void) {
  sfx = 0;
  sound_loaded = create_sound(&sfx, FS_ResolvePathTemp("assets/click.wav"));
  if (sound_loaded < 0) {
    fprintf(stderr, "audio: could not load assets/click.wav\n");
  }
}

static void Audio_Exit(void) {
}

static void Audio_Update(float time) {
  (void)time;

  if (INPT_ButtonEx(BTN_START, BTN_RELEASE)) {
    Sys_Quit();
  }

  if (INPT_ButtonEx(BTN_A, BTN_RELEASE) && sound_loaded > 0) {
    SND_Play(sfx);
  }
}

static void Audio_Render2D(float time) {
  (void)time;

  UI_TextSize(32);
  UI_DrawStringCentered(320, 440, "Audio Sample");

  UI_TextSize(18);
  UI_DrawStringCentered(320, 400, "Press A to play sound");
  UI_DrawStringCentered(320, 370, "Press Start to exit");
}

static void Audio_Render3D(float time) {
  (void)time;
}
