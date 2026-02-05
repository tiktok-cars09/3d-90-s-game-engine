CC = gcc
CXX = g++

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS := $(shell sdl2-config --libs)

CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic $(SDL_CFLAGS)
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic $(SDL_CFLAGS)
LDFLAGS = $(SDL_LIBS) -lm

IMGUI ?= 0
IMGUI_DIR ?= third_party/imgui

GAME90_DEFS = -DGAME90_ENABLE_IMGUI=$(IMGUI)
CXXFLAGS += $(GAME90_DEFS)
CFLAGS += $(GAME90_DEFS)

GAME_SRC_C = src/main.c
GAME_SRC_CPP = src/imgui_c.cpp
GAME_OBJ = $(GAME_SRC_C:.c=.o) $(GAME_SRC_CPP:.cpp=.o)

EDITOR_SRC = src/map_editor.c
EDITOR_OBJ = $(EDITOR_SRC:.c=.o)

IMGUI_SOURCES =
IMGUI_OBJ =
ifeq ($(IMGUI),1)
IMGUI_SOURCES = \
	$(IMGUI_DIR)/imgui.cpp \
	$(IMGUI_DIR)/imgui_draw.cpp \
	$(IMGUI_DIR)/imgui_tables.cpp \
	$(IMGUI_DIR)/imgui_widgets.cpp \
	$(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
	$(IMGUI_DIR)/backends/imgui_impl_sdlrenderer2.cpp
IMGUI_OBJ = $(IMGUI_SOURCES:.cpp=.o)
GAME_OBJ += $(IMGUI_OBJ)
CXXFLAGS += -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
endif

all: game90 map_editor

game90: $(GAME_OBJ)
	$(CXX) -o $@ $(GAME_OBJ) $(LDFLAGS)

map_editor: $(EDITOR_OBJ)
	$(CC) -o $@ $(EDITOR_OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f game90 map_editor $(GAME_OBJ) $(EDITOR_OBJ)

.PHONY: all clean
