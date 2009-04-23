/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifdef SDL_JOYSTICK_WII

#include "SDL_events.h"
#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#include <gccore.h>

#define MAX_JOYSTICKS	4
#define MAX_AXES	6
#define MAX_BUTTONS	8
#define	MAX_HATS	1

#define	JOYNAMELEN	8

/* The private structure used to keep track of a joystick */
typedef struct joystick_hwdata_t {
	int prev_buttons;
	s8 stickX;
	s8 stickY;
	s8 substickX;
	s8 substickY;
	u8 triggerL;
	u8 triggerR;
} joystick_hwdata;



/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int SDL_SYS_JoystickInit(void)
{
	return 4;
}

static char joy_name[] = "port 0";

/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
	sprintf(joy_name, "port %d", index);
	return (const char *)joy_name;
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
	/* allocate memory for system specific hardware data */
	joystick->hwdata = SDL_malloc(sizeof(joystick_hwdata));
	if (joystick->hwdata == NULL)
	{
		SDL_OutOfMemory();
		return(-1);
	}
	SDL_memset(joystick->hwdata, 0, sizeof(joystick_hwdata));

	/* fill nbuttons, naxes, and nhats fields */
	joystick->nbuttons = MAX_BUTTONS;
	joystick->naxes = MAX_AXES;
	joystick->nhats = MAX_HATS;
	return(0);
}



/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */

void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	int buttons,prev_buttons,i,changed;
	joystick_hwdata *prev_state;

	const	int sdl_buttons[] = {
		PAD_BUTTON_A,			
		PAD_BUTTON_B,			
		PAD_BUTTON_X,			
		PAD_BUTTON_Y,			
		PAD_TRIGGER_Z,			
		PAD_TRIGGER_R,			
		PAD_TRIGGER_L,			
		PAD_BUTTON_START	    
	};

	if (!joystick) return;

	
	buttons = PAD_ButtonsHeld(joystick->index);
	
	prev_state = (joystick_hwdata *)joystick->hwdata;
	
	prev_buttons = prev_state->prev_buttons;
	changed = buttons^prev_buttons;

	if ((changed)&(PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_BUTTON_DOWN|PAD_BUTTON_UP)) {
		int hat = SDL_HAT_CENTERED;
		if (buttons&PAD_BUTTON_UP) hat|=SDL_HAT_UP;
		if (buttons&PAD_BUTTON_DOWN) hat|=SDL_HAT_DOWN;
		if (buttons&PAD_BUTTON_LEFT) hat|=SDL_HAT_LEFT;
		if (buttons&PAD_BUTTON_RIGHT) hat|=SDL_HAT_RIGHT;
		SDL_PrivateJoystickHat(joystick, 0, hat);
	}


	for(i=0;i<sizeof(sdl_buttons)/sizeof(sdl_buttons[0]);i++) {
		if (changed & sdl_buttons[i]) {
			SDL_PrivateJoystickButton(joystick, i, (buttons & sdl_buttons[i])?SDL_PRESSED:SDL_RELEASED);
		}
	}
	
	prev_state->prev_buttons = buttons;
	
	int axis = PAD_StickX(joystick->index);
	
	if ( prev_state->stickX != axis ) {
		SDL_PrivateJoystickAxis(joystick, 0, axis << 8 );
		prev_state->stickX = axis;
	}
	
	axis = PAD_StickY(joystick->index);

	if ( prev_state->stickY != axis ) {
		SDL_PrivateJoystickAxis(joystick, 1, (-axis) << 8);
		prev_state->stickY = axis;
	}

	axis = PAD_SubStickX(joystick->index);

	if ( prev_state->substickX != axis ) {
		SDL_PrivateJoystickAxis(joystick, 2, axis << 8);
		prev_state->substickX = axis;
	}

	axis = PAD_SubStickY(joystick->index);

	if ( prev_state->substickY != axis ) {
		SDL_PrivateJoystickAxis(joystick, 3, axis << 8);
		prev_state->substickY = axis;
	}

	axis = PAD_TriggerL(joystick->index);

	if ( prev_state->triggerL != axis ) {
		SDL_PrivateJoystickAxis(joystick, 4, axis << 8);
		prev_state->triggerL = axis;
	}

	axis = PAD_TriggerR(joystick->index);

	if ( prev_state->triggerR != axis ) {
		SDL_PrivateJoystickAxis(joystick, 5, axis << 8);
		prev_state->triggerR = axis;
	}

}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	if (joystick->hwdata != NULL) {
		/* free system specific hardware data */
		SDL_free(joystick->hwdata);
	}
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
	return;
}

#endif /* SDL_JOYSTICK_DC */
