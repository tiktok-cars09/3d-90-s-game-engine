#include "imgui_impl_sdl2.h"

#include <SDL2/SDL.h>
#include <float.h>

static SDL_Window *g_Window = NULL;
static SDL_Renderer *g_Renderer = NULL;
static Uint64 g_Time = 0;
static bool g_MousePressed[3] = {false, false, false};
static float g_MouseWheel = 0.0f;
static float g_MouseWheelH = 0.0f;

bool ImGui_ImplSDL2_InitForSDLRenderer(SDL_Window *window, SDL_Renderer *renderer) {
    g_Window = window;
    g_Renderer = renderer;
    g_Time = 0;

    ImGuiIO &io = ImGui::GetIO();
    io.BackendPlatformName = "imgui_impl_sdl2_custom";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    return true;
}

void ImGui_ImplSDL2_Shutdown(void) {
    g_Window = NULL;
    g_Renderer = NULL;
    g_Time = 0;
}

bool ImGui_ImplSDL2_ProcessEvent(const SDL_Event *event) {
    if (!event) return false;
    ImGuiIO &io = ImGui::GetIO();
    switch (event->type) {
    case SDL_MOUSEWHEEL:
        if (event->wheel.x > 0) g_MouseWheelH += 1.0f;
        if (event->wheel.x < 0) g_MouseWheelH -= 1.0f;
        if (event->wheel.y > 0) g_MouseWheel += 1.0f;
        if (event->wheel.y < 0) g_MouseWheel -= 1.0f;
        return true;
    case SDL_MOUSEBUTTONDOWN:
        if (event->button.button == SDL_BUTTON_LEFT) g_MousePressed[0] = true;
        if (event->button.button == SDL_BUTTON_RIGHT) g_MousePressed[1] = true;
        if (event->button.button == SDL_BUTTON_MIDDLE) g_MousePressed[2] = true;
        return true;
    case SDL_TEXTINPUT:
        io.AddInputCharactersUTF8(event->text.text);
        return true;
    default:
        return false;
    }
}

void ImGui_ImplSDL2_NewFrame(void) {
    ImGuiIO &io = ImGui::GetIO();

    int w = 0, h = 0;
    int display_w = 0, display_h = 0;
    if (g_Window) {
        SDL_GetWindowSize(g_Window, &w, &h);
    }
    if (g_Renderer) {
        SDL_GetRendererOutputSize(g_Renderer, &display_w, &display_h);
    } else {
        display_w = w;
        display_h = h;
    }

    if (w > 0 && h > 0) {
        io.DisplaySize = ImVec2((float)w, (float)h);
        io.DisplayFramebufferScale = ImVec2(
            (float)display_w / (float)w,
            (float)display_h / (float)h);
    }

    Uint64 current_time = SDL_GetPerformanceCounter();
    double freq = (double)SDL_GetPerformanceFrequency();
    if (g_Time > 0) {
        io.DeltaTime = (float)((current_time - g_Time) / freq);
    } else {
        io.DeltaTime = 1.0f / 60.0f;
    }
    g_Time = current_time;

    int mx = 0, my = 0;
    Uint32 mouse_mask = SDL_GetMouseState(&mx, &my);
    io.MousePos = ImVec2((float)mx, (float)my);

    io.MouseDown[0] = g_MousePressed[0] || (mouse_mask & SDL_BUTTON(SDL_BUTTON_LEFT));
    io.MouseDown[1] = g_MousePressed[1] || (mouse_mask & SDL_BUTTON(SDL_BUTTON_RIGHT));
    io.MouseDown[2] = g_MousePressed[2] || (mouse_mask & SDL_BUTTON(SDL_BUTTON_MIDDLE));
    g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;

    io.MouseWheel = g_MouseWheel;
    io.MouseWheelH = g_MouseWheelH;
    g_MouseWheel = 0.0f;
    g_MouseWheelH = 0.0f;
}
