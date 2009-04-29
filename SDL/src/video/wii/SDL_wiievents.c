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

#include "SDL_wiivideo.h"
#include "SDL_wiievents_c.h"

int lastX = 0;
int lastY = 0;
Uint8 lastButtonStateA = SDL_RELEASED;
Uint8 lastButtonStateB = SDL_RELEASED;

Uint8 lastButtonStateLeftMouse = SDL_RELEASED;
Uint8 lastButtonStateRightMouse = SDL_RELEASED;

#define USB_CLASS_HID				0x03
#define USB_SUBCLASS_BOOT			0x01
#define USB_PROTOCOL_KEYBOARD		0x01
#define USB_PROTOCOL_MOUSE			0x02

static SDLKey keymap[512];

static s32 stat;
static s32 mstat;

typedef struct
{
	u32 message;
	u32 id; // direction
	u8 modifiers;
	u8 unknown;
	u8 keys[6];
	u8 pad[16];
} key_data_t;


static key_data_t key_data ATTRIBUTE_ALIGN(32);

static u8 prev_keys[6];
static u8 prev_modifiers;

static key_data_t key_data1,key_data2;

static int keyboard_kb=-1;

static int keyboard_stop = 1;

typedef enum
{
	KEYBOARD_PRESSED = 0,
	KEYBOARD_RELEASED,
	KEYBOARD_DISCONNECTED,
	KEYBOARD_CONNECTED
}keyboard_eventType;


typedef struct _KeyboardEvent{
	int type;
	int modifiers; /* actually, could as well scrap this. */
	int scancode;
} keyboardEvent;

typedef struct _MouseEvent{
	u8 button;
	int rx;
	int ry;
} mouseEvent;



static keyboardEvent ke;
static mouseEvent me;

typedef struct _node
{
	lwp_node node;
	keyboardEvent event;
}node;

typedef struct _mousenode
{
	lwp_node mousenode;
	mouseEvent event;
}mousenode;


lwp_queue *queue;
lwp_queue *mousequeue;


static int mouse_initialized =0;

static int mouse_vid = 0;
static int mouse_pid = 0;
static s32 mousefd=0;
static signed char *mousedata = 0;
#define DEVLIST_MAXSIZE				0x08

//Add an event to the event queue
s32 KEYBOARD_addEvent(int type, int scancode, int modifiers)
{
	node *n = (node *)malloc(sizeof(node));
	n->event.type = type;
	n->event.scancode = scancode;
	n->event.modifiers= modifiers;
	__lwp_queue_append(queue,(lwp_node*)n);
	return 1;
}

//Add an event to the event queue
s32 MOUSE_addEvent(u8 button, int rx, int ry)
{
	mousenode *n = (mousenode *)malloc(sizeof(mousenode));
	n->event.button = button;
	n->event.rx = rx;
	n->event.ry= ry;
	__lwp_queue_append(mousequeue,(lwp_node*)n);
	return 1;
}

//Get the first event of the event queue
s32 KEYBOARD_getEvent(keyboardEvent* event)
{
	node *n = (node*) __lwp_queue_get(queue);
	if (!n)
		return 0;
	*event = n->event;
	return 1;
}


//Get the first event of the event queue
s32 MOUSE_getEvent(mouseEvent* event)
{
	mousenode *n = (mousenode*) __lwp_queue_get(mousequeue);
	if (!n)
		return 0;
	*event = n->event;
	return 1;
}

/* index = bit# from the Wii. value = SDL keycode. */
static int modifier_keycodes[] = { SDLK_LCTRL, SDLK_LSHIFT, SDLK_LALT, SDLK_LMETA, SDLK_RCTRL, SDLK_RSHIFT, SDLK_RALT, SDLK_RMETA };

s32 keyboard_callback(int ret,void * none)
{
	int i, j;
	SDLKey key;

	if (keyboard_kb >= 0)
	{
		if (key_data.message != 0x7fffffff)
		{
			if (key_data.message == 2)
			{
				for (i = 0; i < 6; i++)
				{
					if (key_data.keys[i])
					{
						int found = false;
						for (j = 0; j < 6; j++)
						{
							if (prev_keys[j] == key_data.keys[i])
							{
								found = true;
								break;
							}
						}
						if (!found)
							KEYBOARD_addEvent(KEYBOARD_PRESSED,
									keymap[key_data.keys[i]],
									key_data.modifiers);
					}
				}

				for (i = 0; i < 6; i++)
				{
					if (prev_keys[i])
					{
						int found = false;
						for (j = 0; j < 6; j++)
						{
							if (prev_keys[i] == key_data.keys[j])
							{
								found = true;
								break;
							}
						}
						if (!found)
							KEYBOARD_addEvent(KEYBOARD_RELEASED,
									keymap[prev_keys[i]], key_data.modifiers);
					}
				}

				if (prev_modifiers != key_data.modifiers)
				{
					for (i = 0; i < sizeof(modifier_keycodes)
							/ sizeof(modifier_keycodes[0]); ++i)
					{
						key = modifier_keycodes[i];
						j = 1 << i; /* bit mask for bit# i */

						if ((key_data.modifiers & j) != 0 && (prev_modifiers
								& j) == 0)
						{ /* newly pressed. */
							KEYBOARD_addEvent(KEYBOARD_PRESSED, key, 0);
						}
						else if ((key_data.modifiers & j) == 0
								&& (prev_modifiers & j) != 0)
						{ /* newly released. */
							KEYBOARD_addEvent(KEYBOARD_RELEASED, key, 0);
						}
						else
						{
							/* unchanged. */
						}
					}
				}

				memcpy(prev_keys, key_data.keys, 6);
				prev_modifiers = key_data.modifiers;
			}

			key_data.message = 0x7fffffff;
			if (!keyboard_stop)
				IOS_IoctlAsync(keyboard_kb, 1, (void *) &key_data, 16,
						(void *) &key_data, 16, keyboard_callback, NULL);
		}
	}

	return 0;
}

void initkeymap()
{
	int i;


	for ( i=0; i<SDL_arraysize(keymap); ++i )
		keymap[i] = SDLK_UNKNOWN;
	//a-z
	for (i = 0; i<27; i++) {
		keymap[4+i] = SDLK_a + i;
	}

	//numbers
	for (i = 1; i<10; i++) {
		keymap[30 + i - 1] = SDLK_1 + i - 1;
	}
	keymap[39] = SDLK_0;
	keymap[40] = SDLK_RETURN;
	keymap[41] = SDLK_ESCAPE;
	keymap[42] = SDLK_BACKSPACE;
	keymap[43] = SDLK_TAB;
	keymap[44] = SDLK_SPACE;
	keymap[45] = SDLK_MINUS;
	keymap[46] = SDLK_EQUALS;
	keymap[47] = SDLK_LEFTBRACKET;
	keymap[47] = SDLK_RIGHTBRACKET;
	keymap[49] = SDLK_BACKSLASH;
	keymap[51] = SDLK_SEMICOLON;
	keymap[52] = SDLK_QUOTE;
	keymap[53] = SDLK_BACKQUOTE;
	keymap[54] = SDLK_COMMA;
	keymap[55] = SDLK_PERIOD;
	keymap[56] = SDLK_SLASH;
	keymap[57] = SDLK_CAPSLOCK;


	//F1 to F12
	for (i = 1; i<13; i++) {
		keymap[58 + i - 1] = SDLK_F1 + i - 1;
	}

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

	keymap[87] = SDLK_KP_ENTER;


	//keypad numbers
	for (i = 1; i<10; i++) {
		keymap[89 + i - 1] = SDLK_KP1 + i - 1;
	}
	keymap[98] = SDLK_KP0;
}

static int keyboard_initialized =0;

void wii_keyboard_init()
{
	if (!keyboard_initialized)
	{
		//printf("init keyboard");
		initkeymap();
		queue = (lwp_queue*) malloc(sizeof(lwp_queue));
		__lwp_queue_initialize(queue, 0, 0, 0);
		keyboard_kb = IOS_Open("/dev/usb/kbd", 1);
		/*
		 if (keyboard_kb>=0) {
		 printf("keyboard kb ok\n");
		 } else{
		 printf("keyboard kb not ok\n");
		 }*/
		sleep(2);
		key_data.message = 0x0;
		key_data1.id = 0;
		key_data2.id = 0;
		keyboard_stop = 0;
		if (keyboard_kb >= 0)
			IOS_IoctlAsync(keyboard_kb, 1, (void *) &key_data, 16,
					(void *) &key_data, 16, keyboard_callback, NULL);
		keyboard_initialized = 1;
	}
	else
	{
		if (keyboard_kb >= 0)
			IOS_IoctlAsync(keyboard_kb, 1, (void *) &key_data, 16,
					(void *) &key_data, 16, keyboard_callback, NULL);
	}
}

#define USB_REQ_GETPROTOCOL			0x03
#define USB_REQ_SETPROTOCOL			0x0B
#define USB_REQ_GETREPORT			0x01
#define USB_REQ_SETREPORT			0x09
#define USB_REPTYPE_INPUT			0x01
#define USB_REPTYPE_OUTPUT			0x02
#define USB_REPTYPE_FEATURE			0x03

#define USB_REQTYPE_GET				0xA1
#define USB_REQTYPE_SET				0x21


s32 mousecallback(s32 result,void *usrdata)
{
	if (result>0) {
		u8 button = mousedata[0];

		int x = mousedata[1];
		int y = mousedata[2];
		//int w = (-1)<<(data[3]-1);

		MOUSE_addEvent(button, x, y);

		USB_ReadIntrMsgAsync(mousefd, 0x81 ,4, mousedata, mousecallback, 0);
	} else {
		mouse_initialized = 0;
		mousefd =0;
	}
	return 0;
}

u8 mouseconfiguration;
u32 mouseinterface;
u32 mousealtInterface;

static int wii_find_mouse()
{
	s32 fd = 0;
	static u8 *buffer = 0;

	if (!buffer)
	{
		buffer = (u8*) memalign(32, DEVLIST_MAXSIZE << 3);
	}
	if (buffer == NULL)
	{
		return -1;
	}
	memset(buffer, 0, DEVLIST_MAXSIZE << 3);

	u8 dummy;
	u16 vid, pid;

	if (USB_GetDeviceList("/dev/usb/oh0", buffer, DEVLIST_MAXSIZE, 0, &dummy)
			< 0)
	{

		free(buffer);
		buffer = 0;
		return -2;
	}

	u8 mouseep;
	u32 mouseep_size;

	int i;

	for (i = 0; i < DEVLIST_MAXSIZE; i++)
	{
		memcpy(&vid, (buffer + (i << 3) + 4), 2);
		memcpy(&pid, (buffer + (i << 3) + 6), 2);

		if ((vid == 0) && (pid == 0))
			continue;
		fd = 0;

		int err = USB_OpenDevice("oh0", vid, pid, &fd);
		if (err < 0)
		{
			continue;
		}
		else
		{
			//fprintf(stderr, "ok open %04x:%04x\n", vid, pid);
			//SDL_Delay(1000);

		}

		u32 iConf, iInterface;
		usb_devdesc udd;
		usb_configurationdesc *ucd;
		usb_interfacedesc *uid;
		usb_endpointdesc *ued;

		USB_GetDescriptors(fd, &udd);

		for (iConf = 0; iConf < udd.bNumConfigurations; iConf++)
		{
			ucd = &udd.configurations[iConf];
			for (iInterface = 0; iInterface < ucd->bNumInterfaces; iInterface++)
			{
				uid = &ucd->interfaces[iInterface];

				if ((uid->bInterfaceClass == USB_CLASS_HID)
						&& (uid->bInterfaceSubClass == USB_SUBCLASS_BOOT)
						&& (uid->bInterfaceProtocol == USB_PROTOCOL_MOUSE))
				{
					int iEp;
					for (iEp = 0; iEp < uid->bNumEndpoints; iEp++)
					{
						ued = &uid->endpoints[iEp];
						mouse_vid = vid;
						mouse_pid = pid;

						mouseep = ued->bEndpointAddress;
						mouseep_size = ued->wMaxPacketSize;
						mouseconfiguration = ucd->bConfigurationValue;
						mouseinterface = uid->bInterfaceNumber;
						mousealtInterface = uid->bAlternateSetting;

					}
					break;
				}

			}
		}
		USB_FreeDescriptors(&udd);
		USB_CloseDevice(&fd);

	}
	if (mouse_pid != 0 || mouse_vid != 0)
		return 0;
	return -1;

}

void wii_mouse_init()
{
	if (!mouse_initialized)
	{

		if (!mousequeue)
		{
			mousequeue = (lwp_queue*) malloc(sizeof(lwp_queue));
			__lwp_queue_initialize(mousequeue, 0, 0, 0);
		}

		if (wii_find_mouse() != 0)
			return;

		if (USB_OpenDevice("oh0", mouse_vid, mouse_pid, &mousefd) < 0)
		{
			return;
		}
		if (!mousedata)
		{
			mousedata = (signed char*) memalign(32, 20);
		}

		//set boot protocol
		USB_WriteCtrlMsg(mousefd, USB_REQTYPE_SET, USB_REQ_SETPROTOCOL, 0, 0,
				0, 0);
		USB_ReadIntrMsgAsync(mousefd, 0x81, 4, mousedata, mousecallback, 0);

		mouse_initialized = 1;
	}
}

static int posted;

void PumpEvents()
{
	WPADData *wd = WPAD_Data(0);

	stat = KEYBOARD_getEvent(&ke);
	mstat = MOUSE_getEvent(&me);
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

	if (stat && (ke.type == KEYBOARD_RELEASED || ke.type == KEYBOARD_PRESSED))
	{
		SDL_keysym keysym;
		memset(&keysym, 0, sizeof(keysym));
		Uint8 keystate = (ke.type == KEYBOARD_PRESSED) ? SDL_PRESSED
				: SDL_RELEASED;
		keysym.sym = ke.scancode;
		keysym.mod = 0;
		/* to_SDL_Modifiers(ke.modifiers); don't waste effort,
		 SDL_PrivateKeyboard makes its own mind up about
		 them from the press/release events of the modifier
		 keys. */
		/*don't do this either. SDL_SetModState(to_SDL_Modifiers(ke.modifiers));*/
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

}
