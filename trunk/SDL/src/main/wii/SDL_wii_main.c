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

static bool FindIOS(u32 ios)
{
	s32 ret;
	u32 n;

	u64 *titles = NULL;
	u32 num_titles=0;

	ret = ES_GetNumTitles(&num_titles);
	if (ret < 0)
		return false;

	if(num_titles < 1) 
		return false;

	titles = (u64 *)memalign(32, num_titles * sizeof(u64) + 32);
	if (!titles)
		return false;

	ret = ES_GetTitles(titles, num_titles);
	if (ret < 0)
	{
		free(titles);
		return false;
	}
		
	for(n=0; n < num_titles; n++)
	{
		if((titles[n] & 0xFFFFFFFF)==ios) 
		{
			free(titles); 
			return true;
		}
	}
    free(titles); 
	return false;
}
#endif

/* Do initialisation which has to be done first for the console to work */
/* Entry point */
int main(int argc, char *argv[])
{
#ifdef HW_RVL
	u32 version = IOS_GetVersion();

	if(version != 58)
	{
		if(FindIOS(58))
			IOS_ReloadIOS(58);
		else if((version < 61 || version >= 200) && FindIOS(61))
			IOS_ReloadIOS(61);
	}

	// Wii Power/Reset buttons
	WPAD_Init();
	WPAD_SetPowerButtonCallback((WPADShutdownCallback)ShutdownCB);
	SYS_SetPowerCallback(ShutdownCB);
	SYS_SetResetCallback(ResetCB);
#endif
	PAD_Init();
	WII_InitVideoSystem();
#ifdef HW_RVL
	WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL, 640, 480);

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
