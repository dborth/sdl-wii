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


    Yohanes Nugroho (yohanes@gmail.com)
	Tantric
*/
#include "SDL_config.h"

#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"
#include <wiiuse/wpad.h>
#include <malloc.h>
#include <unistd.h>
#include <ogc/usbmouse.h>
#include <wiikeyboard/keyboard.h>

#include "SDL_wiivideo.h"
#include "SDL_wiievents_c.h"

static int lastX = 0;
static int lastY = 0;
static Uint8 lastButtonStateA = SDL_RELEASED;
static Uint8 lastButtonStateB = SDL_RELEASED;

static SDLKey keymap[232];

static s32 stat;
static s32 mstat;

static keyboard_event ke;
static mouse_event me;

static int posted;

extern bool TerminateRequested;
extern void Terminate();

void PumpEvents()
{
#ifdef HW_RVL
	if (TerminateRequested) Terminate();
#endif
	WPADData *wd = WPAD_Data(0);

	stat = KEYBOARD_GetEvent(&ke);
	mstat = MOUSE_GetEvent(&me);
	int x, y;

	SDL_GetMouseState(&x, &y);

	if (wd->ir.valid)
	{
		x = wd->ir.x;
		y = wd->ir.y;

		if (lastX != x || lastY != y)
		{
			posted += SDL_PrivateMouseMotion(0, 0, x, y);
			lastX = x;
			lastY = y;
		}

		Uint8 stateA = SDL_RELEASED;
		Uint8 stateB = SDL_RELEASED;

		if (wd->btns_h & WPAD_BUTTON_A)
		{
			stateA = SDL_PRESSED;
		}
		if (wd->btns_h & WPAD_BUTTON_B)
		{
			stateB = SDL_PRESSED;
		}

		if (stateA != lastButtonStateA)
		{
			lastButtonStateA = stateA;
			posted += SDL_PrivateMouseButton(stateA, SDL_BUTTON_LEFT, x, y);
		}
		if (stateB != lastButtonStateB)
		{
			lastButtonStateB = stateB;
			posted += SDL_PrivateMouseButton(stateB, SDL_BUTTON_RIGHT, x, y);
		}
	}

	if (stat && (ke.type == KEYBOARD_RELEASED || ke.type == KEYBOARD_PRESSED))
	{
		SDL_keysym keysym;
		memset(&keysym, 0, sizeof(keysym));
		Uint8 keystate = (ke.type == KEYBOARD_PRESSED) ? SDL_PRESSED
				: SDL_RELEASED;
		keysym.sym = keymap[ke.keycode];
		keysym.mod = 0;
		posted += SDL_PrivateKeyboard(keystate, &keysym);
	}

	if (mstat)
	{
		posted += SDL_PrivateMouseMotion(me.button, 1, me.rx, me.ry);

		u8 button = me.button;

		if (button & 0x1)
		{
			if (!(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(1)))
			{
				posted += SDL_PrivateMouseButton(SDL_PRESSED, 1, 0, 0);
			}
		}
		else
		{
			if ((SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(1)))
			{
				posted += SDL_PrivateMouseButton(SDL_RELEASED, 1, 0, 0);
			}
		}

		if (button & 0x2)
		{
			if (!(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(3)))
			{
				posted += SDL_PrivateMouseButton(SDL_PRESSED, 3, 0, 0);
			}
		}
		else
		{
			if ((SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(3)))
			{
				posted += SDL_PrivateMouseButton(SDL_RELEASED, 3, 0, 0);
			}
		}
	}
}

void WII_PumpEvents(_THIS)
{
	do
	{
		posted = 0;
		PumpEvents();
		usleep(100);
	} while (posted);
}

void WII_InitOSKeymap(_THIS)
{
	int i;

	for (i = 0; i < 232; ++i)
		keymap[i] = SDLK_UNKNOWN;

	// a-z
	for (i = 0; i < 27; i++)
		keymap[4 + i] = SDLK_a + i;

	// 1-9
	for (i = 0; i < 9; i++)
		keymap[30 + i] = SDLK_1 + i;

	keymap[39] = SDLK_0;
	keymap[40] = SDLK_RETURN;
	keymap[41] = SDLK_ESCAPE;
	keymap[42] = SDLK_BACKSPACE;
	keymap[43] = SDLK_TAB;
	keymap[44] = SDLK_SPACE;
	keymap[45] = SDLK_MINUS;
	keymap[46] = SDLK_EQUALS;
	keymap[47] = SDLK_LEFTBRACKET;
	keymap[48] = SDLK_RIGHTBRACKET;
	keymap[49] = SDLK_BACKSLASH;
	keymap[50] = SDLK_UNKNOWN; // unused
	keymap[51] = SDLK_SEMICOLON;
	keymap[52] = SDLK_QUOTE;
	keymap[53] = SDLK_BACKQUOTE;
	keymap[54] = SDLK_COMMA;
	keymap[55] = SDLK_PERIOD;
	keymap[56] = SDLK_SLASH;
	keymap[57] = SDLK_CAPSLOCK;

	// F1 to F12
	for (i = 0; i < 12; i++)
		keymap[58 + i] = SDLK_F1 + i;

	keymap[70] = SDLK_PRINT;
	keymap[71] = SDLK_SCROLLOCK;
	keymap[72] = SDLK_PAUSE;
	keymap[73] = SDLK_INSERT;
	keymap[74] = SDLK_HOME;
	keymap[75] = SDLK_PAGEUP;
	keymap[76] = SDLK_DELETE;
	keymap[77] = SDLK_END;
	keymap[78] = SDLK_PAGEDOWN;

	keymap[79] = SDLK_RIGHT;
	keymap[80] = SDLK_LEFT;
	keymap[81] = SDLK_DOWN;
	keymap[82] = SDLK_UP;
	keymap[83] = SDLK_NUMLOCK;

	keymap[84] = SDLK_KP_DIVIDE;
	keymap[85] = SDLK_KP_MULTIPLY;
	keymap[86] = SDLK_KP_MINUS;
	keymap[87] = SDLK_KP_PLUS;

	keymap[88] = SDLK_KP_ENTER;

	// keypad 1-9
	for (i = 0; i < 9; i++)
		keymap[89 + i] = SDLK_KP1 + i;

	keymap[98] = SDLK_KP0;
	keymap[99] = SDLK_KP_PERIOD;
	keymap[100] = SDLK_UNKNOWN; // unused
	keymap[101] = SDLK_UNKNOWN; // Applic
	keymap[102] = SDLK_POWER;
	keymap[103] = SDLK_KP_EQUALS;
	keymap[104] = SDLK_F13;
	keymap[105] = SDLK_F14;
	keymap[106] = SDLK_F15;
	keymap[107] = SDLK_UNKNOWN; // F16
	keymap[108] = SDLK_UNKNOWN; // F17
	keymap[109] = SDLK_UNKNOWN; // F18
	keymap[110] = SDLK_UNKNOWN; // F19
	keymap[111] = SDLK_UNKNOWN; // F20
	keymap[112] = SDLK_UNKNOWN; // F21
	keymap[113] = SDLK_UNKNOWN; // F22
	keymap[114] = SDLK_UNKNOWN; // F23
	keymap[115] = SDLK_UNKNOWN; // F24
	keymap[116] = SDLK_UNKNOWN; // Execute
	keymap[117] = SDLK_HELP;
	keymap[118] = SDLK_MENU;
	keymap[119] = SDLK_UNKNOWN; // Select
	keymap[120] = SDLK_UNKNOWN; // Stop
	keymap[121] = SDLK_UNKNOWN; // Again
	keymap[122] = SDLK_UNDO;

	keymap[134] = SDLK_KP_EQUALS;
	// 135-143 - International
	// 144-152 - Language
	keymap[154] = SDLK_SYSREQ;

	keymap[224] = SDLK_LCTRL;
	keymap[225] = SDLK_LSHIFT;
	keymap[226] = SDLK_LALT;
	keymap[227] = SDLK_LMETA;
	keymap[228] = SDLK_RCTRL;
	keymap[229] = SDLK_RSHIFT;
	keymap[230] = SDLK_RALT;
	keymap[231] = SDLK_RMETA;
}
