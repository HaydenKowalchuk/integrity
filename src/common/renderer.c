#include <integrity/common/common.h>
#include <integrity/common/renderer.h>

#if defined(__APPLE__)
#include <GLKit/GLKMatrix4.h>
#endif

static bool video_aspect_wide = false;
static GLfloat g_proj_matrix[16];

void perspectiveGL(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar) {
  GLdouble fW, fH;

  // fH = tan( (fovY / 2) / 180 * M_PI ) * zNear;
  fH = tan(fovY / 360.0 * M_PI) * zNear;
  fW = fH * aspect;

  glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

static void capture_proj_matrix(void) {
  glGetFloatv(GL_PROJECTION_MATRIX, g_proj_matrix);
}

/* A general OpenGL initialization function.  Sets all of the initial parameters. */
void RNDR_Init(int Width, int Height)  // We call this right after our OpenGL window is created.
{
  glClearDepth(1.0);        // Enables Clearing Of The Depth Buffer
  glDepthFunc(GL_LESS);     // The Type Of Depth Test To Do
  glShadeModel(GL_SMOOTH);  // Enables Smooth Color Shading
  glFrontFace(GL_CCW);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();  // Reset The Projection Matrix

  // gluPerspective(45.0f, (video_aspect_wide) ? WIDE_ASPECT : SQUARE_ASPECT, 0.1f, 100.0f);  // Calculate The Aspect Ratio Of The Window
  perspectiveGL(45.0f, (video_aspect_wide) ? WIDE_ASPECT : SQUARE_ASPECT, 1.0f, 512.0f);  // Calculate The Aspect Ratio Of The Window
  capture_proj_matrix();

  glMatrixMode(GL_MODELVIEW);
  glEnableClientState(GL_VERTEX_ARRAY);

  SCR_WIDTH = Width;
  SCR_HEIGHT = Height;
}

void RNDR_Reset(void)  // We call this right after our OpenGL window is created.
{
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearDepth(1.0);

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(g_proj_matrix);

  glMatrixMode(GL_MODELVIEW);
  glEnableClientState(GL_VERTEX_ARRAY);
}

/* The function called when our window is resized (which shouldn't happen, because we're fullscreen) */
void RNDR_Resize(int Width, int Height) {
  if (Height == 0)  // Prevent A Divide By Zero If The Window Is Too Small
    Height = 1;

  glViewport(0, 0, Width, Height);  // Reset The Current Viewport And Perspective Transformation

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  perspectiveGL(45.0f, (GLfloat)Width / (GLfloat)Height, 1.0f, 512.0f);
  capture_proj_matrix();
  glMatrixMode(GL_MODELVIEW);
}

void RNDR_SetWidescreen(bool wide) {
  video_aspect_wide = wide;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  perspectiveGL(45.0f, (video_aspect_wide) ? WIDE_ASPECT : SQUARE_ASPECT, 1.0f, 512.0f);
  capture_proj_matrix();
  glMatrixMode(GL_MODELVIEW);
}

void RNDR_FlipAspect(void) {
  RNDR_SetWidescreen(!video_aspect_wide);
}

#if defined(__APPLE__)
// macOS implementation of gluLookAt using GLKMatrix4MakeLookAt
void gluLookAt(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble centerX, GLdouble centerY, GLdouble centerZ, GLdouble upX, GLdouble upY, GLdouble upZ) {
  GLKMatrix4 matrix = GLKMatrix4MakeLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
  glMultMatrixf(matrix.m);
}
#endif
