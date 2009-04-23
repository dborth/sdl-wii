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

/* Globals */
GXRModeObj*			display_mode			= 0;
Wii_Y1CBY2CR	(*frame_buffer1)[][320]	= 0;
Wii_Y1CBY2CR	(*frame_buffer2)[][320]	= 0;
Wii_Y1CBY2CR	(*frame_buffer)[][320]	= 0;


extern void wii_keyboard_init();
extern void wii_mouse_init();

/* Do initialisation which has to be done first for the console to work */
static void WII_Init(void)
{
	//printf("WII_Init ENTER\n");

	/* Initialise the video system */
	VIDEO_Init();

	display_mode = VIDEO_GetPreferredMode(NULL);

	WPAD_Init();
	PAD_Init();

	WPAD_SetDataFormat(0, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(0, 640, 480);

	/* Allocate the frame buffer */
	frame_buffer1 = (Wii_Y1CBY2CR (*)[][320])(MEM_K0_TO_K1(SYS_AllocateFramebuffer(display_mode)));
	//frame_buffer2 = (Wii_Y1CBY2CR (*)[][320])(MEM_K0_TO_K1(SYS_AllocateFramebuffer(display_mode)));
	frame_buffer = frame_buffer1;

	/* Set up the video system with the chosen mode */
	VIDEO_Configure(display_mode);

	/* Set the frame buffer */
	VIDEO_SetNextFramebuffer(frame_buffer);

	// Show the screen.
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (display_mode->viTVMode & VI_NON_INTERLACE)
	{
		VIDEO_WaitVSync();
	}

	// Initialise the debug console.
	console_init(frame_buffer,20,20,display_mode->fbWidth,display_mode->xfbHeight,display_mode->fbWidth*VI_DISPLAY_PIX_SZ);

	USB_Initialize();
	wii_mouse_init(); //must be called first
	wii_keyboard_init();

	
	//printf("WII_Init EXIT\n");
}

/* Entry point */
int main(int argc, char *argv[])
{
	printf("main ENTER\n");

	/* Set up the screen mode */
	WII_Init();

	/* Call the user's main function */
	return(SDL_main(argc, argv));
	
	printf("main EXIT\n");
}

/* This function isn't implemented */
/*int unlink(const char* file_name)
{
	return -1;
}
*/
