CC = gcc
CFLAGS = -std=c11 -O2 -Wall `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lm
SRC = src/main.c
OBJ = $(SRC:.c=.o)
BIN = game90

EDITOR_SRC = src/map_editor.c
EDITOR_BIN = map_editor

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LDFLAGS)

$(EDITOR_BIN): $(EDITOR_SRC)
	$(CC) $(CFLAGS) -o $(EDITOR_BIN) $(EDITOR_SRC) $(LDFLAGS)

clean:
	rm -f $(BIN) $(OBJ)

.PHONY: all clean
