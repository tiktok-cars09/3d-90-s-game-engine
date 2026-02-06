#include "imgui_c.h"

#if defined(GAME90_ENABLE_IMGUI) && GAME90_ENABLE_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

struct ImGuiCState {
    SDL_Window *window;
    SDL_Renderer *renderer;
    bool initialized;
};

static ImGuiCState g_state = {nullptr, nullptr, false};

extern "C" {

int imgui_c_init(void *native_context) {
    if (g_state.initialized) {
        return 1;
    }

    ImGuiCContext *ctx = static_cast<ImGuiCContext *>(native_context);
    if (!ctx || !ctx->window || !ctx->renderer) {
        return 0;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForSDLRenderer(ctx->window, ctx->renderer)) {
        ImGui::DestroyContext();
        return 0;
    }
    if (!ImGui_ImplSDLRenderer2_Init(ctx->renderer)) {
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        return 0;
    }

    g_state.window = ctx->window;
    g_state.renderer = ctx->renderer;
    g_state.initialized = true;
    return 1;
}

void imgui_c_shutdown(void) {
    if (!g_state.initialized) {
        return;
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    g_state = {nullptr, nullptr, false};
}

void imgui_c_process_event(const SDL_Event *event) {
    if (!g_state.initialized || !event) {
        return;
    }
    ImGui_ImplSDL2_ProcessEvent(event);
}

void imgui_c_new_frame(void) {
    if (!g_state.initialized) {
        return;
    }

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void imgui_c_render(void) {
    if (!g_state.initialized) {
        return;
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}

void imgui_c_begin(const char *name) {
    if (!g_state.initialized || !name) {
        return;
    }

    ImGui::Begin(name);
}

void imgui_c_end(void) {
    if (!g_state.initialized) {
        return;
    }

    ImGui::End();
}

void imgui_c_text(const char *text) {
    if (!g_state.initialized) {
        return;
    }

    ImGui::TextUnformatted(text ? text : "");
}

int imgui_c_button(const char *label) {
    if (!g_state.initialized) return 0;
    return ImGui::Button(label ? label : "") ? 1 : 0;
}

} // extern "C"
#else
extern "C" {

int imgui_c_init(void *native_context) {
    (void)native_context;
    return 0;
}

void imgui_c_shutdown(void) {
}

void imgui_c_process_event(const SDL_Event *event) {
    (void)event;
}

void imgui_c_new_frame(void) {
}

void imgui_c_render(void) {
}

void imgui_c_begin(const char *name) {
    (void)name;
}

void imgui_c_end(void) {
}

void imgui_c_text(const char *text) {
    (void)text;
}

} // extern "C"
#endif
