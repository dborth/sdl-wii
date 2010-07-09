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
    slouken at libsdl.or
*/
#include "SDL_config.h"

// Public includes.
#include "SDL_timer.h"

// Audio internal includes.
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_sysaudio.h"
#include "../SDL_audio_c.h"

// Wii audio internal includes.
#include <ogcsys.h>
#include <ogc/audio.h>
#include <ogc/cache.h>
#include "SDL_wiiaudio.h"

#define SAMPLES_PER_DMA_BUFFER (512)

static const char WIIAUD_DRIVER_NAME[] = "wii";
static Uint32 dma_buffers[2][SAMPLES_PER_DMA_BUFFER*8] __attribute__((aligned(32)));
static int dma_buffers_size[2] = { SAMPLES_PER_DMA_BUFFER*4, SAMPLES_PER_DMA_BUFFER*4 };
static Uint8 whichab = 0;

#define AUDIOSTACK 16384*2
static lwpq_t audioqueue;
static lwp_t athread = LWP_THREAD_NULL;
static Uint8 astack[AUDIOSTACK];
static bool stopaudio = false;
static int currentfreq;

/****************************************************************************
 * Audio Threading
 ***************************************************************************/
static void *
AudioThread (void *arg)
{
	while (1)
	{
		if(stopaudio)
			break;

		memset(dma_buffers[whichab], 0, SAMPLES_PER_DMA_BUFFER*4);

		// Is the device ready?
		if (!current_audio || current_audio->paused)
		{
			DCFlushRange(dma_buffers[whichab], SAMPLES_PER_DMA_BUFFER*4);
			dma_buffers_size[whichab] = SAMPLES_PER_DMA_BUFFER*4;
		}
		else if (current_audio->convert.needed) // Is conversion required?
		{
			SDL_mutexP(current_audio->mixer_lock);
			// Get the client to produce audio
			current_audio->spec.callback(
				current_audio->spec.userdata,
				current_audio->convert.buf,
				current_audio->convert.len);
			SDL_mutexV(current_audio->mixer_lock);

			// Convert the audio
			SDL_ConvertAudio(&current_audio->convert);

			// Copy from SDL buffer to DMA buffer
			memcpy(dma_buffers[whichab], current_audio->convert.buf, current_audio->convert.len_cvt);
			DCFlushRange(dma_buffers[whichab], current_audio->convert.len_cvt);
			dma_buffers_size[whichab] = current_audio->convert.len_cvt;
		}
		else
		{
			SDL_mutexP(current_audio->mixer_lock);
			current_audio->spec.callback(
				current_audio->spec.userdata,
				(Uint8 *)dma_buffers[whichab],
				SAMPLES_PER_DMA_BUFFER*4);
			DCFlushRange(dma_buffers[whichab], SAMPLES_PER_DMA_BUFFER*4);
			dma_buffers_size[whichab] = SAMPLES_PER_DMA_BUFFER*4;
			SDL_mutexV(current_audio->mixer_lock);
		}
		LWP_ThreadSleep (audioqueue);
	}
	return NULL;
}

/****************************************************************************
 * DMACallback
 * Playback audio and signal audio thread that more samples are required
 ***************************************************************************/
static void
DMACallback()
{
	whichab ^= 1;
	AUDIO_InitDMA ((Uint32)dma_buffers[whichab], dma_buffers_size[whichab]);
	LWP_ThreadSignal (audioqueue);
}

void WII_AudioStop()
{
	AUDIO_StopDMA ();
	AUDIO_RegisterDMACallback(0);
	stopaudio = true;
	LWP_ThreadSignal(audioqueue);
	LWP_JoinThread(athread, NULL);
	LWP_CloseQueue (audioqueue);
	athread = LWP_THREAD_NULL;
}

void WII_AudioStart()
{
	if (currentfreq == 32000)
		AUDIO_SetDSPSampleRate(AI_SAMPLERATE_32KHZ);
	else
		AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);

	// startup conversion thread
	stopaudio = false;
	LWP_InitQueue (&audioqueue);
	LWP_CreateThread (&athread, AudioThread, NULL, astack, AUDIOSTACK, 67);

	// Start the first chunk of audio playing
	AUDIO_RegisterDMACallback(DMACallback);
	DMACallback();
	AUDIO_StartDMA();
}

static int WIIAUD_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	if (spec->freq != 32000 && spec->freq != 48000)
		spec->freq = 32000;

	// Set up actual spec.
	spec->format	= AUDIO_S16MSB;
	spec->channels	= 2;
	spec->samples	= SAMPLES_PER_DMA_BUFFER;
	spec->padding	= 0;
	SDL_CalculateAudioSpec(spec);

	memset(dma_buffers[0], 0, sizeof(dma_buffers[0]));
	memset(dma_buffers[1], 0, sizeof(dma_buffers[0]));

	currentfreq = spec->freq;
	WII_AudioStart();

	return 1;
}

void static WIIAUD_WaitAudio(_THIS)
{

}

static void WIIAUD_PlayAudio(_THIS)
{

}

static Uint8 *WIIAUD_GetAudioBuf(_THIS)
{
	return NULL;
}

static void WIIAUD_CloseAudio(_THIS)
{
	// Stop any DMA going on
	AUDIO_StopDMA();

	// terminate conversion thread
	LWP_ThreadSignal(audioqueue);
}

static void WIIAUD_DeleteDevice(SDL_AudioDevice *device)
{
	// Forget the DMA callback
	AUDIO_RegisterDMACallback(0);

	// Stop any DMA going on
	AUDIO_StopDMA();

	// terminate conversion thread
	LWP_ThreadSignal(audioqueue);

	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_AudioDevice *WIIAUD_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;

	/* Initialize all variables that we clean on shutdown */
	this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
	if ( this ) {
		SDL_memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateAudioData *)
				SDL_malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			SDL_free(this);
		}
		return(0);
	}
	SDL_memset(this->hidden, 0, (sizeof *this->hidden));

	// Initialise the Wii side of the audio system
	AUDIO_Init(0);

	/* Set the function pointers */
	this->OpenAudio = WIIAUD_OpenAudio;
	this->WaitAudio = WIIAUD_WaitAudio;
	this->PlayAudio = WIIAUD_PlayAudio;
	this->GetAudioBuf = WIIAUD_GetAudioBuf;
	this->CloseAudio = WIIAUD_CloseAudio;
	this->free = WIIAUD_DeleteDevice;

	return this;
}

static int WIIAUD_Available(void)
{
	return 1;
}

AudioBootStrap WIIAUD_bootstrap = {
	WIIAUD_DRIVER_NAME, "SDL Wii audio driver",
	WIIAUD_Available, WIIAUD_CreateDevice
};
