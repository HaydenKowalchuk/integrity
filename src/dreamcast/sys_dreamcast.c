#include <arch/arch.h>
#include <assert.h>
#include <integrity/common/common.h>
#include <integrity/common/file_access.h>
#include <integrity/common/input.h>
#include <integrity/common/renderer.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "../private.h"

static void drawtext(int x, int y, const char* string);
void snd_bump_poll(void);

// settings
unsigned int SCR_WIDTH = 640;
unsigned int SCR_WIDTH_WIDE = 854;
unsigned int SCR_HEIGHT = 480;

int Sys_FileTime(const char* path) {
  struct stat buffer;
  return ((stat(path, &buffer) == 0) ? 1 : -1);
}

void Sys_mkdir(__attribute__((unused)) const char* path) {
}

/*
===============================================================================

SYSTEM IO

===============================================================================
*/
void Sys_Quit(void) {
  /* Reset video mode, clear screen */
  // vid_set_mode(DM_640x480, PM_RGB565);
  // vid_empty();
  // arch_exit();
#if 1
  glKosSwapBuffers();
  Game_Exit();
  // Host_Shutdown();
  vid_set_mode(DM_640x480, PM_RGB565);
  vid_empty();

  /* Display the error message on screen */
  drawtext(32, 64, "nuQuake shutdown...");
  arch_menu();
#endif
}

void Sys_Error(const char* error, ...) {
  va_list argptr;

  printf("Sys_Error: ");
  va_start(argptr, error);
  vprintf(error, argptr);
  va_end(argptr);
  printf("\n");

  Sys_Quit();
}

void Sys_Printf(const char* fmt, ...) {
  va_list argptr;

  va_start(argptr, fmt);
  vprintf(fmt, argptr);
  va_end(argptr);
}

float Sys_FloatTime(void) {
  struct timeval tp;
  struct timezone tzp;
  static int secbase = 0;

  gettimeofday(&tp, &tzp);

#define divisor (1 / 1000000.0)

  if (!secbase) {
    secbase = tp.tv_sec;
    return tp.tv_usec * divisor;
  }

  return (tp.tv_sec - secbase) + tp.tv_usec * divisor /* / 1000000.0*/;
}

static unsigned int frame_count;
unsigned int Sys_Frames(void) {
  return frame_count;
}

void Sys_Sleep(void) {
}

bool Sys_IsFullscreen(void) {
  return true;
}

void Sys_SetFullscreen(bool fullscreen) {
  (void)fullscreen;
}

static void drawtext(int x, int y, const char* string) {
  printf("%s\n", string);
  int offset = ((y * 640) + x);
  char* kos_missing_constness = (void*)string;
  bfont_draw_str(vram_s + offset, 640, 1, kos_missing_constness);
}

static void assert_hnd(const char* file, int line, const char* expr, const char* msg, const char* func) {
  char strbuffer[256];

  /* Reset video mode, clear screen */
  vid_set_mode(DM_640x480, PM_RGB565);
  vid_empty();

  /* Display the error message on screen */
  drawtext(32, 64, "nuQuake - Assertion failure");

  snprintf(strbuffer, 256, " Location: %s, line %d (%s)", file, line, func);
  drawtext(32, 96, strbuffer);

  snprintf(strbuffer, 256, "Assertion: %s", expr);
  drawtext(32, 128, strbuffer);

  snprintf(strbuffer, 256, "  Message: %s", msg);
  drawtext(32, 160, strbuffer);

  arch_menu();
}

// process all input: the handler will figure out which are pressed/released this frame and react accordingly
void processInput(void) {
  static inputs _input;
  unsigned int buttons;

  maple_device_t* cont;
  cont_state_t* state;

  cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
  if (!cont)
    return;
  state = (cont_state_t*)maple_dev_status(cont);

  buttons = state->buttons;

  /*  Reset Everything */
  memset(&_input, 0, sizeof(inputs));

  /* DPAD */
  _input.dpad = (state->buttons >> 4) & ~240;  // mrneo240 ;)

  /* BUTTONS */
  _input.btn_a = (uint8_t)!!(buttons & CONT_A);
  _input.btn_b = (uint8_t)!!(buttons & CONT_B);
  _input.btn_x = (uint8_t)!!(buttons & CONT_X);
  _input.btn_y = (uint8_t)!!(buttons & CONT_Y);
  _input.btn_start = (uint8_t)!!(buttons & CONT_START);

  /* ANALOG */
  _input.axes_1 = ((uint8_t)(state->joyx) + 128);
  _input.axes_2 = ((uint8_t)(state->joyy) + 128);

  /* TRIGGERS */
  _input.trg_left = (uint8_t)state->ltrig & 255;
  _input.trg_right = (uint8_t)state->rtrig & 255;

  INPT_ReceiveFromHost(_input);
}

static int done = 0;

int main(int argc, char** argv) {
  // Set up assertion handler
  assert_set_handler(assert_hnd);

  FS_ClearSearchPaths();
  FS_AddSearchPath("/pc");
  FS_AddSearchPath("/cd");
  FS_AddSearchPath("/sd/integrity");
  FS_AddSearchPath("/ide/integrity");

  glKosInit();

  frame_count = 0;

  // profiler_enable();
  Game_Main(argc, argv);

  float currenttime, newtime, frametime;
  float accumulator = 0.0f;
  const float dt = 1.0f / 60.0f;

  currenttime = Sys_FloatTime();

  while (!done) {
    newtime = Sys_FloatTime();
    frametime = newtime - currenttime;
    currenttime = newtime;
    accumulator += frametime;

    while (accumulator > dt) {
      /* Handle Input in Game */
      processInput();
      Host_Input(dt);
      Host_Update(dt);

      accumulator -= dt;
    }
    if (accumulator < 0) {
      accumulator = 0;
    }

    // Render
    Host_Frame(frametime);
    // snd_bump_poll(); // For Sound
    frame_count++;

    glKosSwapBuffers();
  }
  return 1;
}
