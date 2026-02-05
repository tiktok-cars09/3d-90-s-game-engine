#ifndef GAME_MAP_H
#define GAME_MAP_H

#include <stdbool.h>

extern int mapW;
extern int mapH;
extern int *worldMap;

#define MAP_AT(x, y) worldMap[(x) + mapW * (y)]

void load_default_map(void);
bool load_map_file(const char *path);

#endif
