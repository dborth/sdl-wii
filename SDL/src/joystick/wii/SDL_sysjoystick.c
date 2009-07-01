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
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <math.h>

#define PI 					3.14159265f

#define MAX_GC_JOYSTICKS	4
#define MAX_WII_JOYSTICKS	4
#define MAX_JOYSTICKS		(MAX_GC_JOYSTICKS + MAX_WII_JOYSTICKS)

#define MAX_GC_AXES			6
#define MAX_GC_BUTTONS		8
#define	MAX_GC_HATS			1

#define MAX_WII_AXES		6
#define MAX_WII_BUTTONS		20
#define	MAX_WII_HATS		1

#define	JOYNAMELEN			10

#define AXIS_MIN	-32768  /* minimum value for axis coordinate */
#define AXIS_MAX	32767   /* maximum value for axis coordinate */

typedef struct joystick_paddata_t
{
	u16 prev_buttons;
	s8 stickX;
	s8 stickY;
	s8 substickX;
	s8 substickY;
	u8 triggerL;
	u8 triggerR;
}joystick_paddata;

typedef struct joystick_wpaddata_t
{
	u32 prev_buttons;
	s8 nunchuk_stickX;
	s8 nunchuk_stickY;
	s8 classicL_stickX;
	s8 classicL_stickY;
	s8 classicR_stickX;
	s8 classicR_stickY;
	u8 classic_triggerL;
	u8 classic_triggerR;
}joystick_wpaddata;

/* The private structure used to keep track of a joystick */
typedef struct joystick_hwdata_t
{
	int index;
	int type;
	union
	{
		joystick_paddata gamecube;
		joystick_wpaddata wiimote;
	};
} joystick_hwdata;

static const u32 sdl_buttons_wii[] =
{
	WPAD_BUTTON_A,
	WPAD_BUTTON_B,
	WPAD_BUTTON_1,
	WPAD_BUTTON_2,
	WPAD_BUTTON_MINUS,
	WPAD_BUTTON_PLUS,
	WPAD_BUTTON_HOME,
	WPAD_NUNCHUK_BUTTON_Z, /* 7 */
	WPAD_NUNCHUK_BUTTON_C, /* 8 */
	WPAD_CLASSIC_BUTTON_A, /* 9 */
	WPAD_CLASSIC_BUTTON_B,
	WPAD_CLASSIC_BUTTON_X,
	WPAD_CLASSIC_BUTTON_Y,
	WPAD_CLASSIC_BUTTON_FULL_L,
	WPAD_CLASSIC_BUTTON_FULL_R,
	WPAD_CLASSIC_BUTTON_ZL,
	WPAD_CLASSIC_BUTTON_ZR,
	WPAD_CLASSIC_BUTTON_MINUS,
	WPAD_CLASSIC_BUTTON_PLUS,
	WPAD_CLASSIC_BUTTON_HOME
};

static const u16 sdl_buttons_gc[] =
{
	PAD_BUTTON_A,
	PAD_BUTTON_B,
	PAD_BUTTON_X,
	PAD_BUTTON_Y,
	PAD_TRIGGER_Z,
	PAD_TRIGGER_R,
	PAD_TRIGGER_L,
	PAD_BUTTON_START
};

static int __jswpad_enabled = 1;
static int __jspad_enabled = 1;
static int __numwiijoysticks = 4;
static int __numgcjoysticks = 4;

/* Helpers to separate nunchuk vs classic buttons which share the
 * same scan codes. In particular, up on the classic controller is
 * the same as Z on the nunchuk. The numbers refer to the sdl_buttons_wii
 * list above. */
static int wii_button_is_nunchuk(int idx)
{
	return idx == 7 || idx == 8;
}

static int wii_button_is_classic(int idx)
{
	return idx >= 9;
}

/* Function to scan the system for joysticks.
 * This function should return the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return -1 on an unrecoverable fatal error.
 */
int SDL_SYS_JoystickInit(void)
{
	return 8;
}

static char joy_name[] = "Gamecube 0";

/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
	if((index < 4) && (__jswpad_enabled) && (index < __numwiijoysticks))
	sprintf(joy_name, "Wiimote %d", index);
	else if((index < 8) && (__jspad_enabled) && (index < (__numgcjoysticks + 4)) && (index> 3))
	sprintf(joy_name, "Gamecube %d", index);
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
	if((joystick->index < 4) && (__jswpad_enabled))
	{
		if(joystick->index < __numwiijoysticks)
		{
			((joystick_hwdata*)(joystick->hwdata))->index = joystick->index;
			((joystick_hwdata*)(joystick->hwdata))->type = 0;
			joystick->nbuttons = MAX_WII_BUTTONS;
			joystick->naxes = MAX_WII_AXES;
			joystick->nhats = MAX_WII_HATS;
		}
	}
	else if((joystick->index < 8) && (__jspad_enabled))
	{
		if(joystick->index < (__numgcjoysticks + 4))
		{
			((joystick_hwdata*)(joystick->hwdata))->index = joystick->index - 4;
			((joystick_hwdata*)(joystick->hwdata))->type = 1;
			joystick->nbuttons = MAX_GC_BUTTONS;
			joystick->naxes = MAX_GC_AXES;
			joystick->nhats = MAX_GC_HATS;
		}
	}
	return(0);
}

static s16 WPAD_Stick(u8 chan, u8 right, int axis)
{
	float mag = 0.0;
	float ang = 0.0;
	WPADData *data = WPAD_Data(chan);

	switch (data->exp.type)
	{
		case WPAD_EXP_NUNCHUK:
		case WPAD_EXP_GUITARHERO3:
			if (right == 0)
			{
				mag = data->exp.nunchuk.js.mag;
				ang = data->exp.nunchuk.js.ang;
			}
			break;

		case WPAD_EXP_CLASSIC:
			if (right == 0)
			{
				mag = data->exp.classic.ljs.mag;
				ang = data->exp.classic.ljs.ang;
			}
			else
			{
				mag = data->exp.classic.rjs.mag;
				ang = data->exp.classic.rjs.ang;
			}
			break;

		default:
			break;
	}

	/* calculate x/y value (angle need to be converted into radian) */
	if (mag > 1.0) mag = 1.0;
	else if (mag < -1.0) mag = -1.0;
	double val;

	if(axis == 0) // x-axis
		val = mag * sin((PI * ang)/180.0f);
	else // y-axis
		val = mag * cos((PI * ang)/180.0f);

	return (s16)(val * 128.0f);
}

static void _HandleWiiJoystickUpdate(SDL_Joystick* joystick)
{
	u32 buttons, prev_buttons, changed;
	u32 exp_type;
	struct expansion_t exp;
	int i, axis;
	joystick_hwdata *prev_state;

	buttons = WPAD_ButtonsHeld(joystick->index);

	if (WPAD_Probe(joystick->index, &exp_type) != 0)
		exp_type = WPAD_EXP_NONE;

	WPAD_Expansion(joystick->index, &exp);

	prev_state = (joystick_hwdata *)joystick->hwdata;
	prev_buttons = prev_state->wiimote.prev_buttons;
	changed = buttons ^ prev_buttons;

	if(exp_type == WPAD_EXP_CLASSIC) // classic controller
	{
		if(changed & (WPAD_CLASSIC_BUTTON_LEFT | WPAD_CLASSIC_BUTTON_RIGHT |
			WPAD_CLASSIC_BUTTON_DOWN | WPAD_CLASSIC_BUTTON_UP))
		{
			int hat = SDL_HAT_CENTERED;
			if(buttons & WPAD_CLASSIC_BUTTON_UP) hat |= SDL_HAT_UP;
			if(buttons & WPAD_CLASSIC_BUTTON_DOWN) hat |= SDL_HAT_DOWN;
			if(buttons & WPAD_CLASSIC_BUTTON_LEFT) hat |= SDL_HAT_LEFT;
			if(buttons & WPAD_CLASSIC_BUTTON_RIGHT) hat |= SDL_HAT_RIGHT;
			SDL_PrivateJoystickHat(joystick, 0, hat);
		}
	}
	else // wiimote
	{
		if(changed & (WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT | WPAD_BUTTON_DOWN | WPAD_BUTTON_UP))
		{
			int hat = SDL_HAT_CENTERED;
			if(buttons & WPAD_BUTTON_UP) hat |= SDL_HAT_LEFT;
			if(buttons & WPAD_BUTTON_DOWN) hat |= SDL_HAT_RIGHT;
			if(buttons & WPAD_BUTTON_LEFT) hat |= SDL_HAT_DOWN;
			if(buttons & WPAD_BUTTON_RIGHT) hat |= SDL_HAT_UP;
			SDL_PrivateJoystickHat(joystick, 0, hat);
		}
	}

	for(i = 0; i < (sizeof(sdl_buttons_wii) / sizeof(sdl_buttons_wii[0])); i++)
	{
		if ( (exp_type == WPAD_EXP_CLASSIC && wii_button_is_nunchuk(i)) ||
				(exp_type == WPAD_EXP_NUNCHUK && wii_button_is_classic(i)) )
			continue;

		if (changed & sdl_buttons_wii[i])
			SDL_PrivateJoystickButton(joystick, i,
				(buttons & sdl_buttons_wii[i]) ? SDL_PRESSED : SDL_RELEASED);
	}
	prev_state->wiimote.prev_buttons = buttons;

	if(exp_type == WPAD_EXP_CLASSIC)
	{
		axis = WPAD_Stick(joystick->index, 0, 0);
		if(prev_state->wiimote.classicL_stickX != axis)
		{
			s16 value;
			if (axis >= 128) value = AXIS_MAX;
			else if (axis <=-128) value = AXIS_MIN;
			else value = axis << 8;
			SDL_PrivateJoystickAxis(joystick, 0, value);
			prev_state->wiimote.classicL_stickX = axis;
		}
		axis = WPAD_Stick(joystick->index, 0, 1);
		if(prev_state->wiimote.classicL_stickY != axis)
		{
			s16 value;
			if (axis >= 128) value = AXIS_MAX;
			else if (axis <=-128) value = AXIS_MIN;
			else value = axis << 8;
			SDL_PrivateJoystickAxis(joystick, 1, -value);
			prev_state->wiimote.classicL_stickY = axis;
		}
		axis = WPAD_Stick(joystick->index, 1, 0);
		if(prev_state->wiimote.classicR_stickX != axis)
		{
			SDL_PrivateJoystickAxis(joystick, 2, axis << 8);
			prev_state->wiimote.classicR_stickX = axis;
		}
		axis = WPAD_Stick(joystick->index, 1, 1);
		if(prev_state->wiimote.classicR_stickY != axis)
		{
			SDL_PrivateJoystickAxis(joystick, 3, -(axis << 8));
			prev_state->wiimote.classicR_stickY = axis;
		}
		axis = exp.classic.r_shoulder;
		if(prev_state->wiimote.classic_triggerR != axis)
		{
			SDL_PrivateJoystickAxis(joystick, 4, axis << 8);
			prev_state->wiimote.classic_triggerR = axis;
		}
		axis = exp.classic.l_shoulder;
		if(prev_state->wiimote.classic_triggerL != axis)
		{
			SDL_PrivateJoystickAxis(joystick, 5, axis << 8);
			prev_state->wiimote.classic_triggerL = axis;
		}
	}
	else if(exp_type == WPAD_EXP_NUNCHUK)
	{
		axis = WPAD_Stick(joystick->index, 0, 0);
		if(prev_state->wiimote.nunchuk_stickX != axis)
		{
			s16 value;
			if (axis >= 128) value = AXIS_MAX;
			else if (axis <=-128) value = AXIS_MIN;
			else value = axis << 8;
			SDL_PrivateJoystickAxis(joystick, 0, value);
			prev_state->wiimote.nunchuk_stickX = axis;
		}
		axis = WPAD_Stick(joystick->index, 0, 1);
		if(prev_state->wiimote.nunchuk_stickY != axis)
		{
			s16 value;
			if (axis >= 128) value = AXIS_MAX;
			else if (axis <=-128) value = AXIS_MIN;
			else value = axis << 8;
			SDL_PrivateJoystickAxis(joystick, 1, -value);
			prev_state->wiimote.nunchuk_stickY = axis;
		}
	}
}

static void _HandleGCJoystickUpdate(SDL_Joystick* joystick)
{
	u16 buttons, prev_buttons, changed;
	int i;
	int axis;
	joystick_hwdata *prev_state;

	buttons = PAD_ButtonsHeld(joystick->index - 4);
	prev_state = (joystick_hwdata *)joystick->hwdata;
	prev_buttons = prev_state->gamecube.prev_buttons;
	changed = buttons ^ prev_buttons;

	if(changed & (PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT | PAD_BUTTON_DOWN | PAD_BUTTON_UP))
	{
		int hat = SDL_HAT_CENTERED;
		if(buttons & PAD_BUTTON_UP) hat |= SDL_HAT_UP;
		if(buttons & PAD_BUTTON_DOWN) hat |= SDL_HAT_DOWN;
		if(buttons & PAD_BUTTON_LEFT) hat |= SDL_HAT_LEFT;
		if(buttons & PAD_BUTTON_RIGHT) hat |= SDL_HAT_RIGHT;
		SDL_PrivateJoystickHat(joystick, 0, hat);
	}

	for(i = 0; i < (sizeof(sdl_buttons_gc) / sizeof(sdl_buttons_gc[0])); i++)
	{
		if (changed & sdl_buttons_gc[i])
			SDL_PrivateJoystickButton(joystick, i,
				(buttons & sdl_buttons_gc[i]) ? SDL_PRESSED : SDL_RELEASED);
	}
	prev_state->gamecube.prev_buttons = buttons;
	axis = PAD_StickX(joystick->index);
	if(prev_state->gamecube.stickX != axis)
	{
		SDL_PrivateJoystickAxis(joystick, 0, axis << 8);
		prev_state->gamecube.stickX = axis;
	}

	axis = PAD_StickY(joystick->index);
	if(prev_state->gamecube.stickY != axis)
	{
		SDL_PrivateJoystickAxis(joystick, 1, (-axis) << 8);
		prev_state->gamecube.stickY = axis;
	}

	axis = PAD_SubStickX(joystick->index);
	if(prev_state->gamecube.substickX != axis)
	{
		SDL_PrivateJoystickAxis(joystick, 2, axis << 8);
		prev_state->gamecube.substickX = axis;
	}

	axis = PAD_SubStickY(joystick->index);
	if(prev_state->gamecube.substickY != axis)
	{
		SDL_PrivateJoystickAxis(joystick, 3, axis << 8);
		prev_state->gamecube.substickY = axis;
	}

	axis = PAD_TriggerL(joystick->index);
	if(prev_state->gamecube.triggerL != axis)
	{
		SDL_PrivateJoystickAxis(joystick, 4, axis << 8);
		prev_state->gamecube.triggerL = axis;
	}

	axis = PAD_TriggerR(joystick->index);
	if(prev_state->gamecube.triggerR != axis)
	{
		SDL_PrivateJoystickAxis(joystick, 5, axis << 8);
		prev_state->gamecube.triggerR = axis;
	}
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */

void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	if(!joystick || !joystick->hwdata)
		return;

	WPAD_ScanPads();
	PAD_ScanPads();

	switch(((joystick_hwdata*)(joystick->hwdata))->type)
	{
		case 0:
		if(__jswpad_enabled)
		_HandleWiiJoystickUpdate(joystick);
		break;
		case 1:
		if(__jspad_enabled)
		_HandleGCJoystickUpdate(joystick);
		break;
		default:
		break;
	}
}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	if(!joystick || !joystick->hwdata) // joystick already closed
		return;

	SDL_free(joystick->hwdata);
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{

}

#endif /* SDL_JOYSTICK_DC */
