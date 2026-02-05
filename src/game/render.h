#ifndef GAME_RENDER_H
#define GAME_RENDER_H

#include <SDL2/SDL.h>

#define GAME_TEX_W 64
#define GAME_TEX_H 64

void init_textures(Uint32 textures[4][GAME_TEX_W * GAME_TEX_H]);
void render_world(
    Uint32 *pixels,
    int renderW,
    int renderH,
    double posX,
    double posY,
    double dirX,
    double dirY,
    double planeX,
    double planeY,
    const Uint32 textures[4][GAME_TEX_W * GAME_TEX_H]);

#endif
