#include "input_test.h"

#include <integrity/common/input.h>
#include <integrity/common/renderer.h>
#include <integrity/ui/ui_backend.h>
#include <stdbool.h>

WINDOW_TITLE("input_test", WINDOWED);

static bool fullscreen = false;

static void Input_Init(void);
static void Input_Exit(void);
static void Input_Update(float time);
static void Input_Render2D(float time);

/* Registers the Title scene and populates the struct */
STARTUP_SCENE(&Input_Init, &Input_Exit, &Input_Render2D, NULL, &Input_Update, SCENE_BLOCK);
SCENE(scene_input_test, &Input_Init, NULL, &Input_Render2D, NULL, &Input_Update, SCENE_BLOCK);

static void draw_button(int x, int y, const char* label, int w, int h, bool active) {
  dimen_RECT rect = {x, y, w, h};
  if (active)
    UI_DrawFill(&rect, 255, 255, 255);
  else
    UI_DrawFill(&rect, 80, 80, 80);
  UI_TextSize(14);
  UI_TextColorEx(1, 1, 1, 1);
  UI_DrawStringCentered(x + w / 2, y + h / 2 - 7, label);
}

static void draw_dpad(int center_x, int center_y, int size, bool up, bool down, bool left, bool right) {
  int half = size / 2;
  int third = size / 3;
  dimen_RECT rect;

  /* Up */
  rect = (dimen_RECT){center_x - third, center_y - half, third * 2, third};
  UI_DrawFill(&rect, up ? 255 : 80, up ? 255 : 80, up ? 255 : 80);
  /* Down */
  rect = (dimen_RECT){center_x - third, center_y + half - third, third * 2, third};
  UI_DrawFill(&rect, down ? 255 : 80, down ? 255 : 80, down ? 255 : 80);
  /* Left */
  rect = (dimen_RECT){center_x - half, center_y - third, third, third * 2};
  UI_DrawFill(&rect, left ? 255 : 80, left ? 255 : 80, left ? 255 : 80);
  /* Right */
  rect = (dimen_RECT){center_x + half - third, center_y - third, third, third * 2};
  UI_DrawFill(&rect, right ? 255 : 80, right ? 255 : 80, right ? 255 : 80);
  /* Center */
  rect = (dimen_RECT){center_x - third, center_y - third, third * 2, third * 2};
  UI_DrawFill(&rect, 40, 40, 40);

  UI_TextSize(12);
  UI_TextColorEx(1, 1, 1, 1);
  UI_DrawStringCentered(center_x, center_y - half + third / 2 - 6, "U");
  UI_DrawStringCentered(center_x, center_y + half - third + third / 2 - 6, "D");
  UI_DrawStringCentered(center_x - half + third / 2, center_y - 6, "L");
  UI_DrawStringCentered(center_x + half - third + third / 2, center_y - 6, "R");
}

static void Input_Init(void) {
}

static void Input_Exit(void) {
}

static void Input_Update(float time) {
  (void)time;
  if (INPT_ButtonEx(BTN_START, BTN_RELEASE)) {
    Sys_Quit();
  }

  if (INPT_TriggerPressed(TRIGGER_L) && INPT_TriggerPressed(TRIGGER_R)) {
    fullscreen = !fullscreen;
    Sys_SetFullscreen(fullscreen);
  }
}

static void Input_Render2D(float time) {
  (void)time;

  /* Draw face buttons */
  draw_button(440, 240, "A", 40, 40, INPT_Button(BTN_A));
  draw_button(400, 200, "X", 40, 40, INPT_Button(BTN_X));
  draw_button(480, 200, "B", 40, 40, INPT_Button(BTN_B));
  draw_button(440, 160, "Y", 40, 40, INPT_Button(BTN_Y));

  /* Start button */
  draw_button(290, 300, "START", 60, 30, INPT_Button(BTN_START));

  /* D-Pad */
  draw_dpad(150, 200, 80, INPT_DPADDirection(DPAD_UP), INPT_DPADDirection(DPAD_DOWN), INPT_DPADDirection(DPAD_LEFT), INPT_DPADDirection(DPAD_RIGHT));

  /* Analog stick area */
  dimen_RECT stick_bg = {230, 156, 140, 140};
  UI_DrawFill(&stick_bg, 40, 40, 40);
  dimen_RECT stick_border = {230, 156, 140, 140};
  UI_DrawFill(&stick_border, 80, 80, 80);
  dimen_RECT stick_inner = {232, 158, 136, 136};
  UI_DrawFill(&stick_inner, 40, 40, 40);

  /* Center crosshair */
  dimen_RECT cross_v = {298, 158, 4, 136};
  UI_DrawFill(&cross_v, 60, 60, 60);
  dimen_RECT cross_h = {232, 224, 136, 4};
  UI_DrawFill(&cross_h, 60, 60, 60);

  float ax = INPT_AnalogF(AXES_X);
  float ay = INPT_AnalogF(AXES_Y);
  int cx = 300 + (int)(ax * 64.0f);
  int cy = 226 + (int)(ay * 64.0f);
  dimen_RECT cursor = {cx - 4, cy - 4, 8, 8};
  UI_DrawFill(&cursor, 255, 0, 0);
  float xfilter = ax * SQRT(1 - ay * ay * 0.5f);
  float yfilter = ay * SQRT(1 - ax * ax * 0.5f);
  int cxfilter = 300 + (int)(xfilter * 64.0f);
  int cyfilter = 226 + (int)(yfilter * 64.0f);
  dimen_RECT cursorfilter = {cxfilter - 4, cyfilter - 4, 8, 8};
  UI_DrawFill(&cursorfilter, 0, 255, 0);

  /* Analog raw values */
  UI_TextSize(14);
  UI_TextColorEx(1, 1, 1, 1);
  char msg[32];
  snprintf(msg, sizeof(msg), "raw: %3d, %3d", INPT_AnalogI(AXES_X), INPT_AnalogI(AXES_Y));
  UI_DrawStringCentered(300, 148, msg);
  snprintf(msg, sizeof(msg), "raw: %3.2f, %3.2f", (double)ax, (double)ay);
  UI_DrawStringCentered(300, 134, msg);
  snprintf(msg, sizeof(msg), "flt: %3.2f, %3.2f", (double)xfilter, (double)yfilter);
  UI_DrawStringCentered(300, 120, msg);

  /* Trigger indicators */
  UI_TextSize(14);
  draw_button(20, 72, "L TRIG", 80, 30, INPT_TriggerPressed(TRIGGER_L));
  draw_button(540, 72, "R TRIG", 80, 30, INPT_TriggerPressed(TRIGGER_R));

  /* Bottom help text */
  UI_TextSize(16);
  UI_TextColorEx(1, 1, 1, 1);
  UI_DrawStringCentered(320, 420, "Both Triggers fullscreen");
  UI_DrawStringCentered(320, 460, "Press START to exit");
}
