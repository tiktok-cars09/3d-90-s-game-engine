#include <SDL2/SDL.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int screenW = 800;
int screenH = 600;

int mapW = 24;
int mapH = 24;
int *worldMap = NULL; // allocated and filled at startup

static int defaultMap[24*24] = {
    /* row 0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    /* row 1 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 2 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 3 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 4 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 5 */ 1,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 6 */ 1,0,0,0,0,0,0,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 7 */ 1,0,0,0,0,0,0,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 8 */ 1,0,0,0,0,0,0,3,3,0,3,3,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 9 */ 1,0,0,0,0,0,0,3,3,0,3,3,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row10 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row11 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row12 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row13 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row14 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row15 */ 1,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,1,
    /* row16 */ 1,0,0,0,0,0,0,0,3,0,0,0,0,0,0,3,0,0,0,0,0,0,0,1,
    /* row17 */ 1,0,0,0,0,0,0,0,3,0,0,0,0,0,0,3,0,0,0,0,0,0,0,1,
    /* row18 */ 1,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,1,
    /* row19 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row20 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row21 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row22 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row23 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

#define MAP_AT(x,y) worldMap[(x) + mapW*(y)]

static void load_default_map(void) {
    // generate a BSP-style map for default
    // small BSP generator: carve rooms and connect them with corridors
    int W = mapW; int H = mapH;
    int *m = malloc(sizeof(int) * W * H);
    if (!m) { fprintf(stderr, "failed to allocate worldMap\n"); exit(1); }
    // fill with walls (1)
    for (int i = 0; i < W*H; ++i) m[i] = 1;

    typedef struct Node { int x,y,w,h; struct Node *a,*b; int roomx, roomy, roomw, roomh; } Node;
    // simple recursive split
    #define MIN_ROOM 5
    #define MAX_ITERS 256
    Node nodes[MAX_ITERS]; int ncount = 0;
    nodes[ncount++] = (Node){1,1,W-2,H-2,NULL,NULL,0,0,0,0};
    for (int i=0;i<ncount;i++) {
        Node *nd = &nodes[i];
        if (nd->w <= MIN_ROOM*2 || nd->h <= MIN_ROOM*2) continue;
        // split longer side
        if (nd->w > nd->h) {
            int split = nd->w/2 + (rand() % (nd->w/4+1)) - nd->w/8;
            if (ncount+1 < MAX_ITERS) {
                nodes[ncount] = (Node){nd->x, nd->y, split, nd->h, NULL, NULL,0,0,0,0};
                nd->x += split; nd->w -= split; nd->a = &nodes[ncount++]; nd->b = NULL;
            }
        } else {
            int split = nd->h/2 + (rand() % (nd->h/4+1)) - nd->h/8;
            if (ncount+1 < MAX_ITERS) {
                nodes[ncount] = (Node){nd->x, nd->y, nd->w, split, NULL, NULL,0,0,0,0};
                nd->y += split; nd->h -= split; nd->a = &nodes[ncount++]; nd->b = NULL;
            }
        }
    }
    // create rooms in leaves
    int roomCentersX[128]; int roomCentersY[128]; int roomCount = 0;
    for (int i=0;i<ncount;i++) {
        Node *nd = &nodes[i];
        if (nd->a == NULL && nd->b == NULL) {
            int rw = nd->w - 2; if (rw < 3) rw = nd->w;
            int rh = nd->h - 2; if (rh < 3) rh = nd->h;
            int rx = nd->x + 1 + (rand() % (rw>1?rw:1));
            int ry = nd->y + 1 + (rand() % (rh>1?rh:1));
            int rw2 = (rw>4)?(3 + rand()% (rw-2)): (rw);
            int rh2 = (rh>4)?(3 + rand()% (rh-2)): (rh);
            nd->roomx = rx; nd->roomy = ry; nd->roomw = rw2; nd->roomh = rh2;
            int sx = rx; int sy = ry; int ex = rx + rw2; int ey = ry + rh2;
            if (ex >= W-1) ex = W-2; if (ey >= H-1) ey = H-2;
            for (int yy=sy; yy<ey; yy++) for (int xx=sx; xx<ex; xx++) m[xx + W*yy] = 0;
            if (roomCount < 128) { roomCentersX[roomCount] = rx + rw2/2; roomCentersY[roomCount] = ry + rh2/2; roomCount++; }
        }
    }
    // connect rooms in sequence
    for (int i=1;i<roomCount;i++) {
        int x1 = roomCentersX[i-1], y1 = roomCentersY[i-1];
        int x2 = roomCentersX[i], y2 = roomCentersY[i];
        int cx = x1, cy = y1;
        while (cx != x2) { m[cx + W*cy] = 0; cx += (x2>cx)?1:-1; }
        while (cy != y2) { m[cx + W*cy] = 0; cy += (y2>cy)?1:-1; }
    }

    // paint room-border walls different values per room area to get solid colored walls
    // set any wall cell adjacent to floor to wall-type depending on nearest room index
    for (int y=1;y<H-1;y++) for (int x=1;x<W-1;x++) {
        if (m[x + W*y] == 1) {
            bool adjfloor = false;
            for (int yy=-1; yy<=1 && !adjfloor; yy++) for (int xx=-1; xx<=1 && !adjfloor; xx++) if (m[(x+xx) + W*(y+yy)] == 0) adjfloor = true;
            if (adjfloor) {
                // choose a color idx based on position
                int idx = ((x/4) + (y/4)) % 3 + 1;
                m[x + W*y] = idx;
            }
        }
    }

    // replace worldMap
    if (worldMap) free(worldMap);
    mapW = W; mapH = H; worldMap = m;
}

static bool load_map_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    char line[4096];
    // read first non-comment line for dimensions
    int w=0,h=0;
    while (fgets(line, sizeof(line), f)) {
        // skip leading whitespace
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '#') continue;
        if (sscanf(p, "%d %d", &w, &h) == 2) break;
    }
    if (w <= 0 || h <= 0) { fclose(f); return false; }
    int *m = malloc(sizeof(int)*w*h);
    if (!m) { fclose(f); return false; }
    // initialize to walls so missing/extra data won't leave garbage
    for (int i = 0; i < w*h; ++i) m[i] = 1;
    int x=0,y=0;
    while (y < h && fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p) {
            // skip whitespace
            while (*p && isspace((unsigned char)*p)) p++;
            if (!*p || *p == '#') break;
            int val;
            int consumed = 0;
            if (sscanf(p, "%d%n", &val, &consumed) == 1 && consumed > 0) {
                // guard against writing past allocated rows
                if (y >= h) break;
                m[x + w*y] = val;
                x++;
                p += consumed;
                if (x >= w) { x = 0; y++; if (y >= h) break; }
            } else break;
        }
    }
    fclose(f);
        if (worldMap) free(worldMap);
        mapW = w; mapH = h; worldMap = m;
        return true;
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("90s Raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenW, screenH, SDL_WINDOW_RESIZABLE);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    double posX = 22.0, posY = 12.0; // player start
    double dirX = -1.0, dirY = 0.0; // initial direction vector
    double fov_deg = 80.0;
    double planeLen = tan((fov_deg * M_PI / 180.0) / 2.0);
    double planeX = 0.0, planeY = planeLen; // camera plane computed from FOV
    const double mouseSensitivity = 0.0035;
    
    const int texWidth = 64;
    const int texHeight = 64;
    Uint32 textures[4][64*64];

    // generate simple procedural textures: 1=red brick,2=green,3=blue
    // make solid color wall textures for clearer solid blocks
    for (int t = 1; t <= 3; t++) {
        Uint8 r = 0, g = 0, b = 0;
        if (t == 1) { r = 180; g = 80; b = 80; }
        if (t == 2) { r = 80; g = 180; b = 80; }
        if (t == 3) { r = 80; g = 80; b = 180; }
        for (int y = 0; y < texHeight; y++) for (int x = 0; x < texWidth; x++) textures[t][y * texWidth + x] = (r<<16) | (g<<8) | b;
    }

    bool running = true;
    Uint32 oldTime = SDL_GetTicks();

    SDL_SetRelativeMouseMode(SDL_TRUE);
    bool isFullscreen = false;

    // if no map argument provided, offer to pick one from maps/ or use default
    if (argc > 1) {
        if (!load_map_file(argv[1])) load_default_map();
    } else {
        // list maps directory
        DIR *d = opendir("maps");
        if (d) {
            struct dirent *ent;
            char *files[256];
            int count = 0;
            while ((ent = readdir(d)) != NULL) {
                if (ent->d_type == DT_REG) {
                    const char *name = ent->d_name;
                    size_t L = strlen(name);
                    if (L > 4 && strcmp(name + L - 4, ".map") == 0) {
                        files[count] = malloc(512);
                        snprintf(files[count], 512, "maps/%s", name);
                        count++;
                        if (count >= 255) break;
                    }
                }
            }
            closedir(d);
            if (count > 0) {
                printf("Available maps:\n");
                for (int i=0;i<count;i++) printf("%d) %s\n", i+1, files[i]);
                printf("Enter map number to load, or 0 to use default: ");
                char buf[32];
                if (fgets(buf, sizeof(buf), stdin)) {
                    int sel = atoi(buf);
                    if (sel > 0 && sel <= count) {
                        if (!load_map_file(files[sel-1])) load_default_map();
                    } else {
                        load_default_map();
                    }
                } else load_default_map();
                for (int i=0;i<count;i++) free(files[i]);
            } else {
                load_default_map();
            }
        } else {
            load_default_map();
        }
    }

    // map loaded
    // ensure player start is inside map bounds
        if (posX < 1.0) posX = 1.5;
        if (posY < 1.0) posY = 1.5;
        if (posX >= mapW - 1) posX = mapW - 2 + 0.5;
        if (posY >= mapH - 1) posY = mapH - 2 + 0.5;
    // render-to-texture buffer (cap internal resolution for performance)
    const int MAX_RENDER_W = 1024;
    const int MAX_RENDER_H = 768;
    int renderW = screenW;
    int renderH = screenH;
    if (renderW > MAX_RENDER_W) renderW = MAX_RENDER_W;
    if (renderH > MAX_RENDER_H) renderH = MAX_RENDER_H;
    SDL_Texture *screenTex = NULL;
    Uint32 *pixels = NULL;
    SDL_Texture *tmpTex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, renderW, renderH);
    Uint32 *tmpPixels = malloc((size_t)renderW * renderH * sizeof(Uint32));
    if (!tmpTex || !tmpPixels) {
        fprintf(stderr, "Failed to allocate render texture/pixels for %dx%d\n", renderW, renderH);
        if (tmpTex) SDL_DestroyTexture(tmpTex);
        free(tmpPixels);
    } else {
        screenTex = tmpTex;
        pixels = tmpPixels;
    }

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_p) {
                    // pause menu
                    const SDL_MessageBoxButtonData buttons[] = {
                        { 0, 0, "Resume" },
                        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Exit" },
                    };
                    const SDL_MessageBoxData msg = {
                        SDL_MESSAGEBOX_INFORMATION, win, "Paused", "Game is paused.", SDL_arraysize(buttons), buttons, NULL
                    };
                    int buttonid = 0;
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    SDL_ShowCursor(SDL_ENABLE);
                    if (SDL_ShowMessageBox(&msg, &buttonid) < 0) {
                        buttonid = 0; // default to resume on error
                    }
                    if (buttonid == 1) {
                        running = false;
                    } else {
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        SDL_ShowCursor(SDL_DISABLE);
                    }
                }
                if (e.key.keysym.sym == SDLK_F11) {
                    // toggle fullscreen
                    if (!isFullscreen) {
                        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        isFullscreen = true;
                    } else {
                        SDL_SetWindowFullscreen(win, 0);
                        isFullscreen = false;
                    }
                    // update sizes and render target
                    SDL_GetWindowSize(win, &screenW, &screenH);
                    int newRenderW = screenW;
                    int newRenderH = screenH;
                    if (newRenderW > MAX_RENDER_W) newRenderW = MAX_RENDER_W;
                    if (newRenderH > MAX_RENDER_H) newRenderH = MAX_RENDER_H;
                    if (newRenderW != renderW || newRenderH != renderH) {
                        renderW = newRenderW;
                        renderH = newRenderH;
                            SDL_Texture *newTex2 = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, renderW, renderH);
                            Uint32 *newPixels2 = malloc((size_t)renderW * renderH * sizeof(Uint32));
                            if (!newTex2 || !newPixels2) {
                                fprintf(stderr, "Failed to allocate render texture/pixels for fullscreen %dx%d\n", renderW, renderH);
                                if (newTex2) SDL_DestroyTexture(newTex2);
                                free(newPixels2);
                            } else {
                                if (screenTex) SDL_DestroyTexture(screenTex);
                                screenTex = newTex2;
                                if (pixels) free(pixels);
                                pixels = newPixels2;
                            }
                    }
                }
            }
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);

        Uint32 currentTime = SDL_GetTicks();
        double frameTime = (currentTime - oldTime) / 1000.0; // seconds
        oldTime = currentTime;

        double moveSpeed = frameTime * 5.0; // the constant value is in squares/second
        double rotSpeed = frameTime * 3.0; // radians/second

        int mx, my;
        SDL_GetRelativeMouseState(&mx, &my);
        if (mx != 0) {
            double rot = -mx * mouseSensitivity;
            double oldDirX = dirX;
            dirX = dirX * cos(rot) - dirY * sin(rot);
            dirY = oldDirX * sin(rot) + dirY * cos(rot);
            double oldPlaneX = planeX;
            planeX = planeX * cos(rot) - planeY * sin(rot);
            planeY = oldPlaneX * sin(rot) + planeY * cos(rot);
        }

        if (state[SDL_SCANCODE_W]) {
            int nx = (int)(posX + dirX * moveSpeed);
            int ny = (int)posY;
            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH && MAP_AT(nx,ny) == 0) posX += dirX * moveSpeed;
            nx = (int)posX; ny = (int)(posY + dirY * moveSpeed);
            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH && MAP_AT(nx,ny) == 0) posY += dirY * moveSpeed;
        }
        if (state[SDL_SCANCODE_S]) {
            int nx = (int)(posX - dirX * moveSpeed);
            int ny = (int)posY;
            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH && MAP_AT(nx,ny) == 0) posX -= dirX * moveSpeed;
            nx = (int)posX; ny = (int)(posY - dirY * moveSpeed);
            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH && MAP_AT(nx,ny) == 0) posY -= dirY * moveSpeed;
        }
        if (state[SDL_SCANCODE_D]) {
            double strafeX = dirY;
            double strafeY = -dirX;
            int nx = (int)(posX + strafeX * moveSpeed);
            int ny = (int)posY;
            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH && MAP_AT(nx,ny) == 0) posX += strafeX * moveSpeed;
            nx = (int)posX; ny = (int)(posY + strafeY * moveSpeed);
            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH && MAP_AT(nx,ny) == 0) posY += strafeY * moveSpeed;
        }
        if (state[SDL_SCANCODE_A]) {
            double strafeX = dirY;
            double strafeY = -dirX;
            int nx = (int)(posX - strafeX * moveSpeed);
            int ny = (int)posY;
            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH && MAP_AT(nx,ny) == 0) posX -= strafeX * moveSpeed;
            nx = (int)posX; ny = (int)(posY - strafeY * moveSpeed);
            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH && MAP_AT(nx,ny) == 0) posY -= strafeY * moveSpeed;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            double oldDirX = dirX;
            dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
            dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
            double oldPlaneX = planeX;
            planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
            planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
        }
        if (state[SDL_SCANCODE_LEFT]) {
            double oldDirX = dirX;
            dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
            dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
            double oldPlaneX = planeX;
            planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
            planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
        }

        // handle window resize events that may have occurred
        int w, h;
        SDL_GetWindowSize(win, &w, &h);
        if (w != screenW || h != screenH) {
            screenW = w;
            screenH = h;
            int newRenderW = screenW;
            int newRenderH = screenH;
            if (newRenderW > MAX_RENDER_W) newRenderW = MAX_RENDER_W;
            if (newRenderH > MAX_RENDER_H) newRenderH = MAX_RENDER_H;
            if (newRenderW != renderW || newRenderH != renderH) {
                renderW = newRenderW;
                renderH = newRenderH;
                SDL_Texture *newTex2 = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, renderW, renderH);
                Uint32 *newPixels2 = malloc((size_t)renderW * renderH * sizeof(Uint32));
                if (!newTex2 || !newPixels2) {
                    fprintf(stderr, "Failed to allocate render texture/pixels for %dx%d\n", renderW, renderH);
                    if (newTex2) SDL_DestroyTexture(newTex2);
                    free(newPixels2);
                } else {
                    if (screenTex) SDL_DestroyTexture(screenTex);
                    screenTex = newTex2;
                    if (pixels) free(pixels);
                    pixels = newPixels2;
                }
            }
        }

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
                if (mapX >= 0 && mapX < mapW && mapY >= 0 && mapY < mapH && MAP_AT(mapX,mapY) > 0) hit = 1;
                if (++safety > (mapW*mapH*2)) break; // safety guard
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
            if (mapX >= 0 && mapX < mapW && mapY >= 0 && mapY < mapH) val = MAP_AT(mapX,mapY);
            int texNum = (val >=1 && val <=3) ? val : 1;

            double wallX; // where exactly the wall was hit
            if (side == 0) wallX = posY + perpWallDist * rayDirY;
            else            wallX = posX + perpWallDist * rayDirX;
            wallX -= floor(wallX);

            int texX = (int)(wallX * (double)texWidth);
            if (texX < 0) texX = 0;
            if (texX >= texWidth) texX = texWidth - 1;
            if (side == 0 && rayDirX > 0) texX = texWidth - texX - 1;
            if (side == 1 && rayDirY < 0) texX = texWidth - texX - 1;

            for (int y = drawStart; y <= drawEnd; y++) {
                int d = (y * 256) - (rh * 128) + (lineHeight * 128);
                int texY = (lineHeight != 0) ? ((d * texHeight) / lineHeight) / 256 : 0;
                if (texY < 0) texY = 0;
                if (texY >= texHeight) texY = texHeight - 1;
                Uint32 col = textures[texNum][texY * texWidth + texX];
                // ensure alpha set
                col |= 0xFF000000;
                if (side == 1) {
                    Uint8 r = ((col >> 16) & 0xFF) / 2;
                    Uint8 g = ((col >> 8) & 0xFF) / 2;
                    Uint8 b = (col & 0xFF) / 2;
                    col = (0xFF << 24) | (r<<16) | (g<<8) | b;
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
                Uint8 r = f/2, g = f, b = f/3;
                pixels[y * rw + x] = (0xFF<<24) | (r<<16) | (g<<8) | b;
            }
        }

        // upload pixel buffer and scale to window
        SDL_UpdateTexture(screenTex, NULL, pixels, renderW * sizeof(Uint32));
        SDL_SetRenderDrawColor(ren, 0,0,0,255);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, screenTex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    if (screenTex) SDL_DestroyTexture(screenTex);
    if (pixels) free(pixels);
    if (worldMap) free(worldMap);
    SDL_Quit();
    return 0;
}
