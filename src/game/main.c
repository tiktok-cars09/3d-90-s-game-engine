#include <SDL2/SDL.h>
#include "imgui_c.h"
#include "map.h"
#include "render.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int screenW = 800;
int screenH = 600;

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

    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, renderer_flags);
    if (!ren) {
        renderer_flags = SDL_RENDERER_ACCELERATED;
        ren = SDL_CreateRenderer(win, -1, renderer_flags);
    }
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    ImGuiCContext imgui_ctx;
    imgui_ctx.window = win;
    imgui_ctx.renderer = ren;
    int imgui_enabled = imgui_c_init(&imgui_ctx);
    if (!imgui_enabled) {
        fprintf(stderr, "ImGui disabled or failed to initialize (build with IMGUI=1 and set IMGUI_DIR if needed).\n");
    }

    double posX = 22.0, posY = 12.0; // player start
    double dirX = -1.0, dirY = 0.0; // initial direction vector
    double fov_deg = 80.0;
    double planeLen = tan((fov_deg * M_PI / 180.0) / 2.0);
    double planeX = 0.0, planeY = planeLen; // camera plane computed from FOV
    const double mouseSensitivity = 0.0035;
    
    Uint32 textures[4][GAME_TEX_W * GAME_TEX_H];
    init_textures(textures);

    bool running = true;
    Uint32 oldTime = SDL_GetTicks();

    bool ui_visible = imgui_enabled;
    if (ui_visible) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
    } else {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_ShowCursor(SDL_DISABLE);
        SDL_RaiseWindow(win);
        SDL_SetWindowGrab(win, SDL_TRUE);
    }
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
                const char *name = ent->d_name;
                size_t L = strlen(name);
                if (L > 4 && strcmp(name + L - 4, ".map") == 0) {
                    files[count] = malloc(512);
                    snprintf(files[count], 512, "maps/%s", name);
                    count++;
                    if (count >= 255) break;
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
            if (imgui_enabled) {
                imgui_c_process_event(&e);
            }
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (imgui_enabled && e.key.keysym.sym == SDLK_F1) {
                    ui_visible = !ui_visible;
                    if (ui_visible) {
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        SDL_ShowCursor(SDL_ENABLE);
                        SDL_SetWindowGrab(win, SDL_FALSE);
                    } else {
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        SDL_ShowCursor(SDL_DISABLE);
                        SDL_RaiseWindow(win);
                        SDL_SetWindowGrab(win, SDL_TRUE);
                    }
                }
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
                        if (ui_visible) {
                            SDL_SetRelativeMouseMode(SDL_FALSE);
                            SDL_ShowCursor(SDL_ENABLE);
                            SDL_SetWindowGrab(win, SDL_FALSE);
                        } else {
                            SDL_SetRelativeMouseMode(SDL_TRUE);
                            SDL_ShowCursor(SDL_DISABLE);
                            SDL_RaiseWindow(win);
                            SDL_SetWindowGrab(win, SDL_TRUE);
                        }
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
        double fps = (frameTime > 0.0) ? (1.0 / frameTime) : 0.0;

        double moveSpeed = frameTime * 5.0; // the constant value is in squares/second
        double rotSpeed = frameTime * 3.0; // radians/second

        int mx = 0;
        int my = 0;
        if (!ui_visible) {
            SDL_GetRelativeMouseState(&mx, &my);
        }
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

        render_world(pixels, renderW, renderH, posX, posY, dirX, dirY, planeX, planeY, textures);

        // upload pixel buffer and scale to window
        SDL_UpdateTexture(screenTex, NULL, pixels, renderW * sizeof(Uint32));
        SDL_SetRenderDrawColor(ren, 0,0,0,255);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, screenTex, NULL, NULL);
        if (imgui_enabled && ui_visible) {
            imgui_c_new_frame();
            imgui_c_begin("Overlay");
            char fps_text[64];
            snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", fps);
            imgui_c_text(fps_text);
            imgui_c_end();
            imgui_c_render();
        }
        SDL_RenderPresent(ren);
    }

    if (imgui_enabled) {
        imgui_c_shutdown();
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    if (screenTex) SDL_DestroyTexture(screenTex);
    if (pixels) free(pixels);
    if (worldMap) free(worldMap);
    SDL_Quit();
    return 0;
}
