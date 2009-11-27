all: 
	cd SDL; $(MAKE) -f Makefile
	cd SDL_gfx; $(MAKE) -f Makefile
	cd smpeg; $(MAKE) -f Makefile
	cd SDL_mixer; $(MAKE) -f Makefile
	cd SDL_image; $(MAKE) -f Makefile
	cd SDL_net; $(MAKE) -f Makefile
	cd SDL_ttf; $(MAKE) -f Makefile

clean: 
	cd SDL; $(MAKE) -f Makefile clean
	cd SDL_gfx; $(MAKE) -f Makefile clean
	cd smpeg; $(MAKE) -f Makefile clean
	cd SDL_mixer; $(MAKE) -f Makefile clean
	cd SDL_image; $(MAKE) -f Makefile clean
	cd SDL_net; $(MAKE) -f Makefile clean
	cd SDL_ttf; $(MAKE) -f Makefile clean

install: 
	cd SDL; $(MAKE) -f Makefile install
	cd SDL_gfx; $(MAKE) -f Makefile install
	cd smpeg; $(MAKE) -f Makefile install
	cd SDL_mixer; $(MAKE) -f Makefile install
	cd SDL_image; $(MAKE) -f Makefile install
	cd SDL_net; $(MAKE) -f Makefile install
	cd SDL_ttf; $(MAKE) -f Makefile install
