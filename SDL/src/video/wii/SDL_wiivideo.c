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

// Standard includes.
#include <math.h>

// SDL internal includes.
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "SDL_timer.h"
#include "SDL_thread.h"

// SDL Wii specifics.
#include <gccore.h>
#include <ogcsys.h>
#include <malloc.h>
#include <ogc/texconv.h>
#include <wiiuse/wpad.h>
#include "SDL_wiivideo.h"
#include "SDL_wiievents_c.h"

static const char	WIIVID_DRIVER_NAME[] = "wii";
static SDL_Thread * videothread = 0;
static SDL_mutex * videomutex = 0;

/*** SDL ***/
static SDL_Rect mode_320;
static SDL_Rect mode_640;

static SDL_Rect* modes_descending[] =
{
	&mode_640,
	&mode_320,
	NULL
};

/*** 2D Video ***/
#define HASPECT 			320
#define VASPECT 			240
#define TEXTUREMEM_SIZE 	(640*480*4)

unsigned int *xfb[2] = { NULL, NULL }; // Double buffered
int whichfb = 0; // Switch
static GXRModeObj* vmode = 0;
static unsigned char texturemem[TEXTUREMEM_SIZE] __attribute__((aligned(32))); // GX texture
static unsigned char textureconvert[TEXTUREMEM_SIZE] __attribute__((aligned(32))); // 565 mem

/*** GX ***/
#define DEFAULT_FIFO_SIZE 256 * 1024
static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] __attribute__((aligned(32)));
static GXTexObj texobj;
static Mtx view;

/* New texture based scaler */
typedef struct tagcamera
{
	guVector pos;
	guVector up;
	guVector view;
}
camera;

/*** Square Matrix
     This structure controls the size of the image on the screen.
	 Think of the output as a -80 x 80 by -60 x 60 graph.
***/
static s16 square[] ATTRIBUTE_ALIGN (32) =
{
  /*
   * X,   Y,  Z
   * Values set are for roughly 4:3 aspect
   */
	-HASPECT,  VASPECT, 0,		// 0
	 HASPECT,  VASPECT, 0,	// 1
	 HASPECT, -VASPECT, 0,	// 2
	-HASPECT, -VASPECT, 0	// 3
};


static camera cam = {
	{0.0F, 0.0F, 0.0F},
	{0.0F, 0.5F, 0.0F},
	{0.0F, 0.0F, -0.5F}
};

/****************************************************************************
 * Scaler Support Functions
 ***************************************************************************/
static void
draw_init (SDL_Surface *current, int bpp)
{
	GX_ClearVtxDesc ();
	GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_CLR0, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_SetArray (GX_VA_POS, square, 3 * sizeof (s16));

	GX_SetNumTexGens (1);
	GX_SetNumChans (0);

	GX_SetTexCoordGen (GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);

	memset (&view, 0, sizeof (Mtx));
	guLookAt(view, &cam.pos, &cam.up, &cam.view);
	GX_LoadPosMtxImm (view, GX_PNMTX0);

	GX_InvVtxCache ();	// update vertex cache

	// initialize the texture obj we are going to use
	if (bpp == 8 || bpp == 16)
		GX_InitTexObj (&texobj, texturemem, current->w, current->h, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
	else
		GX_InitTexObj (&texobj, texturemem, current->w, current->h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

	GX_LoadTexObj (&texobj, GX_TEXMAP0);	// load texture object so its ready to use
}

static inline void
draw_vert (u8 pos, u8 c, f32 s, f32 t)
{
	GX_Position1x8 (pos);
	GX_Color1x8 (c);
	GX_TexCoord2f32 (s, t);
}

static inline void
draw_square (Mtx v)
{
	Mtx m;			// model matrix.
	Mtx mv;			// modelview matrix.

	guMtxIdentity (m);
	guMtxTransApply (m, m, 0, 0, -100);
	guMtxConcat (v, m, mv);

	GX_LoadPosMtxImm (mv, GX_PNMTX0);
	GX_Begin (GX_QUADS, GX_VTXFMT0, 4);
	draw_vert (0, 0, 0.0, 0.0);
	draw_vert (1, 0, 1.0, 0.0);
	draw_vert (2, 0, 1.0, 1.0);
	draw_vert (3, 0, 0.0, 1.0);
	GX_End ();
}

static int quit_flip_thread = 0;
int flip_thread(void * arg)
{
	while(1)
	{
		if (quit_flip_thread)
			break;

		whichfb ^= 1;

		SDL_mutexP(videomutex);

		// clear texture objects
		GX_InvVtxCache();
		GX_InvalidateTexAll();

		// load texture into GX
		DCFlushRange(texturemem, TEXTUREMEM_SIZE);

		GX_SetNumChans(1);
		GX_LoadTexObj(&texobj, GX_TEXMAP0);

		draw_square(view); // render textured quad
		GX_CopyDisp(xfb[whichfb], GX_TRUE);
		GX_DrawDone ();

		SDL_mutexV(videomutex);

		VIDEO_SetNextFramebuffer(xfb[whichfb]);
		VIDEO_Flush();
		VIDEO_WaitVSync();
	}
	return 0;
}

static void
SetupGX()
{
	Mtx44 p;
	int df = 1;
	
	GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
	GX_SetDispCopyYScale ((f32) vmode->xfbHeight / (f32) vmode->efbHeight);
	GX_SetScissor (0, 0, vmode->fbWidth, vmode->efbHeight);

	GX_SetDispCopySrc (0, 0, vmode->fbWidth, vmode->efbHeight);
	GX_SetDispCopyDst (vmode->fbWidth, vmode->xfbHeight);
	GX_SetCopyFilter (vmode->aa, vmode->sample_pattern, (df == 1) ? GX_TRUE : GX_FALSE, vmode->vfilter); // deflicker ON only for filtered mode

	GX_SetFieldMode (vmode->field_rendering, ((vmode->viHeight == 2 * vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
	GX_SetPixelFmt (GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	GX_SetDispCopyGamma (GX_GM_1_0);
	GX_SetCullMode (GX_CULL_NONE);
	GX_SetBlendMode(GX_BM_BLEND,GX_BL_DSTALPHA,GX_BL_INVSRCALPHA,GX_LO_CLEAR);

	GX_SetZMode (GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate (GX_TRUE);

	guOrtho(p, 480/2, -(480/2), -(640/2), 640/2, 100, 1000); // matrix, t, b, l, r, n, f
	GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);
}

static void
StartVideoThread()
{
	if(videothread == 0)
	{
		quit_flip_thread = 0;
		videothread = SDL_CreateThread(flip_thread, NULL);
	}
}

void
WII_InitVideoSystem()
{
	/* Initialise the video system */
	VIDEO_Init();
	vmode = VIDEO_GetPreferredMode(NULL);

	switch (vmode->viTVMode >> 2)
	{
		case VI_PAL: // 576 lines (PAL 50hz)
			// display should be centered vertically (borders)
			vmode = &TVPal574IntDfScale;
			vmode->xfbHeight = 480;
			vmode->viYOrigin = (VI_MAX_HEIGHT_PAL - 480)/2;
			vmode->viHeight = 480;
			break;

		case VI_NTSC: // 480 lines (NTSC 60hz)
			break;

		default: // 480 lines (PAL 60Hz)
			break;
	}

	/* Set up the video system with the chosen mode */
	VIDEO_Configure(vmode);

	// Allocate the video buffers
	xfb[0] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
	xfb[1] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));

	VIDEO_ClearFrameBuffer(vmode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer(vmode, xfb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer (xfb[0]);

	// Show the screen.
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (vmode->viTVMode & VI_NON_INTERLACE)
			VIDEO_WaitVSync();
		else
			while (VIDEO_GetNextField())
				VIDEO_WaitVSync();

	/*** Clear out FIFO area ***/
	memset (&gp_fifo, 0, DEFAULT_FIFO_SIZE);

	/*** Initialise GX ***/
	GX_Init (&gp_fifo, DEFAULT_FIFO_SIZE);

	GXColor background = { 0, 0, 0, 0xff };
	GX_SetCopyClear (background, 0x00ffffff);

	SetupGX();
}

int WII_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	// Set up the modes.
	mode_640.w = vmode->fbWidth;
	mode_640.h = vmode->xfbHeight;
	mode_320.w = mode_640.w / 2;
	mode_320.h = mode_640.h / 2;

	// Set the current format.
	vformat->BitsPerPixel	= 16;
	vformat->BytesPerPixel	= 2;

	this->hidden->buffer = NULL;
	this->hidden->width = 0;
	this->hidden->height = 0;
	this->hidden->pitch = 0;

	/* We're done! */
	return(0);
}

SDL_Rect **WII_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	return &modes_descending[0];
}

SDL_Surface *WII_SetVideoMode(_THIS, SDL_Surface *current,
								   int width, int height, int bpp, Uint32 flags)
{
	SDL_Rect* 		mode;
	size_t			bytes_per_pixel;
	Uint32			r_mask = 0;
	Uint32			b_mask = 0;
	Uint32			g_mask = 0;

	// Find a mode big enough to store the requested resolution
	mode = modes_descending[0];
	while (mode)
	{
		if (mode->w == width && mode->h == height)
			break;
		else
			++mode;
	}

	// Didn't find a mode?
	if (!mode)
	{
		SDL_SetError("Display mode (%dx%d) is unsupported.",
			width, height);
		return NULL;
	}

	if(bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32)
	{
		SDL_SetError("Resolution (%d bpp) is unsupported (8/16/24/32 bpp only).",
			bpp);
		return NULL;
	}

	bytes_per_pixel = bpp / 8;

	// Free any existing buffer.
	if (this->hidden->buffer)
	{
		free(this->hidden->buffer);
		this->hidden->buffer = NULL;
	}

	// Allocate the new buffer.
	this->hidden->buffer = memalign(32, width * height * bytes_per_pixel);
	if (!this->hidden->buffer )
	{
		SDL_SetError("Couldn't allocate buffer for requested mode");
		return(NULL);
	}

	// Allocate the new pixel format for the screen
	if (!SDL_ReallocFormat(current, bpp, r_mask, g_mask, b_mask, 0))
	{
		free(this->hidden->buffer);
		this->hidden->buffer = NULL;

		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return(NULL);
	}

	// Clear the buffer
	SDL_memset(this->hidden->buffer, 0, width * height * bytes_per_pixel);

	// Set up the new mode framebuffer
	current->flags = (flags & SDL_DOUBLEBUF) | (flags & SDL_FULLSCREEN) | (flags & SDL_HWPALETTE);
	current->w = width;
	current->h = height;
	current->pitch = current->w * bytes_per_pixel;
	current->pixels = this->hidden->buffer;

	/* Set the hidden data */
	this->hidden->width = current->w;
	this->hidden->height = current->h;
	this->hidden->pitch = current->pitch;

	draw_init(current, bpp);
	StartVideoThread();
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

static inline void Set_RGBAPixel(_THIS, int x, int y, u32 color)
{
	u8 *truc = (u8*) texturemem;
	int width = this->hidden->width;
	u32 offset;

	offset = (((y >> 2) << 4) * width) + ((x >> 2) << 6) + (((y % 4 << 2) + x % 4) << 1);

	*(truc + offset) = color & 0xFF;
	*(truc + offset + 1) = (color >> 24) & 0xFF;
	*(truc + offset + 32) = (color >> 16) & 0xFF;
	*(truc + offset + 33) = (color >> 8) & 0xFF;
}

static inline void Set_RGB565Pixel(_THIS, int x, int y, u16 color)
{
	u8 *truc = (u8*) texturemem;
	int width = this->hidden->width;
	u32 offset;

	offset = (((y >> 2) << 3) * width) + ((x >> 2) << 5) + (((y % 4 << 2) + x % 4) << 1);

	*(truc + offset) = (color >> 8) & 0xFF;
	*(truc + offset + 1) = color & 0xFF;
}

static void UpdateRect_8(_THIS, SDL_Rect *rect)
{
	u8 *src;
	u8 *ptr;
	u16 color;
	int i, j;
	Uint16 *palette = this->hidden->palette;
	for (i = 0; i < rect->h; i++)
	{
		src = (this->hidden->buffer + (this->hidden->width * (i + rect->y)) + (rect->x));
		for (j = 0; j < rect->w; j++)
		{
			ptr = src + j;
			color = palette[*ptr];
			Set_RGB565Pixel(this, rect->x + j, rect->y + i, color);
		}
	}
}

static void UpdateRect_16(_THIS, SDL_Rect *rect)
{
	u8 *src;
	u8 *ptr;
	u16 color;
	int i, j;
	for (i = 0; i < rect->h; i++)
	{
		src = (this->hidden->buffer + (this->hidden->width * 2 * (i + rect->y)) + (rect->x * 2));
		for (j = 0; j < rect->w; j++)
		{
			ptr = src + (j * 2);
			color = (ptr[0] << 8) | ptr[1];
			Set_RGB565Pixel(this, rect->x + j, rect->y + i, color);
		}
	}
}

static void UpdateRect_24(_THIS, SDL_Rect *rect)
{
	u8 *src;
	u8 *ptr;
	u32 color;
	int i, j;
	for (i = 0; i < rect->h; i++)
	{
		src = (this->hidden->buffer + (this->hidden->width * 3 * (i + rect->y)) + (rect->x * 3));
		for (j = 0; j < rect->w; j++)
		{
			ptr = src + (j * 3);
			color = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | 0xff;
			Set_RGBAPixel(this, rect->x + j, rect->y + i, color);
		}
	}
}

static void UpdateRect_32(_THIS, SDL_Rect *rect)
{
	u8 *src;
	u8 *ptr;
	u32 color;
	int i, j;
	for (i = 0; i < rect->h; i++)
	{
		src = (this->hidden->buffer + (this->hidden->width * 4 * (i + rect->y)) + (rect->x * 4));
		for (j = 0; j < rect->w; j++)
		{
			ptr = src + (j * 4);
			color = (ptr[1] << 24) | (ptr[2] << 16) | (ptr[3] << 8) | ptr[0];
			Set_RGBAPixel(this, rect->x + j, rect->y + i, color);
		}
	}
}

static void WII_UpdateRect(_THIS, SDL_Rect *rect)
{
	const SDL_Surface* const screen = this->screen;
	SDL_mutexP(videomutex);
	switch(screen->format->BytesPerPixel) {
	case 1:
		UpdateRect_8(this, rect);
		break;
	case 2:
		UpdateRect_16(this, rect);
		break;
	case 3:
		UpdateRect_24(this, rect);
		break;
	case 4:
		UpdateRect_32(this, rect);
		break;
	default:
		fprintf(stderr, "Invalid BPP %d\n", screen->format->BytesPerPixel);
		break;
	}
	SDL_mutexV(videomutex);
}

static void WII_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	int i;
	for (i = 0; i < numrects; i++)
	{
		WII_UpdateRect(this, &rects[i]);
	}
}

static void flipHWSurface_8_16(_THIS, SDL_Surface *surface)
{
	int new_pitch = this->hidden->width * 2;
	long long int *dst = (long long int *) texturemem;
	long long int *src1 = (long long int *) textureconvert;
	long long int *src2 = (long long int *) (textureconvert + new_pitch);
	long long int *src3 = (long long int *) (textureconvert + (new_pitch * 2));
	long long int *src4 = (long long int *) (textureconvert + (new_pitch * 3));
	int rowpitch = (new_pitch >> 3) * 3;
	int rowadjust = (new_pitch % 8) * 4;
	Uint16 *palette = this->hidden->palette;
	char *ra = NULL;
	int h, w;

	// crude convert
	Uint16 * ptr_cv = (Uint16 *) textureconvert;
	Uint8 *ptr = (Uint8 *)this->hidden->buffer;

	for (h = 0; h < this->hidden->height; h++)
	{
		for (w = 0; w < this->hidden->width; w++)
		{
			Uint16 v = palette[*ptr];

			*ptr_cv++ = v;
			ptr++;
		}
	}

	// same as 16bit
	for (h = 0; h < this->hidden->height; h += 4)
	{
		for (w = 0; w < (this->hidden->width >> 2); w++)
		{
			*dst++ = *src1++;
			*dst++ = *src2++;
			*dst++ = *src3++;
			*dst++ = *src4++;
		}

		src1 += rowpitch;
		src2 += rowpitch;
		src3 += rowpitch;
		src4 += rowpitch;

		if ( rowadjust )
		{
			ra = (char *)src1;
			src1 = (long long int *)(ra + rowadjust);
			ra = (char *)src2;
			src2 = (long long int *)(ra + rowadjust);
			ra = (char *)src3;
			src3 = (long long int *)(ra + rowadjust);
			ra = (char *)src4;
			src4 = (long long int *)(ra + rowadjust);
		}
	}
}

static void flipHWSurface_16_16(_THIS, SDL_Surface *surface)
{
	int h, w;
	long long int *dst = (long long int *) texturemem;
	long long int *src1 = (long long int *) this->hidden->buffer;
	long long int *src2 = (long long int *) (this->hidden->buffer + this->hidden->pitch);
	long long int *src3 = (long long int *) (this->hidden->buffer + (this->hidden->pitch * 2));
	long long int *src4 = (long long int *) (this->hidden->buffer + (this->hidden->pitch * 3));
	int rowpitch = (this->hidden->pitch >> 3) * 3;
	int rowadjust = (this->hidden->pitch % 8) * 4;
	char *ra = NULL;

	for (h = 0; h < this->hidden->height; h += 4)
	{
		for (w = 0; w < this->hidden->width; w += 4)
		{
			*dst++ = *src1++;
			*dst++ = *src2++;
			*dst++ = *src3++;
			*dst++ = *src4++;
		}

		src1 += rowpitch;
		src2 += rowpitch;
		src3 += rowpitch;
		src4 += rowpitch;

		if ( rowadjust )
		{
			ra = (char *)src1;
			src1 = (long long int *)(ra + rowadjust);
			ra = (char *)src2;
			src2 = (long long int *)(ra + rowadjust);
			ra = (char *)src3;
			src3 = (long long int *)(ra + rowadjust);
			ra = (char *)src4;
			src4 = (long long int *)(ra + rowadjust);
		}
	}
}

static void flipHWSurface_24_16(_THIS, SDL_Surface *surface)
{
	SDL_Rect screen_rect = {0, 0, this->hidden->width, this->hidden->height};
	WII_UpdateRect(this, &screen_rect);
}

static void flipHWSurface_32_16(_THIS, SDL_Surface *surface)
{
	SDL_Rect screen_rect = {0, 0, this->hidden->width, this->hidden->height};
	WII_UpdateRect(this, &screen_rect);
}

static int WII_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	switch(surface->format->BytesPerPixel)
	{
		case 1:
			flipHWSurface_8_16(this, surface);
			break;
		case 2:
			flipHWSurface_16_16(this, surface);
			break;
		case 3:
			flipHWSurface_24_16(this, surface);
			break;
		case 4:
			flipHWSurface_32_16(this, surface);
			break;
		default:
			return -1;
	}
	return 1;
}

int WII_SetColors(_THIS, int first_color, int color_count, SDL_Color *colors)
{
	const int last_color = first_color + color_count;
	Uint16* const palette = this->hidden->palette;
	int     component;

	/* Build the RGB565 palette. */
	for (component = first_color; component != last_color; ++component)
	{
		const SDL_Color* const in = &colors[component - first_color];
		const unsigned int r    = (in->r >> 3) & 0x1f;
		const unsigned int g    = (in->g >> 2) & 0x3f;
		const unsigned int b    = (in->b >> 3) & 0x1f;

		palette[component] = (r << 11) | (g << 5) | b;
	}

	return(1);
}

void WII_VideoStart()
{
	SetupGX();
	StartVideoThread();
}

void WII_VideoQuit(_THIS)
{
	quit_flip_thread = 1;
	SDL_WaitThread(videothread, NULL);
	videothread = 0;
	GX_AbortFrame();
	GX_Flush();

	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
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

	videomutex = SDL_CreateMutex();

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
	device->FlipHWSurface = WII_FlipHWSurface;
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
