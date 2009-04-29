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

extern void wii_keyboard_init();
extern void wii_mouse_init();

/* Do initialisation which has to be done first for the console to work */
/* Entry point */
int main(int argc, char *argv[])
{
	WPAD_Init();
	PAD_Init();
	WII_InitVideoSystem();
	WPAD_SetDataFormat(0, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(0, 640, 480);

	USB_Initialize();
	wii_mouse_init(); //must be called first
	wii_keyboard_init();

	/* Call the user's main function */
	return(SDL_main(argc, argv));
}

/* This function isn't implemented */
/*int unlink(const char* file_name)
{
	return -1;
}
*/
