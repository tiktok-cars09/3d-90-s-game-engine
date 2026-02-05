#include "render.h"

#include "map.h"

#include <math.h>

void init_textures(Uint32 textures[4][GAME_TEX_W * GAME_TEX_H]) {
    // generate simple procedural textures: 1=red brick,2=green,3=blue
    // make solid color wall textures for clearer solid blocks
    for (int t = 1; t <= 3; t++) {
        Uint8 r = 0, g = 0, b = 0;
        if (t == 1) { r = 180; g = 80; b = 80; }
        if (t == 2) { r = 80; g = 180; b = 80; }
        if (t == 3) { r = 80; g = 80; b = 180; }
        for (int y = 0; y < GAME_TEX_H; y++) for (int x = 0; x < GAME_TEX_W; x++) {
            textures[t][y * GAME_TEX_W + x] = (r << 16) | (g << 8) | b;
        }
    }
}

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
    Uint32 textures[4][GAME_TEX_W * GAME_TEX_H]) {
    // render into pixel buffer at capped render resolution
    int rw = renderW;
    int rh = renderH;
    for (int i = 0; i < rw * rh; i++) pixels[i] = 0xFF404040; // clear to ceiling color

    for (int x = 0; x < rw; x++) {
        double cameraX = 2.0 * x / (double)rw - 1.0;
        double rayDirX = dirX + planeX * cameraX;
        double rayDirY = dirY + planeY * cameraX;

        int mapX = (int)posX;
        int mapY = (int)posY;

        double sideDistX;
        double sideDistY;

        double deltaDistX = (rayDirX == 0) ? 1e30 : fabs(1.0 / rayDirX);
        double deltaDistY = (rayDirY == 0) ? 1e30 : fabs(1.0 / rayDirY);
        double perpWallDist;

        int stepX;
        int stepY;

        int hit = 0;
        int side;

        if (rayDirX < 0) { stepX = -1; sideDistX = (posX - mapX) * deltaDistX; }
        else { stepX = 1; sideDistX = (mapX + 1.0 - posX) * deltaDistX; }
        if (rayDirY < 0) { stepY = -1; sideDistY = (posY - mapY) * deltaDistY; }
        else { stepY = 1; sideDistY = (mapY + 1.0 - posY) * deltaDistY; }

        // DDA
        int safety = 0;
        while (hit == 0) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            if (mapX >= 0 && mapX < mapW && mapY >= 0 && mapY < mapH && MAP_AT(mapX, mapY) > 0) hit = 1;
            if (++safety > (mapW * mapH * 2)) break; // safety guard
        }

        if (side == 0) perpWallDist = (rayDirX != 0.0) ? (mapX - posX + (1 - stepX) / 2.0) / rayDirX : 1e-6;
        else           perpWallDist = (rayDirY != 0.0) ? (mapY - posY + (1 - stepY) / 2.0) / rayDirY : 1e-6;
        if (!isfinite(perpWallDist) || perpWallDist <= 0.0) perpWallDist = 1e-6;

        int lineHeight = (int)(rh / perpWallDist);
        if (lineHeight <= 0) lineHeight = rh;
        if (lineHeight > (1<<20)) lineHeight = (1<<20);

        int drawStart = -lineHeight / 2 + rh / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + rh / 2;
        if (drawEnd >= rh) drawEnd = rh - 1;

        // textured wall
        int val = 0;
        if (mapX >= 0 && mapX < mapW && mapY >= 0 && mapY < mapH) val = MAP_AT(mapX, mapY);
        int texNum = (val >= 1 && val <= 3) ? val : 1;

        double wallX; // where exactly the wall was hit
        if (side == 0) wallX = posY + perpWallDist * rayDirY;
        else            wallX = posX + perpWallDist * rayDirX;
        wallX -= floor(wallX);

        int texX = (int)(wallX * (double)GAME_TEX_W);
        if (texX < 0) texX = 0;
        if (texX >= GAME_TEX_W) texX = GAME_TEX_W - 1;
        if (side == 0 && rayDirX > 0) texX = GAME_TEX_W - texX - 1;
        if (side == 1 && rayDirY < 0) texX = GAME_TEX_W - texX - 1;

        for (int y = drawStart; y <= drawEnd; y++) {
            int d = (y * 256) - (rh * 128) + (lineHeight * 128);
            int texY = (lineHeight != 0) ? ((d * GAME_TEX_H) / lineHeight) / 256 : 0;
            if (texY < 0) texY = 0;
            if (texY >= GAME_TEX_H) texY = GAME_TEX_H - 1;
            Uint32 col = textures[texNum][texY * GAME_TEX_W + texX];
            // ensure alpha set
            col |= 0xFF000000;
            if (side == 1) {
                Uint8 r = ((col >> 16) & 0xFF) / 2;
                Uint8 g = ((col >> 8) & 0xFF) / 2;
                Uint8 b = (col & 0xFF) / 2;
                col = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
            pixels[y * rw + x] = col;
        }

        // floor (simple shading per column)
        for (int y = drawEnd + 1; y < rh; y++) {
            double currentDist = rh / (2.0 * y - rh);
            double weight = currentDist / perpWallDist;
            double floorX = weight * (mapX + 0.5) + (1.0 - weight) * posX;
            double floorY = weight * (mapY + 0.5) + (1.0 - weight) * posY;
            int checker = ((int)floor(floorX) + (int)floor(floorY)) % 2;
            Uint8 f = checker ? 80 : 110;
            Uint8 r = f / 2, g = f, b = f / 3;
            pixels[y * rw + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }
}
