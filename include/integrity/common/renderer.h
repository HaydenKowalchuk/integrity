#pragma once

#include <integrity/common/common.h>
#include <integrity/common/image_loader.h>
#include <integrity/common/obj_loader.h>
#include <integrity/common/renderer_types.h>

#define WIDE_ASPECT (16.0f / 9.0f)
#define SQUARE_ASPECT (4.0f / 3.0f)

#if defined(_arch_dreamcast)
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glkos.h>
#include <GL/glu.h>
#endif
#if defined(__MINGW32__) || defined(__linux__) || defined(__APPLE__)
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif
#if defined(__PSP__)
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLES/egl.h>
#endif

/* Override gluPerspective & gluLookAt on desktop */
#if defined(__MINGW32__) || defined(__linux__) || defined(__APPLE__)

#else

#endif

#define glCheckError() glCheckError_(__FILE__, __LINE__)
GLenum glCheckError_(const char* file, int line);

// We call this right after our OpenGL window is created.
void RNDR_Init(int Width, int Height);
void RNDR_Reset(void);

/* The function called when our window is resized (which shouldn't happen, because we're fullscreen) */
void RNDR_Resize(int Width, int Height);

GLuint RNDR_CreateTextureFromImage(tx_image* img);
GLuint RNDR_CreateTextureFromImageEx(tx_image* img, int wrap_s, int wrap_t, int mag_filter, int min_filter);

static inline void GL_Bind(tx_image* img) {
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, img->id);
}

/* Related to new aspect ratio */
void RNDR_SetWidescreen(bool wide);
void RNDR_FlipAspect(void);

// settings
extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;
