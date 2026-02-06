#ifndef IMGUI_C_H
#define IMGUI_C_H

#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ImGuiCContext {
    SDL_Window *window;
    SDL_Renderer *renderer;
} ImGuiCContext;

int imgui_c_init(void *native_context);
void imgui_c_shutdown(void);
void imgui_c_process_event(const SDL_Event *event);
void imgui_c_new_frame(void);
void imgui_c_render(void);
void imgui_c_begin(const char *name);
void imgui_c_end(void);
void imgui_c_text(const char *text);
int imgui_c_button(const char *label);

#ifdef __cplusplus
}
#endif

#endif
