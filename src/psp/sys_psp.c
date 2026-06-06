#include <GL/glut.h>
#include <GLES/egl.h>
#include <integrity/common/common.h>
#include <integrity/common/input.h>
#include <integrity/common/renderer.h>
#include <integrity/common/sound_system.h>
#include <pspctrl.h>
#include <pspgl_misc.h>
#include <pspuser.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/unistd.h>

#include "../private.h"
#include "psp_setup.h"

void Sys_Quit(void);

// char *psp_log_file = "log_jump.txt";

// settings
unsigned int SCR_WIDTH = 480;
unsigned int SCR_HEIGHT = 272;
int analog_emulation = 0;

static EGLDisplay dpy;
static EGLContext ctx;
static EGLSurface surface;
static unsigned int glut_display_mode = 0;

static EGLint attrib_list[] = {
    EGL_RED_SIZE, 8, /* 0 */
    EGL_GREEN_SIZE,
    8, /* 2 */
    EGL_BLUE_SIZE,
    8, /* 4 */
    EGL_ALPHA_SIZE,
    0, /* 6 */
    EGL_STENCIL_SIZE,
    0, /* 8 */
    EGL_DEPTH_SIZE,
    0, /* 10 */
    EGL_NONE};

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

  if (!secbase) {
    secbase = tp.tv_sec;
    return tp.tv_usec / 1000000.0;
  }

  return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0;
}

static unsigned int frame_count;
unsigned int Sys_Frames(void) {
  return frame_count;
}

char* Sys_ConsoleInput(void) {
  return NULL;
}

void Sys_Sleep(void) {
}

void Sys_Quit(void) {
  Game_Exit();
  eglTerminate(dpy);
  done = 1;
}

bool Sys_IsFullscreen(void) {
  return true;
}

void Sys_SetFullscreen(bool fullscreen) {
  (void)fullscreen;
}

void create_gl(void) {
  EGLConfig config;
  EGLint num_configs;

  /* pass NativeDisplay=0, we only have one screen... */
  dpy = eglGetDisplay(0);
  eglInitialize(dpy, NULL, NULL);

  /* Select type of Display mode:
     Double buffer
     RGBA color
     Alpha components supported
     Depth buffered for automatic clipping */
  glut_display_mode = (GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);

  if (glut_display_mode & GLUT_ALPHA)
    attrib_list[7] = 8;
  if (glut_display_mode & GLUT_STENCIL)
    attrib_list[9] = 8;
  if (glut_display_mode & GLUT_DEPTH)
    attrib_list[11] = 16;

  eglChooseConfig(dpy, attrib_list, &config, 1, &num_configs);

  if (num_configs == 0) {
    printf("glutCreateWindow: eglChooseConfig returned no configurations for display mode %x\n", glut_display_mode);
  }

  ctx = eglCreateContext(dpy, config, NULL, NULL);
  surface = eglCreateWindowSurface(dpy, config, 0, NULL);
  eglMakeCurrent(dpy, surface, surface, ctx);
}

// process all input: the handler will figure out which are pressed/released this frame and react accordingly
void processInput(void) {
  static inputs _input;
  static SceCtrlData pad;

  /*  Reset Everything */
  memset(&_input, 0, sizeof(inputs));
  memset(&pad, 0, sizeof(SceCtrlData));

  if (!sceCtrlPeekBufferPositive(&pad, 1))
    return;

  sceCtrlSetIdleCancelThreshold(40, 40);

#if 0
  printf("Analog X = %d ", pad.Lx);
  printf("Analog Y = %d \n", pad.Ly);

  if (pad.Buttons & PSP_CTRL_SELECT) {
    printf("Select pressed \n");
  }
  if (pad.Buttons & PSP_CTRL_LTRIGGER) {
    printf("L-trigger pressed \n");
  }
  if (pad.Buttons & PSP_CTRL_RTRIGGER) {
    printf("R-trigger pressed \n");
  }

#endif

  /* DPAD */
  _input.dpad |= (!!(pad.Buttons & PSP_CTRL_UP)) << SHIFT_UP;
  _input.dpad |= (!!(pad.Buttons & PSP_CTRL_DOWN)) << SHIFT_DOWN;
  _input.dpad |= (!!(pad.Buttons & PSP_CTRL_LEFT)) << SHIFT_LEFT;
  _input.dpad |= (!!(pad.Buttons & PSP_CTRL_RIGHT)) << SHIFT_RIGHT;

  /* ANALOG EMULATION */
  if (analog_emulation) {
#define DEADZONE_1 40
#define CENTER_1 127
    _input.dpad |= (pad.Ly >= CENTER_1 + DEADZONE_1) << SHIFT_DOWN;
    _input.dpad |= (pad.Ly <= CENTER_1 - DEADZONE_1) << SHIFT_UP;
    _input.dpad |= (pad.Lx >= CENTER_1 + DEADZONE_1) << SHIFT_RIGHT;
    _input.dpad |= (pad.Lx <= CENTER_1 - DEADZONE_1) << SHIFT_LEFT;
  }

/* ANALOG INPUT */
#define DEADZONE 16 /* in both directions */
#define CENTER 128
  //_input.axes_1 = ((pad.Lx <= CENTER - DEADZONE) || (pad.Lx >= CENTER + DEADZONE)) ? (((pad.Ly <= CENTER - DEADZONE) || (pad.Ly >= CENTER + DEADZONE)) ? pad.Lx : CENTER) : CENTER;
  //_input.axes_2 = ((pad.Ly <= CENTER - DEADZONE) || (pad.Ly >= CENTER + DEADZONE)) ? (((pad.Lx <= CENTER - DEADZONE) || (pad.Lx >= CENTER + DEADZONE)) ? pad.Ly : CENTER) : CENTER;

  _input.axes_1 = ((pad.Lx <= CENTER - DEADZONE) || (pad.Lx >= CENTER + DEADZONE)) ? pad.Lx : CENTER;
  _input.axes_2 = ((pad.Ly <= CENTER - DEADZONE) || (pad.Ly >= CENTER + DEADZONE)) ? pad.Ly : CENTER;

  /* TRIGGERS */
  _input.trg_left = (!!(pad.Buttons & PSP_CTRL_LTRIGGER));
  _input.trg_right = (!!(pad.Buttons & PSP_CTRL_RTRIGGER));

  /* BUTTONS */
  _input.btn_a = !!(pad.Buttons & PSP_CTRL_CROSS);
  _input.btn_b = !!(pad.Buttons & PSP_CTRL_CIRCLE);
  _input.btn_x = !!(pad.Buttons & PSP_CTRL_SQUARE);
  _input.btn_y = !!(pad.Buttons & PSP_CTRL_TRIANGLE);
  _input.btn_start = !!(pad.Buttons & PSP_CTRL_START);

  INPT_ReceiveFromHost(_input);
}

int main(int argc, char** argv) {
  SetupCallbacks();
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  // char buf[128] = {0};
  // getcwd(buf, 128);
  // printf("cwd: %s\n", buf);
  FS_ClearSearchPaths();
  FS_AddSearchPath("disc0:/PSP_GAME/USRDIR");
  FS_AddSearchPath("host0:/");
  FS_AddSearchPath("");

  SYS_SND_Setup();

  create_gl();

  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

  frame_count = 0;

  // profiler_enable();

  /* So we start with correct screen size */
  RNDR_SetWidescreen(true);
  Game_Main(argc, argv);

  float currenttime = Sys_FloatTime(), newtime, frametime;
  const float dt = 1.0f / 60.0f;

  while (!done) {
    newtime = Sys_FloatTime();
    frametime = newtime - currenttime;
    currenttime = newtime;

    /* Handle Input in Game */
    Host_Input(dt);
    Host_Update(dt);

    processInput();

    // Render
    Host_Frame(frametime);
    frame_count++;
    eglSwapBuffers(dpy, surface);

    SND_Update();
  }
  SYS_SND_Destroy();
  sceKernelExitGame();
  return 0;
}
