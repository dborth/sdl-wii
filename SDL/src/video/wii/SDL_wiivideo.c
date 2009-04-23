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

// Standard includes.
#include <math.h>

// SDL internal includes.
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"

// SDL Wii specifics.
#include "SDL_wiivideo.h"
#include "SDL_wiievents_c.h"

static const char	WIIVID_DRIVER_NAME[] = "wii";
static unsigned int	y_gamma[256];

static SDL_Rect mode_320;
static SDL_Rect mode_640;

static SDL_Rect* modes_descending[] =
{
	&mode_640,
	&mode_320,
	NULL
};

int WII_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	unsigned int y;

	// Set up the modes.
	mode_640.w = display_mode->fbWidth;
	mode_640.h = display_mode->xfbHeight;
	mode_320.w = mode_640.w / 2;
	mode_320.h = mode_640.h / 2;

	// Set the current format.
	vformat->BitsPerPixel	= 8;
	vformat->BytesPerPixel	= 1;

	// Initialise the gamma table.
	for (y = 0; y < 256; ++y)
	{
		y_gamma[y] = 255.0f * powf(y / 255.0f, 0.5f);
	}

	/* We're done! */
	return(0);
}

SDL_Rect **WII_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	return &modes_descending[0];
}

static inline Wii_Y1CBY2CR PackY1CBY2CR(unsigned int y1, unsigned int cb, unsigned int y2, unsigned int cr)
{
	return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}

static inline unsigned int CalculateY(unsigned int r, unsigned int g, unsigned int b)
{
	const unsigned int y = (299 * r + 587 * g + 114 * b) / 1000;
	return y_gamma[y];
}

static inline unsigned int CalculateCB(unsigned int r, unsigned int g, unsigned int b)
{
	return (-16874 * r - 33126 * g + 50000 * b + 12800000) / 100000;
}

static inline unsigned int CalculateCR(unsigned int r, unsigned int g, unsigned int b)
{
	return (50000 * r - 41869 * g - 8131 * b + 12800000) / 100000;
}

static void UpdateRow_8_1(const void* const src_start, const void* const src_end, Wii_Y1CBY2CR* const dst_start, const Wii_PackedPalette* palette)
{
	const Uint8*		src	= src_start;
	Wii_Y1CBY2CR*	dst	= dst_start;
	while (src != src_end)
	{
		const unsigned int left = *src++;
		const unsigned int right = *src++;
		*dst++ = palette->entries[left][right];
	}
}

static void UpdateRow_8_2(const void* const src_start, const void* const src_end, Wii_Y1CBY2CR* const dst_start, const Wii_PackedPalette* palette)
{
	const Uint8*		src	= src_start;
	Wii_Y1CBY2CR*	dst	= dst_start;
	while (src != src_end)
	{
		const unsigned int	left	= *src++;
		const unsigned int	right	= *src++;
		*dst++ = palette->entries[left][left];
		*dst++ = palette->entries[right][right];
	}
}

static inline Wii_Y1CBY2CR PackRGBs(
	unsigned int r1, unsigned int g1, unsigned int b1,
	unsigned int r2, unsigned int g2, unsigned int b2)
{
	const unsigned int cb1	= CalculateCB(r1, g1, b1);
	const unsigned int cb2	= CalculateCB(r2, g2, b2);
	const unsigned int cb	= (cb1 + cb2) >> 1;

	const unsigned int cr1	= CalculateCR(r1, g1, b1);
	const unsigned int cr2	= CalculateCR(r2, g2, b2);
	const unsigned int cr	= (cr1 + cr2) >> 1;

	return PackY1CBY2CR(CalculateY(r1, g1, b1), cb, CalculateY(r2, g2, b2), cr);
}

static void UpdateRow_32_1(const void* const src_start, const void* const src_end, Wii_Y1CBY2CR* const dst_start, const Wii_PackedPalette* palette)
{
	const Uint32*		src	= src_start;
	Wii_Y1CBY2CR*	dst	= dst_start;
	while (src != src_end)
	{
		const unsigned int left = *src++;
		const unsigned int r1 = left >> 24;
		const unsigned int g1 = (left >> 16) & 0xff;
		const unsigned int b1 = (left >> 8) & 0xff;
		const unsigned int right = *src++;
		const unsigned int r2 = right >> 24;
		const unsigned int g2 = (right >> 16) & 0xff;
		const unsigned int b2 = (right >> 8) & 0xff;
		*dst++ = PackRGBs(r1, g1, b1, r2, g2, b2);
	}
}

static inline Wii_Y1CBY2CR PackRGB(unsigned int r, unsigned int g, unsigned int b)
{
	const unsigned int y	= CalculateY(r, g, b);
	const unsigned int cb	= CalculateCB(r, g, b);
	const unsigned int cr	= CalculateCR(r, g, b);

	return PackY1CBY2CR(y, cb, y, cr);
}

static void UpdateRow_32_2(const void* const src_start, const void* const src_end, Wii_Y1CBY2CR* const dst_start, const Wii_PackedPalette* palette)
{
	const Uint32*		src	= src_start;
	Wii_Y1CBY2CR*	dst	= dst_start;
	while (src != src_end)
	{
		const unsigned int left = *src++;
		const unsigned int r1 = left >> 24;
		const unsigned int g1 = (left >> 16) & 0xff;
		const unsigned int b1 = (left >> 8) & 0xff;
		const unsigned int right = *src++;
		const unsigned int r2 = right >> 24;
		const unsigned int g2 = (right >> 16) & 0xff;
		const unsigned int b2 = (right >> 8) & 0xff;
		*dst++ = PackRGB(r1, g1, b1);
		*dst++ = PackRGB(r2, g2, b2);
	}
}

SDL_Surface *WII_SetVideoMode(_THIS, SDL_Surface *current,
								   int width, int height, int bpp, Uint32 flags)
{
	typedef struct ModeInfo
	{
		const SDL_Rect*			resolution;
		unsigned int			magnification;
		WII_UpdateRowFn*	update_row_8;
		WII_UpdateRowFn*	update_row_32;
	} ModeInfo;

	static const ModeInfo modes_ascending[] =
	{
		{ &mode_320, 2, &UpdateRow_8_2, &UpdateRow_32_2 },
		{ &mode_640, 1, &UpdateRow_8_1, &UpdateRow_32_1 },
		{ NULL }
	};

	const ModeInfo* 		mode;
	size_t					bytes_per_pixel;
	Uint32					r_mask;
	Uint32					b_mask;
	Uint32					g_mask;
	WII_UpdateRowFn*	update_row;

	/* Find a mode big enough to store the requested resolution */
	mode = &modes_ascending[0];
	while (mode->resolution)
	{
		if ((mode->resolution->w >= width) && (mode->resolution->h >= height))
		{
			break;
		}
		else
		{
			++mode;
		}
	}

	// Didn't find a mode?
	if (!mode->resolution)
	{
		SDL_SetError("Display mode (%dx%d) is too large to fit on the screen (%dx%d)",
			width, height, modes_descending[0]->w, modes_descending[0]->h);
		return NULL;
	}

	/* Handle the bpp */
	if (bpp <= 8)
	{
		bpp			= 8;
		r_mask		= 0;
		g_mask		= 0;
		b_mask		= 0;
		update_row	= mode->update_row_8;
	}
	else
	{
		bpp			= 32;
		r_mask		= 0xff000000;
		g_mask		= 0xff0000;
		b_mask		= 0xff00;
		update_row	= mode->update_row_32;
	}
	bytes_per_pixel = bpp / 8;


	// Free any existing buffer.
	if (this->hidden->back_buffer)
	{
		SDL_free(this->hidden->back_buffer);
		this->hidden->back_buffer = NULL;
	}

	if (this->hidden->tmp_buffer)
	{
		SDL_free(this->hidden->tmp_buffer);
		this->hidden->tmp_buffer = NULL;
	}


	// Allocate the new buffer.
	this->hidden->back_buffer = SDL_malloc(width * height * bytes_per_pixel);
	if (!this->hidden->back_buffer )
	{
		SDL_SetError("Couldn't allocate buffer for requested mode");
		return(NULL);
	}

	this->hidden->tmp_buffer = SDL_malloc((width+4) * (height+4) * bytes_per_pixel);
	if (!this->hidden->tmp_buffer )
	{
		SDL_free(this->hidden->back_buffer);
		this->hidden->back_buffer = NULL;
		SDL_SetError("Couldn't allocate buffer for requested mode");
		return(NULL);
	}

	

	/* Allocate the new pixel format for the screen */
	if (!SDL_ReallocFormat(current, bpp, r_mask, g_mask, b_mask, 0))
	{
		SDL_free(this->hidden->back_buffer);
		this->hidden->back_buffer = NULL;
		SDL_free(this->hidden->tmp_buffer);
		this->hidden->tmp_buffer = NULL;

		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return(NULL);
	}

	// Clear the back buffer.
	SDL_memset(this->hidden->back_buffer, 0, width * height * bytes_per_pixel);
	SDL_memset(this->hidden->tmp_buffer, 0, width * height * bytes_per_pixel);

	/* Set up the new mode framebuffer */
	//current->flags =  SDL_DOUBLEBUF |( flags & SDL_FULLSCREEN);
	current->flags =  flags & SDL_FULLSCREEN;
	current->w = width;
	current->h = height;
	current->pitch = current->w * bytes_per_pixel;
	current->pixels = this->hidden->back_buffer;

	/* Set the hidden data */
	this->hidden->update_row	= update_row;
	this->hidden->magnification	= mode->magnification;

#if 0
	// Clear the frame buffer.
	for (y = 0; y < display_mode->xfbHeight; ++y)
	{
		unsigned int x;
		for (x = 0; x < (display_mode->fbWidth / 2); ++x)
		{
			static const Wii_Y1CBY2CR black = PackRGB(0, 0, 0);
			(*frame_buffer)[y][x] = black;
		}
	}
#endif

	/* We're done */
	return(current);
}

/* We don't actually allow hardware surfaces other than the main one */
static int WII_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return(-1);
}

static void WII_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static int WII_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

static void WII_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}


static void WII_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	Wii_Y1CBY2CR	(*now)[][320]	= 0;	
	now = frame_buffer;
	if (frame_buffer==frame_buffer1) {
		frame_buffer = frame_buffer2;
		//this->hidden->back_buffer = frame_buffer1;
		//surface->pixels = frame_buffer1;
	}
	else {
		frame_buffer = frame_buffer1;
		//this->hidden->back_buffer = frame_buffer2;
		//surface->pixels = frame_buffer2;
	}

	VIDEO_SetNextFramebuffer(now);
	VIDEO_Flush();
	VIDEO_WaitVSync();

}

Uint32 blur(Uint32 now, Uint32 above, Uint32 below)
{
	const Uint32 R = 0xff000000;
	const Uint32 G = 0x00ff0000;
	const Uint32 B = 0x0000ff00;
	
	int  r = ((((now  & R)>>24)<<1) + ((above & R)>>24) + ((below & R)>>24))>>2;
	int  g = ((((now  & G)>>16)<<1) + ((above & G)>>16) + ((below & G)>>16))>>2;
	int  b = ((((now  & B)>>8)<<1) + ((above & B)>>8) + ((below & B)>>8))>>2;

	if (r>255) r = 255;
	if (g>255) g = 255;
	if (b>255) b = 255;
	return r << 24 | g <<16 | b <<8;
}

static void UpdateRect(_THIS, const SDL_Rect* const rect)
{
	const SDL_Surface* const screen = this->screen;

	if (screen)
	{
		// Constants.
		const unsigned int							x					= rect->x;
		const unsigned int							y					= rect->y;
		const unsigned int							w					= rect->w;
		const unsigned int							h					= rect->h;
		const struct SDL_PrivateVideoData* const	hidden				= this->hidden;
		WII_UpdateRowFn* const					update_row			= hidden->update_row;
		const unsigned int							magnification		= hidden->magnification;
		const Wii_PackedPalette* const			palette				= &hidden->packed_palette;
		const unsigned int							src_bytes_per_pixel	= screen->format->BytesPerPixel;
		const unsigned int							src_pitch			= screen->pitch;
		const Wii_Y1CBY2CR* const				last_dst_row		= &(*frame_buffer)[(y + h) * magnification][(x * magnification) / 2];

		// These variables change per row.
		const Uint8*		src_row_start			= &hidden->back_buffer[(y * src_pitch) + (x * src_bytes_per_pixel)];
		const Uint8* 		src_row_end				= src_row_start + (w * src_bytes_per_pixel);
		Wii_Y1CBY2CR*	dst_row_start			= &(*frame_buffer)[y * magnification][(x * magnification) / 2];


		Uint32* src = (Uint32*)&hidden->back_buffer[(y * src_pitch) + (x * src_bytes_per_pixel)];

		Uint32* dst = (Uint32*)&hidden->tmp_buffer[(y * screen->w) + x ];


		if (src_bytes_per_pixel==4) {
			int i,j;			
			for (i=0; i<h; i++) {
				Uint32* srctmp = src;
				Uint32* dsttmp = dst;
				for (j=0; j<w; j++) {
					register int above = (i<=0)?0:src[-screen->w];
					register int below = (i>=screen->h-1)?0:src[screen->w];
					*dst = blur(*src,above,below);
					//*dst = *src;
					src++;
					dst++;
				}
				src = srctmp + screen->w;
				dst = dsttmp + screen->w;
			}
		}


		src_row_start			= &hidden->tmp_buffer[(y * screen->w) + (x)];
		src_row_end				= src_row_start + (w * src_bytes_per_pixel);

		// Update each row.
		while (dst_row_start != last_dst_row)
		{
			// Update this row.
			update_row(src_row_start, src_row_end, dst_row_start, palette);
			if (magnification == 1)
			{
				// No magnification, just move on to the next row.
				dst_row_start += 320;
			}
			else
			{
				unsigned int		i;
				Wii_Y1CBY2CR*	next_row = dst_row_start + 320;

				// Copy each magnified row.
				for (i = 1; i < magnification; ++i)
				{
					memcpy(next_row, dst_row_start, 320 * sizeof(Wii_Y1CBY2CR));
					next_row += 320;
				}

				// Move on to the next unmagnified row.
				dst_row_start = next_row;
			}

			// Move on to the next row.
			src_row_start += src_pitch;
			src_row_end += src_pitch;
		}
	}
}

static void WII_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	const SDL_Rect*			rect;
	const SDL_Rect* const	last_rect	= &rects[numrects];

	/* Update each rect */
	for (rect = rects; rect != last_rect; ++rect)
	{
		/* Because we pack pixels together we need to align the rect. */
		const Sint16	x1				= rect->x & ~1;
		const Sint16	x2				= (rect->x + rect->w + 1) & ~1;
		const SDL_Rect	aligned_rect	= {x1, rect->y, x2 - x1, rect->h};

		UpdateRect(this, &aligned_rect);
	}

}

int WII_SetColors(_THIS, int first_color, int color_count, SDL_Color *colors)
{
	const int						last_color		= first_color + color_count;
	Wii_Palette* const			palette			= &this->hidden->palette;
	Wii_PackedPalette* const	packed_palette	= &this->hidden->packed_palette;

	int	component;
	int	left;
	int	right;
	
	/* Build the YCBCR palette. */
	for (component = first_color; component != last_color; ++component)
	{
		const SDL_Color* const in = &colors[component - first_color];
		const unsigned int r	= in->r;
		const unsigned int g	= in->g;
		const unsigned int b	= in->b;
		(*palette)[component].y		= CalculateY(r, g, b);
		(*palette)[component].cb	= CalculateCB(r, g, b);
		(*palette)[component].cr	= CalculateCR(r, g, b);
	}

	/* Build the Y1CBY2CR palette from the YCBCR palette. */
	// @todo optimise - not all of the entries require recalculation.
	for (left = 0; left != 256; ++left)
	{
		const unsigned int	y1	= (*palette)[left].y;
		const unsigned int	cb1	= (*palette)[left].cb;
		const unsigned int	cr1	= (*palette)[left].cr;
		for (right = 0; right != 256; ++right)
		{
			const unsigned int	cb	= (cb1 + (*palette)[right].cb) >> 1;
			const unsigned int	cr	= (cr1 + (*palette)[right].cr) >> 1;
			(*packed_palette).entries[left][right] = PackY1CBY2CR(y1, cb, (*palette)[right].y, cr);
		}
	}

	return(1);
}

/* Note:  If we are terminated, this could be called in the middle of
another SDL video routine -- notably UpdateRects.
*/
void WII_VideoQuit(_THIS)
{
/*	if (this->screen->pixels != NULL)
	{
		SDL_free(this->screen->pixels);
		this->screen->pixels = NULL;
		}*/
}

static void WII_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *WII_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
			SDL_malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			SDL_free(device);
		}
		return(0);
	}
	SDL_memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	device->VideoInit = WII_VideoInit;
	device->ListModes = WII_ListModes;
	device->SetVideoMode = WII_SetVideoMode;
	device->CreateYUVOverlay = NULL;
	device->SetColors = WII_SetColors;
	device->UpdateRects = WII_UpdateRects;
	device->VideoQuit = WII_VideoQuit;
	device->AllocHWSurface = WII_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = WII_LockHWSurface;
	device->UnlockHWSurface = WII_UnlockHWSurface;
	//device->FlipHWSurface = WII_FlipHWSurface;
	device->FlipHWSurface = NULL;
	device->FreeHWSurface = WII_FreeHWSurface;
	device->SetCaption = NULL;
	device->SetIcon = NULL;
	device->IconifyWindow = NULL;
	device->GrabInput = NULL;
	device->GetWMInfo = NULL;
	device->InitOSKeymap = WII_InitOSKeymap;
	device->PumpEvents = WII_PumpEvents;

	device->free = WII_DeleteDevice;

	return device;
}

static int WII_Available(void)
{
	return(1);
}

VideoBootStrap WII_bootstrap = {
	WIIVID_DRIVER_NAME, "Wii video driver",
	WII_Available, WII_CreateDevice
};
