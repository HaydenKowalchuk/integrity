#include <integrity/common/common.h>
#include <integrity/common/input.h>
#include <integrity/common/sound_system.h>
#include <integrity/scene/scene.h>
#include <integrity/ui/ui_backend.h>

WINDOW_TITLE("Audio | Integrity", WINDOWED);

static unsigned int sfx_a = 0;
static unsigned int sfx_b = 0;
static unsigned int sfx_x = 0;
static unsigned int sfx_y = 0;
static int sound_loaded = 0;

static void Audio_Init(void);
static void Audio_Exit(void);
static void Audio_Update(float time);
static void Audio_Render2D(float time);
static void Audio_Render3D(float time);

STARTUP_SCENE(&Audio_Init, &Audio_Exit, &Audio_Render2D, &Audio_Render3D, &Audio_Update, SCENE_BLOCK);

static int load_sound(unsigned int* dst, const char* path) {
  const char* resolved = FS_ResolvePathTemp(path);
  if (FS_FileExists(resolved))
    return create_sound(dst, resolved);
  return -1;
}

static void Audio_Init(void) {
  sound_loaded = 1;
  sound_loaded &= load_sound(&sfx_a, "beep_high.wav") > 0;
  if (!sound_loaded)
    fprintf(stderr, "audio: could not load beep_high sounds\n");
  sound_loaded &= load_sound(&sfx_b, "beep_low.wav") > 0;
  if (!sound_loaded)
    fprintf(stderr, "audio: could not load beep_low sounds\n");
  sound_loaded &= load_sound(&sfx_x, "noise.wav") > 0;
  if (!sound_loaded)
    fprintf(stderr, "audio: could not load noise sounds\n");
  sound_loaded &= load_sound(&sfx_y, "sweep.wav") > 0;
  if (!sound_loaded)
    fprintf(stderr, "audio: could not load sweep sounds\n");
}

static void Audio_Exit(void) {
}

static void Audio_Update(float time) {
  (void)time;

  if (INPT_ButtonEx(BTN_START, BTN_RELEASE)) {
    Sys_Quit();
  }

  if (!sound_loaded) return;

  if (INPT_ButtonEx(BTN_A, BTN_RELEASE)) SND_Play(sfx_a);
  if (INPT_ButtonEx(BTN_B, BTN_RELEASE)) SND_Play(sfx_b);
  if (INPT_ButtonEx(BTN_X, BTN_RELEASE)) SND_Play(sfx_x);
  if (INPT_ButtonEx(BTN_Y, BTN_RELEASE)) SND_Play(sfx_y);
}

static void Audio_Render2D(float time) {
  (void)time;

  UI_TextSize(32);
  UI_DrawStringCentered(320, 440, "Audio Sample");

  UI_TextSize(18);
  UI_DrawStringCentered(320, 400, "A (Cross)    - Play Sound 1");
  UI_DrawStringCentered(320, 375, "B (Circle)   - Play Sound 2");
  UI_DrawStringCentered(320, 350, "X (Square)   - Play Sound 3");
  UI_DrawStringCentered(320, 325, "Y (Triangle) - Play Sound 4");
  UI_DrawStringCentered(320, 290, "Start - Exit");
}

static void Audio_Render3D(float time) {
  (void)time;
}
