CC = gcc
CXX = g++

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS := $(shell sdl2-config --libs)

CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic $(SDL_CFLAGS) -Isrc/ui
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic $(SDL_CFLAGS) -Isrc/ui
LDFLAGS = $(SDL_LIBS) -lm

IMGUI ?= 1
IMGUI_DIR ?=
IMGUI_DIR_AUTO := $(firstword $(wildcard third_party/imgui build/*/_deps/imgui-src))
IMGUI_PATH := $(IMGUI_DIR)
ifeq ($(IMGUI_PATH),)
IMGUI_PATH := $(IMGUI_DIR_AUTO)
endif

IMGUI_ENABLED := $(IMGUI)
ifeq ($(IMGUI),1)
ifeq ($(IMGUI_PATH),)
IMGUI_ENABLED := 0
$(warning IMGUI=1 but ImGui source not found. Set IMGUI_DIR=... or add third_party/imgui.)
endif
endif

GAME90_DEFS = -DGAME90_ENABLE_IMGUI=$(IMGUI_ENABLED)
CXXFLAGS += $(GAME90_DEFS)
CFLAGS += $(GAME90_DEFS)

GAME_SRC_C = src/game/main.c src/game/map.c src/game/render.c
GAME_SRC_CPP = src/ui/imgui_c.cpp
GAME_OBJ = $(GAME_SRC_C:.c=.o) $(GAME_SRC_CPP:.cpp=.o)

EDITOR_SRC = src/editor/map_editor.c
EDITOR_OBJ = $(EDITOR_SRC:.c=.o)

IMGUI_SOURCES =
IMGUI_OBJ =
ifeq ($(IMGUI_ENABLED),1)
IMGUI_SOURCES = \
	$(IMGUI_PATH)/imgui.cpp \
	$(IMGUI_PATH)/imgui_draw.cpp \
	$(IMGUI_PATH)/imgui_tables.cpp \
	$(IMGUI_PATH)/imgui_widgets.cpp \
	$(IMGUI_PATH)/backends/imgui_impl_sdl2.cpp \
	$(IMGUI_PATH)/backends/imgui_impl_sdlrenderer2.cpp
IMGUI_OBJ = $(IMGUI_SOURCES:.cpp=.o)
GAME_OBJ += $(IMGUI_OBJ)
CXXFLAGS += -I$(IMGUI_PATH) -I$(IMGUI_PATH)/backends
endif

all: game90 map_editor

game90: $(GAME_OBJ)
	$(CXX) -o $@ $(GAME_OBJ) $(LDFLAGS)

map_editor: $(EDITOR_OBJ)
	$(CC) -o $@ $(EDITOR_OBJ) $(LDFLAGS)

$(GAME_OBJ) $(IMGUI_OBJ) $(EDITOR_OBJ): Makefile

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f game90 map_editor $(GAME_OBJ) $(EDITOR_OBJ)

.PHONY: all clean
