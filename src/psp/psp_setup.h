#pragma once

/* Define the module info section */
#if !defined(MODULE_NAME)
#define MODULE_NAME NONAME
#endif

#if !defined(MODULE_VERSION_MAJOR)
#define MODULE_VERSION_MAJOR 1
#endif

#if !defined(MODULE_VERSION_MINOR)
#define MODULE_VERSION_MINOR 0
#endif

#if !defined(MODULE_ATTR)
#define MODULE_ATTR 0
#endif

#define __stringify(s) __tostring(s)
#define __tostring(s) #s

PSP_MODULE_INFO(__stringify(MODULE_NAME), MODULE_ATTR, MODULE_VERSION_MAJOR, MODULE_VERSION_MINOR);

/*
	Since malloc uses the heap defined at compile time, we should use a negative value such as PSP_HEAP_SIZE_KB(-1024)
	instead of a hard coded value. So you'll have 23MB on Phat and 55MB on the Slim with 1MB for stacks etc in either case.
*/
PSP_HEAP_SIZE_KB(-1024);

/* Define the main thread's attribute value (optional) */
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
PSP_DISABLE_NEWLIB_TIMEZONE_SUPPORT();

static int done = 0;

/* Exit callback */
int exit_callback(__attribute__((unused)) int arg1, __attribute__((unused)) int arg2, __attribute__((unused)) void *common)
{
	done = 1;
	return 0;
}

/* Callback thread */
int CallbackThread(__attribute__((unused)) SceSize args, __attribute__((unused)) void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}
