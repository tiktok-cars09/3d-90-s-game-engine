#include "map.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int mapW = 24;
int mapH = 24;
int *worldMap = NULL; // allocated and filled at startup

static int defaultMap[24 * 24] = {
    /* row 0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    /* row 1 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 2 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 3 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 4 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 5 */ 1,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 6 */ 1,0,0,0,0,0,0,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 7 */ 1,0,0,0,0,0,0,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 8 */ 1,0,0,0,0,0,0,3,3,0,3,3,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row 9 */ 1,0,0,0,0,0,0,3,3,0,3,3,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row10 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row11 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row12 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row13 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row14 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row15 */ 1,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,1,
    /* row16 */ 1,0,0,0,0,0,0,0,3,0,0,0,0,0,0,3,0,0,0,0,0,0,0,1,
    /* row17 */ 1,0,0,0,0,0,0,0,3,0,0,0,0,0,0,3,0,0,0,0,0,0,0,1,
    /* row18 */ 1,0,0,0,0,0,0,0,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,1,
    /* row19 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row20 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row21 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row22 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* row23 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

void load_default_map(void) {
    (void)defaultMap;
    // generate a BSP-style map for default
    // small BSP generator: carve rooms and connect them with corridors
    int W = mapW;
    int H = mapH;
    int *m = malloc(sizeof(int) * W * H);
    if (!m) { fprintf(stderr, "failed to allocate worldMap\n"); exit(1); }
    // fill with walls (1)
    for (int i = 0; i < W * H; ++i) m[i] = 1;

    typedef struct Node { int x, y, w, h; struct Node *a, *b; int roomx, roomy, roomw, roomh; } Node;
    // simple recursive split
    #define MIN_ROOM 5
    #define MAX_ITERS 256
    Node nodes[MAX_ITERS];
    int ncount = 0;
    nodes[ncount++] = (Node){1,1,W-2,H-2,NULL,NULL,0,0,0,0};
    for (int i = 0; i < ncount; i++) {
        Node *nd = &nodes[i];
        if (nd->w <= MIN_ROOM * 2 || nd->h <= MIN_ROOM * 2) continue;
        // split longer side
        if (nd->w > nd->h) {
            int split = nd->w / 2 + (rand() % (nd->w / 4 + 1)) - nd->w / 8;
            if (ncount + 1 < MAX_ITERS) {
                nodes[ncount] = (Node){nd->x, nd->y, split, nd->h, NULL, NULL,0,0,0,0};
                nd->x += split; nd->w -= split; nd->a = &nodes[ncount++]; nd->b = NULL;
            }
        } else {
            int split = nd->h / 2 + (rand() % (nd->h / 4 + 1)) - nd->h / 8;
            if (ncount + 1 < MAX_ITERS) {
                nodes[ncount] = (Node){nd->x, nd->y, nd->w, split, NULL, NULL,0,0,0,0};
                nd->y += split; nd->h -= split; nd->a = &nodes[ncount++]; nd->b = NULL;
            }
        }
    }
    // create rooms in leaves
    int roomCentersX[128];
    int roomCentersY[128];
    int roomCount = 0;
    for (int i = 0; i < ncount; i++) {
        Node *nd = &nodes[i];
        if (nd->a == NULL && nd->b == NULL) {
            int rw = nd->w - 2; if (rw < 3) rw = nd->w;
            int rh = nd->h - 2; if (rh < 3) rh = nd->h;
            int rx = nd->x + 1 + (rand() % (rw > 1 ? rw : 1));
            int ry = nd->y + 1 + (rand() % (rh > 1 ? rh : 1));
            int rw2 = (rw > 4) ? (3 + rand() % (rw - 2)) : (rw);
            int rh2 = (rh > 4) ? (3 + rand() % (rh - 2)) : (rh);
            nd->roomx = rx; nd->roomy = ry; nd->roomw = rw2; nd->roomh = rh2;
            int sx = rx; int sy = ry; int ex = rx + rw2; int ey = ry + rh2;
            if (ex >= W - 1) ex = W - 2;
            if (ey >= H - 1) ey = H - 2;
            for (int yy = sy; yy < ey; yy++) for (int xx = sx; xx < ex; xx++) m[xx + W * yy] = 0;
            if (roomCount < 128) { roomCentersX[roomCount] = rx + rw2 / 2; roomCentersY[roomCount] = ry + rh2 / 2; roomCount++; }
        }
    }
    // connect rooms in sequence
    for (int i = 1; i < roomCount; i++) {
        int x1 = roomCentersX[i-1], y1 = roomCentersY[i-1];
        int x2 = roomCentersX[i], y2 = roomCentersY[i];
        int cx = x1, cy = y1;
        while (cx != x2) { m[cx + W * cy] = 0; cx += (x2 > cx) ? 1 : -1; }
        while (cy != y2) { m[cx + W * cy] = 0; cy += (y2 > cy) ? 1 : -1; }
    }

    // paint room-border walls different values per room area to get solid colored walls
    // set any wall cell adjacent to floor to wall-type depending on nearest room index
    for (int y = 1; y < H - 1; y++) for (int x = 1; x < W - 1; x++) {
        if (m[x + W * y] == 1) {
            bool adjfloor = false;
            for (int yy = -1; yy <= 1 && !adjfloor; yy++) for (int xx = -1; xx <= 1 && !adjfloor; xx++) if (m[(x + xx) + W * (y + yy)] == 0) adjfloor = true;
            if (adjfloor) {
                // choose a color idx based on position
                int idx = ((x / 4) + (y / 4)) % 3 + 1;
                m[x + W * y] = idx;
            }
        }
    }

    // replace worldMap
    if (worldMap) free(worldMap);
    mapW = W;
    mapH = H;
    worldMap = m;
}

bool load_map_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    char line[4096];
    // read first non-comment line for dimensions
    int w = 0, h = 0;
    while (fgets(line, sizeof(line), f)) {
        // skip leading whitespace
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '#') continue;
        if (sscanf(p, "%d %d", &w, &h) == 2) break;
    }
    if (w <= 0 || h <= 0) { fclose(f); return false; }
    int *m = malloc(sizeof(int) * w * h);
    if (!m) { fclose(f); return false; }
    // initialize to walls so missing/extra data won't leave garbage
    for (int i = 0; i < w * h; ++i) m[i] = 1;
    int x = 0, y = 0;
    while (y < h && fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p) {
            // skip whitespace
            while (*p && isspace((unsigned char)*p)) p++;
            if (!*p || *p == '#') break;
            int val;
            int consumed = 0;
            if (sscanf(p, "%d%n", &val, &consumed) == 1 && consumed > 0) {
                // guard against writing past allocated rows
                if (y >= h) break;
                m[x + w * y] = val;
                x++;
                p += consumed;
                if (x >= w) { x = 0; y++; if (y >= h) break; }
            } else break;
        }
    }
    fclose(f);
    if (worldMap) free(worldMap);
    mapW = w;
    mapH = h;
    worldMap = m;
    return true;
}
