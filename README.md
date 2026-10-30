# ⚠️ Deprecated: use [dborth/SDL](https://github.com/dborth/SDL) instead

**This repository is no longer maintained.** It is an SDL **1.2** port for the Wii (plus SDL_gfx, SDL_image, SDL_mixer, SDL_net, SDL_ttf and smpeg), written for the old Super Mario War port. That port now uses SDL3, and this repository has been archived.

**Where to go now**

- **SDL3 for Wii, GameCube and Wii U:** <https://github.com/dborth/SDL>. Prebuilt packages and continuous builds are on its [releases page](https://github.com/dborth/SDL/releases). Currently SDL3 only: the companion libraries (SDL3_image, SDL3_mixer and so on) are not part of it yet.
- **SDL2 for Wii and GameCube:** `libogc2-sdl2` from [extremscorner/pacman-packages](https://github.com/extremscorner/pacman-packages).
- **Using SDL 1.2:** this repository stays available read-only and still works as it did. SDL3's API is not compatible with SDL 1.2, so existing SDL 1.2 programs need porting; see SDL's migration guides.
- **Example of a game on the new ports:** [smw-wii](https://github.com/dborth/smw-wii).

---

## Port of SDL for the Nintendo Wii
https://github.com/dborth/sdl-wii

This port uses the devKitPro toolchain, available from:
http://www.devkitpro.org


### Credits

     Original GameCube port                            www.devkitpro.org
     Ported to Wii                                     Mindcry (http://mindcry.cpl.de/wii/)
     Many fixes (threading, audio, etc)                Yohanes Nugroho (http://tinyhack.com)
     Renamed to Wii, update to SDL SVN                 David Hudson (http://cs-sdl.sourceforge.net)
     Audio + Video (using GX) systems rewrite          Tantric