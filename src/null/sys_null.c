#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>

#include "errno.h"

#include <integrity/common/common.h>
#include <integrity/common/input.h>
#include "../private.h"
#include <integrity/common/renderer.h>

void Sys_Quit(void);

/*
===============================================================================

SYSTEM IO

===============================================================================
*/
void Sys_Error(const char *error, ...)
{
	va_list argptr;

	printf("Sys_Error: ");
	va_start(argptr, error);
	vprintf(error, argptr);
	va_end(argptr);
	printf("\n");

	Sys_Quit();
}

void Sys_Printf(const char *fmt, ...)
{
	va_list argptr;

	va_start(argptr, fmt);
	vprintf(fmt, argptr);
	va_end(argptr);
}

void Sys_Quit(void)
{
	arch_exit();
#if 0
	glKosSwapBuffers();
	Host_Shutdown();
	vid_set_mode(DM_640x480, PM_RGB565);
	vid_empty();

	/* Display the error message on screen */
	drawtext(32, 64, "nuQuake shutdown...");
	arch_menu();
#endif
}

#include <sys/time.h>

float Sys_FloatTime(void)
{
	struct timeval tp;
	struct timezone tzp;
  static int secbase = 0;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return (float)(tp.tv_usec / 1000000.0f);
	}

	return (float)((tp.tv_sec - secbase) + tp.tv_usec / 1000000.0f);
}

char *Sys_ConsoleInput(void)
{
	return NULL;
}

void Sys_Sleep(void)
{
}

// process all input: the handler will figure out which are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput()
{
	static inputs _input;
	int buttons;

	/*  Reset Everything */
	memset(&_input, 0, sizeof(inputs));

	/* DPAD */
	//_input.dpad = (state->buttons >> 4);

	/* BUTTONS */
	//_input.btn_a = (uint8_t)(buttons & CONT_A);
	//_input.btn_b = (uint8_t)(buttons & CONT_B);
	//_input.btn_x = (uint8_t)(buttons & CONT_X);
	//_input.btn_y = (uint8_t)(buttons & CONT_Y);
	//_input.btn_start = (uint8_t)(buttons & CONT_START);

	INPT_ReceiveFromHost(_input);
}

int main(int argc, char **argv)
{
	float time, oldtime, newtime;

	oldtime = Sys_FloatTime() - 0.1;
	//profiler_enable();
	Game_Main(argc, argv);
	while (1)
	{
		newtime = Sys_FloatTime();
		time = newtime - oldtime;
		processInput();
		//Handle Input in Game
		Host_Input(time);

		Host_Frame(time);
		//glKosSwapBuffers();
		oldtime = newtime;
	}
	return 1;
}
