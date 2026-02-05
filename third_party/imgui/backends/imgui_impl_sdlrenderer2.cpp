#include "imgui_impl_sdlrenderer2.h"

#include <SDL2/SDL.h>
#include <vector>

static SDL_Renderer *g_Renderer = NULL;
static SDL_Texture *g_FontTexture = NULL;

static bool ImGui_ImplSDLRenderer2_CreateFontsTexture(void) {
    ImGuiIO &io = ImGui::GetIO();
    unsigned char *pixels = NULL;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    if (!g_Renderer || !pixels || width == 0 || height == 0) {
        return false;
    }

    if (g_FontTexture) {
        SDL_DestroyTexture(g_FontTexture);
        g_FontTexture = NULL;
    }

    g_FontTexture = SDL_CreateTexture(g_Renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!g_FontTexture) {
        return false;
    }
    SDL_UpdateTexture(g_FontTexture, NULL, pixels, width * 4);
    SDL_SetTextureBlendMode(g_FontTexture, SDL_BLENDMODE_BLEND);

    io.Fonts->SetTexID((ImTextureID)(intptr_t)g_FontTexture);
    return true;
}

bool ImGui_ImplSDLRenderer2_Init(SDL_Renderer *renderer) {
    g_Renderer = renderer;
    return ImGui_ImplSDLRenderer2_CreateFontsTexture();
}

void ImGui_ImplSDLRenderer2_Shutdown(void) {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->SetTexID(0);
    if (g_FontTexture) {
        SDL_DestroyTexture(g_FontTexture);
        g_FontTexture = NULL;
    }
    g_Renderer = NULL;
}

void ImGui_ImplSDLRenderer2_NewFrame(void) {
    if (!g_FontTexture) {
        ImGui_ImplSDLRenderer2_CreateFontsTexture();
    }
}

void ImGui_ImplSDLRenderer2_RenderDrawData(ImDrawData *draw_data) {
    if (!g_Renderer || !draw_data) {
        return;
    }

    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }

    SDL_BlendMode old_blend = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(g_Renderer, &old_blend);
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);

    ImVec2 clip_off = draw_data->DisplayPos;
    ImVec2 clip_scale = draw_data->FramebufferScale;

#if SDL_VERSION_ATLEAST(2, 0, 18)
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        const ImDrawVert *vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx *idx_buffer = cmd_list->IdxBuffer.Data;

        std::vector<SDL_Vertex> vertices;
        vertices.resize(cmd_list->VtxBuffer.Size);
        for (int i = 0; i < cmd_list->VtxBuffer.Size; i++) {
            const ImDrawVert &v = vtx_buffer[i];
            SDL_Vertex &out = vertices[i];
            out.position.x = v.pos.x;
            out.position.y = v.pos.y;
            out.tex_coord.x = v.uv.x;
            out.tex_coord.y = v.uv.y;
            out.color.r = (Uint8)((v.col >> IM_COL32_R_SHIFT) & 0xFF);
            out.color.g = (Uint8)((v.col >> IM_COL32_G_SHIFT) & 0xFF);
            out.color.b = (Uint8)((v.col >> IM_COL32_B_SHIFT) & 0xFF);
            out.color.a = (Uint8)((v.col >> IM_COL32_A_SHIFT) & 0xFF);
        }

        std::vector<int> indices;
        indices.resize(cmd_list->IdxBuffer.Size);
        for (int i = 0; i < cmd_list->IdxBuffer.Size; i++) {
            indices[i] = (int)idx_buffer[i];
        }

        int idx_offset = 0;
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                    SDL_Rect r;
                    r.x = (int)clip_rect.x;
                    r.y = (int)clip_rect.y;
                    r.w = (int)(clip_rect.z - clip_rect.x);
                    r.h = (int)(clip_rect.w - clip_rect.y);
                    SDL_RenderSetClipRect(g_Renderer, &r);

                    SDL_Texture *tex = (SDL_Texture *)pcmd->GetTexID();
                    SDL_RenderGeometry(g_Renderer, tex, vertices.data(), (int)vertices.size(),
                                       indices.data() + idx_offset, (int)pcmd->ElemCount);
                }
            }
            idx_offset += pcmd->ElemCount;
        }
    }
    SDL_RenderSetClipRect(g_Renderer, NULL);
#else
    (void)draw_data;
#endif

    SDL_SetRenderDrawBlendMode(g_Renderer, old_blend);
}
