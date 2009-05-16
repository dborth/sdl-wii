/* Include the SDL main definition header */
#include "SDL_main.h"
#undef main

/* Standard includes */
#include <stdio.h>

/* SDL includes */
#include "../../video/wii/SDL_wiivideo.h"

/* OGC includes */
#include <ogcsys.h>
#include <wiiuse/wpad.h>
#include <ogc/usbmouse.h>
#include <wiikeyboard/keyboard.h>

bool TerminateRequested=false, ShutdownRequested=false, ResetRequested=false;

#ifdef HW_RVL
void SDL_Quit();
static void ShutdownCB()
{
	TerminateRequested = 1;
	ShutdownRequested = 1;
}
static void ResetCB()
{
	TerminateRequested = 1;
	ResetRequested = 1;
}
void ShutdownWii()
{
	TerminateRequested = 0;
	SDL_Quit();
	SYS_ResetSystem(SYS_POWEROFF, 0, 0);
}
void RestartHomebrewChannel()
{
	TerminateRequested = 0;
	SDL_Quit();
	exit(1);
}
void Terminate()
{
	if (ShutdownRequested) ShutdownWii();
	else if (ResetRequested) RestartHomebrewChannel();
}
#endif


/* Do initialisation which has to be done first for the console to work */
/* Entry point */
int main(int argc, char *argv[])
{
#ifdef HW_RVL
	// Wii Power/Reset buttons
	WPAD_Init();
	WPAD_SetPowerButtonCallback((WPADShutdownCallback)ShutdownCB);
	SYS_SetPowerCallback(ShutdownCB);
	SYS_SetResetCallback(ResetCB);
#endif
	PAD_Init();
	WII_InitVideoSystem();
#ifdef HW_RVL
	WPAD_SetDataFormat(0, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(0, 640, 480);

	MOUSE_Init();
	KEYBOARD_Init(NULL);
#endif
	/* Call the user's main function */
	return(SDL_main(argc, argv));
}

/* This function isn't implemented */
/*int unlink(const char* file_name)
{
	return -1;
}
*/
