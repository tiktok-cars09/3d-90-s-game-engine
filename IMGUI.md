# ImGui C Wrapper Usage

This project exposes a minimal C API in `src/ui/imgui_c.h` for use from `src/game/main.c`.

## Functions

- `int imgui_c_init(void *native_context);`
  - Initialize ImGui. Pass a pointer to `ImGuiCContext`.
  - Returns `1` on success, `0` on failure.

- `void imgui_c_shutdown(void);`
  - Shutdown ImGui and free internal state. Safe to call only if init succeeded.

- `void imgui_c_process_event(const SDL_Event *event);`
  - Forward SDL events to ImGui (mouse, keyboard, text). Call this inside your event loop.

- `void imgui_c_new_frame(void);`
  - Start a new ImGui frame. Call once per frame before building UI.

- `void imgui_c_render(void);`
  - Render the ImGui draw data. Call after all UI is built.

- `void imgui_c_begin(const char *name);`
  - Begin a window. Must be paired with `imgui_c_end()`.

- `void imgui_c_end(void);`
  - End the current window.

- `void imgui_c_text(const char *text);`
  - Add a text line in the current window.

## Minimal Pattern (Main Loop)

```c
ImGuiCContext imgui_ctx = { .window = win, .renderer = ren };
int imgui_enabled = imgui_c_init(&imgui_ctx);

while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (imgui_enabled) {
            imgui_c_process_event(&e);
        }
        // other event handling
    }

    // game render

    if (imgui_enabled) {
        imgui_c_new_frame();
        imgui_c_begin("Overlay");
        imgui_c_text("Hello ImGui");
        imgui_c_end();
        imgui_c_render();
    }
}

if (imgui_enabled) {
    imgui_c_shutdown();
}
```
