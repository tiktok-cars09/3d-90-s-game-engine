#pragma once

#include <SDL2/SDL.h>
#include "imgui.h"

IMGUI_IMPL_API bool ImGui_ImplSDL2_InitForSDLRenderer(SDL_Window *window, SDL_Renderer *renderer);
IMGUI_IMPL_API void ImGui_ImplSDL2_Shutdown(void);
IMGUI_IMPL_API void ImGui_ImplSDL2_NewFrame(void);
IMGUI_IMPL_API bool ImGui_ImplSDL2_ProcessEvent(const SDL_Event *event);
