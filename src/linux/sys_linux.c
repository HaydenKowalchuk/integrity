#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <integrity/common/common.h>
#include <integrity/common/input.h>
#include <integrity/common/renderer.h>
#include <stdbool.h>
#include <stdio.h>

#include "../private.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void close_callback(GLFWwindow *window);

static GLFWwindow *main_window;
static GLFWmonitor *monitor;

// settings
unsigned int SCR_WIDTH = 640;
unsigned int SCR_HEIGHT = 480;
static int _window_pos[2];
static int _window_size[2];

float Sys_FloatTime(void) {
  return (float)glfwGetTime();
}
static unsigned int frame_count;
unsigned int Sys_Frames(void) {
  return frame_count;
}
int SYS_SND_Setup(void);
int SYS_SND_Destroy(void);

void Sys_Quit(void) {
  Game_Exit();
  glfwSetWindowShouldClose(main_window, true);
}

bool Sys_IsFullscreen(void) {
  return glfwGetWindowMonitor(main_window) != NULL;
}

void Sys_SetFullscreen(bool fullscreen) {
  /* Bail early */
  if (Sys_IsFullscreen() == fullscreen)
    return;

  if (fullscreen) {
    /* Save current window position and size */
    glfwGetWindowPos(main_window, &_window_pos[0], &_window_pos[1]);
    glfwGetWindowSize(main_window, &_window_size[0], &_window_size[1]);

    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    glfwSetWindowMonitor(main_window, monitor, 0, 0, mode->width, mode->height, 0);
  } else {
    /* Can use stored window size to restore or use predefined */
    glfwSetWindowMonitor(main_window, NULL, _window_pos[0], _window_pos[1], SCR_WIDTH, SCR_HEIGHT, 0);
  }
}

void error_func(int a, const char *str) {
  printf("GLFW Error[%d]: %s\n", a, str);
}

int __attribute__((weak)) main(int argc, char **argv) {
  glfwSetErrorCallback(&error_func);

  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  extern const char *window_title;
  extern int _FULLSCREEN;

  char _w_title[64] = "Integrity Engine";
  if (window_title != NULL && strlen(window_title) != 0) {
    strncpy(_w_title, window_title, 63);
    _w_title[63] = '\0';
  }

  /* Get current monitor info incase we need fullscreen */
  monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(monitor);

  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  if (_FULLSCREEN == 1) {
    main_window = glfwCreateWindow(mode->width, mode->height, _w_title, monitor, NULL);
  } else {
    main_window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, _w_title, NULL, NULL);
  }

  if (main_window == NULL) {
    printf("Failed to create GLFW window\n");
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(main_window);
  glfwSwapInterval(1);
  glfwSetFramebufferSizeCallback(main_window, framebuffer_size_callback);
  glfwSetWindowCloseCallback(main_window, close_callback);

  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("Failed to initialize GLAD\n");
    return -1;
  }

  if (SYS_SND_Setup() != 0) {
    printf("Failed to initialize OpenAL\n");
    /* Should handle Gracefully */
    /* return -1; */
  }

  frame_count = 0;

  float currenttime;
  float accumulator = 0.0f;
  const float dt = 1.0f / 60.0f;

  currenttime = Sys_FloatTime();
  Game_Main(argc, argv);

  // render loop
  // -----------
  while (!glfwWindowShouldClose(main_window)) {
    float newtime = Sys_FloatTime();
    float frametime = newtime - currenttime;
    currenttime = newtime;
    accumulator += frametime;

    while (accumulator >= dt) {
      /* Handle Input in Game */
      processInput(main_window);
      Host_Input(dt);
      Host_Update(dt);
      accumulator -= dt;
    }

    // Render
    Host_Frame(frametime);
    frame_count++;

    glfwSwapBuffers(main_window);
    glfwPollEvents();
  }

  glfwTerminate();

  SYS_SND_Destroy();
  return 0;
}

static inputs _input;
static uint8_t axes_x = 128;
static uint8_t axes_y = 128;

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    Game_Exit();
    glfwSetWindowShouldClose(window, true);
  }

  /*  Reset Everything */
  memset(&_input, 0, sizeof(inputs));

  /* Mark out DPAD inputs */
  if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
    _input.dpad |= (DPAD_LEFT);
  }
  if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    _input.dpad |= (DPAD_RIGHT);
  }
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    _input.dpad |= (DPAD_DOWN);
  }
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    _input.dpad |= (DPAD_UP);
  }

  /* Mark out Buttons */
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
    _input.btn_a = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
    _input.btn_b = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
    _input.btn_x = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
    _input.btn_y = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS) {
    _input.btn_start = 1;
  }

  /* Mark out DPAD inputs */
  if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
    if (axes_x > 0)
      axes_x--;
  }
  if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
    if (axes_x < 255)
      axes_x++;
  }

  if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
    if (axes_y > 0)
      axes_y--;
  }
  if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
    if (axes_y < 255)
      axes_y++;
  }

  _input.axes_1 = axes_x;
  _input.axes_2 = axes_y;

  /* Triggers */
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    _input.trg_left = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
    _input.trg_right = 1;
  }

  /* Fullscreen Toggle */
  if (glfwGetKey(window, GLFW_KEY_F10) == GLFW_PRESS) {
    Sys_SetFullscreen(!Sys_IsFullscreen());
  }

  INPT_ReceiveFromHost(_input);
}

/* Handle window size changes */
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  (void)window;
  glViewport(0, 0, width, height);
}

void close_callback(GLFWwindow *window) {
  (void)window;
  Game_Exit();
}
