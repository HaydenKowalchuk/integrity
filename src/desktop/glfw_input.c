#include <GLFW/glfw3.h>
#include <integrity/common/common.h>
#include <integrity/common/input.h>
#include <stdbool.h>
#include <stdio.h>

static inputs _input;
static uint8_t axes_x = 128;
static uint8_t axes_y = 128;
extern int joystick_present;

void processInputFromKeyboard(GLFWwindow* window) {
  if (joystick_present) {
    return;
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

  /* Mark out Analog Input emulation */
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

  INPT_ReceiveFromHost(_input);
}

void processInputFromJoystick(void) {
  if (!joystick_present) {
    return;
  }

  /*  Reset Everything */
  memset(&_input, 0, sizeof(inputs));

  int axes_count;
  const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);

  if (axes_count >= 2) {
    _input.axes_1 = (uint8_t)((axes[0] * 127.5f) + 127.5f);
    _input.axes_2 = (uint8_t)((axes[1] * 127.5f) + 127.5f);
    // printf("%d\t %0.2f %0.2f %0.2f %0.2f %0.2f %0.2f\n", axes_count,
    //   axes[0],axes[1],axes[2],axes[3], axes[4],axes[5]
    // );
  }

  int button_count;
  const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &button_count);

  if (button_count >= 10) {
    _input.btn_a = buttons[1] == GLFW_PRESS;
    _input.btn_b = buttons[2] == GLFW_PRESS;
    _input.btn_x = buttons[0] == GLFW_PRESS;
    _input.btn_y = buttons[3] == GLFW_PRESS;

    // l1 / r1
    //_input.trg_left = buttons[4] == GLFW_PRESS;
    //_input.trg_right = buttons[5] == GLFW_PRESS;

    // l2 / r2
    _input.trg_left = buttons[6] == GLFW_PRESS;
    _input.trg_right = buttons[7] == GLFW_PRESS;

    _input.btn_start = buttons[9] == GLFW_PRESS;

    /* printf("%d\t %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
       button_count,
       buttons[0] == GLFW_PRESS,
       buttons[1] == GLFW_PRESS,
       buttons[2] == GLFW_PRESS,
       buttons[3] == GLFW_PRESS,
       buttons[4] == GLFW_PRESS,
       buttons[5] == GLFW_PRESS,
       buttons[6] == GLFW_PRESS,
       buttons[7] == GLFW_PRESS,

       buttons[8] == GLFW_PRESS,
       buttons[9] == GLFW_PRESS,
       buttons[10] == GLFW_PRESS,
       buttons[11] == GLFW_PRESS,
       buttons[12] == GLFW_PRESS,
       buttons[13] == GLFW_PRESS,
       buttons[14] == GLFW_PRESS,
       buttons[15] == GLFW_PRESS
     );
     */
  }

  INPT_ReceiveFromHost(_input);
}
