
/* Print out all the keysyms we have, just to verify them */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

int main(int argc, char *argv[])
{
	SDLKey key;

	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
							SDL_GetError());
		exit(1);
	}
	for ( key=SDLK_FIRST; key<SDLK_LAST; ++key ) {
		printf("Key #%d, \"%s\"\n", key, SDL_GetKeyName(key));
	}

	int quit = 0;

	while (!quit) {
		SDL_Event	event;
		while (SDL_PollEvent(&event)) {
			
			switch(event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						quit = SDL_TRUE;
					}
					break;
				case SDL_QUIT:
					quit = SDL_TRUE;
					break;
			}
		}	
		SDL_Delay(1);
	}

	SDL_Quit();
	return(0);
}
