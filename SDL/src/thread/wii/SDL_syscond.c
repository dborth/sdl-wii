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

/* An implementation of condition variables using semaphores and mutexes */
/*
   This implementation borrows heavily from the BeOS condition variable
   implementation, written by Christopher Tate and Owen Smith.  Thanks!
 */

#include "SDL_thread.h"
#include "SDL_sysmutex_c.h"

#include <ogcsys.h>

struct SDL_cond
{
	cond_t cond;
};

/* Create a condition variable */
SDL_cond * SDL_CreateCond(void)
{
	SDL_cond *cond;

	cond = (SDL_cond *) SDL_malloc(sizeof(SDL_cond));
	if ( cond ) {
		if (LWP_CondInit (&(cond->cond))<0) {
			SDL_DestroyCond(cond);
			cond = NULL;
		}
	} else {
		SDL_OutOfMemory();
	}
	return(cond);
}

/* Destroy a condition variable */
void SDL_DestroyCond(SDL_cond *cond)
{
	if ( cond ) {
		LWP_CondDestroy(cond->cond);
		SDL_free(cond);
	}
}

/* Restart one of the threads that are waiting on the condition variable */
int SDL_CondSignal(SDL_cond *cond)
{
	if ( ! cond ) {
		SDL_SetError("Passed a NULL condition variable");
		return -1;
	}

	return LWP_CondSignal(cond->cond)==0?0:-1;

}

/* Restart all threads that are waiting on the condition variable */
int SDL_CondBroadcast(SDL_cond *cond)
{
	if ( ! cond ) {
		SDL_SetError("Passed a NULL condition variable");
		return -1;
	}

	return LWP_CondBroadcast(cond->cond)==0?0:-1;
}

/* Wait on the condition variable for at most 'ms' milliseconds.
   The mutex must be locked before entering this function!
   The mutex is unlocked during the wait, and locked again after the wait.

Typical use:

Thread A:
	SDL_LockMutex(lock);
	while ( ! condition ) {
		SDL_CondWait(cond);
	}
	SDL_UnlockMutex(lock);

Thread B:
	SDL_LockMutex(lock);
	...
	condition = true;
	...
	SDL_UnlockMutex(lock);
 */

extern int clock_gettime(struct timespec *tp);

int SDL_CondWaitTimeout(SDL_cond *cond, SDL_mutex *mutex, Uint32 ms)
{
	int retval;
	struct timespec now;
	struct timespec abstime;

	if ( ! cond ) {
		SDL_SetError("Passed a NULL condition variable");
		return -1;
	}

	clock_gettime(&now);

	abstime.tv_sec = now.tv_sec + (ms/1000);
	abstime.tv_nsec = (now.tv_nsec + (ms%1000) * 1000) * 1000;
        if ( abstime.tv_nsec > 1000000000 ) {
          abstime.tv_sec += 1;
          abstime.tv_nsec -= 1000000000;
        }

	LWP_CondTimedWait ( cond->cond,  mutex->id, &abstime ) ;

	return retval;
}

/* Wait on the condition variable forever */
int SDL_CondWait(SDL_cond *cond, SDL_mutex *mutex)
{
	return LWP_CondWait ( cond->cond,  mutex->id ) ;

}
