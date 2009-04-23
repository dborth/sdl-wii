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

/* This file provides a general interface for SDL to read and write
data sources.  It can easily be extended to files, memory, etc.
*/

#include "SDL_endian.h"
#include "SDL_rwops.h"

#ifdef __WII__

#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

static SDL_bool initialised = SDL_FALSE;

extern bool fatInitDefault(void);

static int SDLCALL wii_seek(SDL_RWops *context, int offset, int whence)
{
	int action;

	switch (whence)
	{
		case RW_SEEK_CUR:
			action = SEEK_CUR;
			break;
		case RW_SEEK_END:
			action = SEEK_END;
			break;
		case RW_SEEK_SET:
			action = SEEK_SET;
			break;
		default:
			SDL_Error(SDL_EFSEEK);
			return(-1);
	}

	if ( fseek(context->hidden.wii.fp, offset, action) == 0 ) {
		return ftell(context->hidden.wii.fp);
	} else {
		SDL_Error(SDL_EFSEEK);
		return(-1);
	}
}

static int SDLCALL wii_read(SDL_RWops *context, void *ptr, int size, int num)
{
	int bytes_read;
	bytes_read = fread(ptr, size, num, context->hidden.wii.fp);
	if ( bytes_read == -1) {
		SDL_Error(SDL_EFREAD);
		return(-1);
	}
	return(bytes_read);
}

static int SDLCALL wii_write(SDL_RWops *context, const void *ptr, int size, int num)
{
	int bytes_written;

	bytes_written = fwrite(ptr, size, num, context->hidden.wii.fp);
	if ( bytes_written != (size * num) ) {
		SDL_Error(SDL_EFWRITE);
		return(-1);
	}
	return(num);
}

static int SDLCALL wii_close(SDL_RWops *context)
{
	if ( context )
	{
		fclose(context->hidden.wii.fp);
		SDL_FreeRW(context);
	}
	return(0);
}

SDL_RWops *SDL_RWFromFile(const char *file, const char *mode)
{
	int stat_result;
	struct stat stat_info;
	FILE* fp;

	if (!file || !*file || !mode || !*mode)
	{
		SDL_SetError("SDL_RWFromFile(): No file or no mode specified");
		return NULL;
	}

	/* Initialise the SD library */
	if (!initialised)
	{
		fatInitDefault();
		initialised = SDL_TRUE;
	}

	/* Opening for reading? */
	memset(&stat_info, 0, sizeof(stat_info));
	if (mode[0] == 'r')
	{
		/* Find the file */
		stat_result = stat(file, &stat_info);
		if (stat_result != 0)
		{
			SDL_SetError("Couldn't find %s to get its length", file);
			return NULL;
		}
	}

	/* Open the file */
	fp = fopen(file, mode);
	if ( fp == NULL )
	{
		SDL_SetError("Couldn't open %s", file);
		return NULL;
	}
	else
	{
		SDL_RWops *rwops = SDL_AllocRW();
		if ( rwops != NULL )
		{
			rwops->seek = wii_seek;
			rwops->read = wii_read;
			rwops->write = wii_write;
			rwops->close = wii_close;
			rwops->hidden.wii.fp = fp;
			rwops->hidden.wii.size = stat_info.st_size;
		}
		return(rwops);
	}
}

#endif /* __WII__ */
