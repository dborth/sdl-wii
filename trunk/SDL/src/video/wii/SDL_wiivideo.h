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

    Tantric, 2009
*/
#include "SDL_config.h"

#ifndef _SDL_wiivideo_h
#define _SDL_wiivideo_h

/* SDL internal includes */
#include "../SDL_sysvideo.h"

/* OGC includes */
#include <ogc/gx_struct.h>

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

/* Types */
typedef Uint16 Wii_Palette[256];

/* Private display data */
struct SDL_PrivateVideoData
{
	Uint8*					buffer;
	int						width;
	int						height;
	int						pitch;

	Wii_Palette             palette;
};

void WII_InitVideoSystem();

#endif /* _SDL_wiivideo_h */
