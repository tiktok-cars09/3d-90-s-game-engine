#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <stdbool.h>
#include <time.h>

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

typedef struct { int W, H; int *data; } Snap;
#define MAX_STACK 64
static Snap undoStack[MAX_STACK]; static int undoCount = 0;
static Snap redoStack[MAX_STACK]; static int redoCount = 0;

// BSP node type used by generator
typedef struct BSPNode { int x,y,w,h; int left,right; int roomx,roomy,roomw,roomh; } BSPNode;

static int find_center_x(BSPNode *nodes, int idx) {
    if (idx < 0) return 0;
    if (nodes[idx].left == -1 && nodes[idx].right == -1) return nodes[idx].roomx + nodes[idx].roomw/2;
    if (nodes[idx].left != -1) return find_center_x(nodes, nodes[idx].left);
    if (nodes[idx].right != -1) return find_center_x(nodes, nodes[idx].right);
    return nodes[idx].x + nodes[idx].w/2;
}
static int find_center_y(BSPNode *nodes, int idx) {
    if (idx < 0) return 0;
    if (nodes[idx].left == -1 && nodes[idx].right == -1) return nodes[idx].roomy + nodes[idx].roomh/2;
    if (nodes[idx].left != -1) return find_center_y(nodes, nodes[idx].left);
    if (nodes[idx].right != -1) return find_center_y(nodes, nodes[idx].right);
    return nodes[idx].y + nodes[idx].h/2;
}

static void free_snap(Snap *s) { if (s && s->data) { free(s->data); s->data = NULL; } }

static void push_undo(int *map_in, int W_in, int H_in) {
    // clear redo
    for (int i = 0; i < redoCount; i++) {
        free_snap(&redoStack[i]);
    }
    redoCount = 0;
    // if full, drop oldest (shift left)
    if (undoCount >= MAX_STACK) {
        free_snap(&undoStack[0]);
        for (int i=1;i<undoCount;i++) undoStack[i-1] = undoStack[i];
        undoCount--;
    }
    Snap *s = &undoStack[undoCount++];
    s->W = W_in; s->H = H_in; s->data = malloc(sizeof(int) * W_in * H_in);
    if (s->data) memcpy(s->data, map_in, sizeof(int) * W_in * H_in);
}

static int do_undo(int **map_p, int *W_p, int *H_p) {
    if (undoCount <= 0) return 0;
    // push current to redo
    if (redoCount >= MAX_STACK) { free_snap(&redoStack[0]); for (int i=1;i<redoCount;i++) redoStack[i-1]=redoStack[i]; redoCount--; }
    Snap *r = &redoStack[redoCount++]; r->W = *W_p; r->H = *H_p; r->data = malloc(sizeof(int) * (*W_p) * (*H_p));
    if (r->data) memcpy(r->data, *map_p, sizeof(int) * (*W_p) * (*H_p));

    // pop last undo into map
    Snap s = undoStack[undoCount-1]; undoCount--;
    free(*map_p);
    *map_p = s.data; *W_p = s.W; *H_p = s.H;
    // prevent double-free (s.data now owned by map)
    return 1;
}

static int do_redo(int **map_p, int *W_p, int *H_p) {
    if (redoCount <= 0) return 0;
    if (undoCount >= MAX_STACK) { free_snap(&undoStack[0]); for (int i=1;i<undoCount;i++) undoStack[i-1]=undoStack[i]; undoCount--; }
    Snap *u = &undoStack[undoCount++]; u->W = *W_p; u->H = *H_p; u->data = malloc(sizeof(int) * (*W_p) * (*H_p));
    if (u->data) memcpy(u->data, *map_p, sizeof(int) * (*W_p) * (*H_p));

    Snap s = redoStack[redoCount-1]; redoCount--;
    free(*map_p);
    *map_p = s.data; *W_p = s.W; *H_p = s.H;
    return 1;
}

// simple BSP generator for editor
static int *generate_bsp(int W, int H, int complexity, unsigned int seed) {
    int *m = malloc(sizeof(int) * W * H);
    if (!m) return NULL;
    for (int i=0;i<W*H;i++) m[i] = 1;
    BSPNode nodes[512]; int ncount = 0;
    nodes[ncount++] = (BSPNode){1,1,W-2,H-2,-1,-1,0,0,0,0};
    srand(seed);

    // split nodes into a BSP tree
    for (int it=0; it<complexity; ++it) {
        // pick random leaf
        int sel = -1;
        for (int i=0;i<ncount;i++) if (nodes[i].left == -1 && nodes[i].right == -1) { if (rand()% (it+2) == 0) sel = i; }
        if (sel == -1) break;
        BSPNode nd = nodes[sel];
        if (nd.w < 6 && nd.h < 6) continue;
        bool splitH = (nd.h > nd.w);
        if (nd.w > nd.h && nd.w > 12) splitH = false;
        if (nd.h > nd.w && nd.h > 12) splitH = true;
        if (splitH) {
            int minSplit = 3, maxSplit = nd.h - 3;
            if (maxSplit <= minSplit) continue;
            int s = minSplit + rand() % (maxSplit - minSplit + 1);
            // top and bottom
            nodes[ncount] = (BSPNode){nd.x, nd.y, nd.w, s, -1,-1,0,0,0,0};
            nodes[sel] = (BSPNode){nd.x, nd.y + s, nd.w, nd.h - s, -1,-1,0,0,0,0};
            nodes[sel].left = ncount; nodes[sel].right = sel; nodes[ncount].left = nodes[ncount].right = -1; ncount++;
        } else {
            int minSplit = 3, maxSplit = nd.w - 3;
            if (maxSplit <= minSplit) continue;
            int s = minSplit + rand() % (maxSplit - minSplit + 1);
            nodes[ncount] = (BSPNode){nd.x, nd.y, s, nd.h, -1,-1,0,0,0,0};
            nodes[sel] = (BSPNode){nd.x + s, nd.y, nd.w - s, nd.h, -1,-1,0,0,0,0};
            nodes[sel].left = ncount; nodes[sel].right = sel; nodes[ncount].left = nodes[ncount].right = -1; ncount++;
        }
    }

    // create rooms in leaves (ellipse rooms to avoid strict rectangles)
    for (int i=0;i<ncount;i++) if (nodes[i].left == -1 && nodes[i].right == -1) {
        BSPNode *nd = &nodes[i];
        int rw = max(3, nd->w - 2);
        int rh = max(3, nd->h - 2);
        int rx = nd->x + 1 + (rand() % (rw));
        int ry = nd->y + 1 + (rand() % (rh));
        int rw2 = (rw>3)?(3 + rand()%(rw-2)):rw;
        int rh2 = (rh>3)?(3 + rand()%(rh-2)):rh;
        if (rx + rw2 >= W-1) rw2 = W-2 - rx;
        if (ry + rh2 >= H-1) rh2 = H-2 - ry;
        nd->roomx = rx; nd->roomy = ry; nd->roomw = rw2; nd->roomh = rh2;
        // carve an ellipse rather than a rectangle
        double rxr = rw2 / 2.0; double ryr = rh2 / 2.0;
        double cx = rx + rxr; double cy = ry + ryr;
        for (int y=ry;y<ry+rh2;y++) for (int x=rx;x<rx+rw2;x++) {
            double dx = (x + 0.5 - cx) / (rxr > 0 ? rxr : 1.0);
            double dy = (y + 0.5 - cy) / (ryr > 0 ? ryr : 1.0);
            if (dx*dx + dy*dy <= 1.0) m[x + W*y] = 0;
        }
    }

    // use file-scope helpers find_center_x/ find_center_y

    // connect rooms by walking internal nodes
    for (int i=0;i<ncount;i++) {
        if (nodes[i].left != -1 && nodes[i].right != -1) {
            int x1 = find_center_x(nodes, nodes[i].left);
            int y1 = find_center_y(nodes, nodes[i].left);
            int x2 = find_center_x(nodes, nodes[i].right);
            int y2 = find_center_y(nodes, nodes[i].right);
            int cx = x1, cy = y1;
            while (cx != x2) { m[cx + W*cy] = 0; cx += (x2>cx)?1:-1; }
            while (cy != y2) { m[cx + W*cy] = 0; cy += (y2>cy)?1:-1; }
        }
    }

    // apply cellular automata smoothing to reduce boxiness
    int smoothPasses = 3;
    for (int pass = 0; pass < smoothPasses; pass++) {
        int *tmp = malloc(sizeof(int) * W * H);
        if (!tmp) break;
        for (int y=0;y<H;y++) for (int x=0;x<W;x++) tmp[x + W*y] = m[x + W*y];
        for (int y=1;y<H-1;y++) for (int x=1;x<W-1;x++) {
            int floors = 0;
            for (int yy=-1; yy<=1; yy++) for (int xx=-1; xx<=1; xx++) if (m[(x+xx) + W*(y+yy)] == 0) floors++;
            // if many neighboring floors, become floor; else wall
            if (floors >= 5) tmp[x + W*y] = 0; else tmp[x + W*y] = 1;
        }
        free(m);
        m = tmp;
    }

    // mark walls adjacent to floors as colored variants
    for (int y=1;y<H-1;y++) for (int x=1;x<W-1;x++) {
        if (m[x + W*y] == 1) {
            bool adj = false; for (int yy=-1;yy<=1 && !adj;yy++) for (int xx=-1;xx<=1 && !adj;xx++) if (m[(x+xx) + W*(y+yy)] == 0) adj = true;
            if (adj) m[x + W*y] = ((x/6 + y/6) % 3) + 1;
        }
    }
    return m;
}

int main(int argc, char **argv) {
    const char *outpath = "maps/custom.map";
    int W = 24, H = 24;
    int *map = NULL;

    // If a path was provided, try to load it. Otherwise prompt for size.
    const char *loadpath = NULL;
    if (argc > 1) loadpath = argv[1];
    if (!loadpath) {
        // ask user for desired size (press Enter for default)
        char buf[128];
        printf("Map Editor\nEnter width and height (e.g. '24 24') or press Enter for default (24 24): ");
        if (fgets(buf, sizeof(buf), stdin)) {
            int a=0,b=0;
            if (sscanf(buf, "%d %d", &a, &b) == 2 && a > 0 && b > 0) { W = a; H = b; }
        }
    }
    map = calloc(W*H, sizeof(int));
    if (!map) return 1;

    if (loadpath) {
        FILE *f = fopen(loadpath, "r");
        if (f) {
            int rw, rh;
            if (fscanf(f, "%d %d", &rw, &rh) == 2) {
                int *m = realloc(map, sizeof(int)*rw*rh);
                if (m) map = m;
                W = rw; H = rh;
                for (int y=0;y<H;y++) for (int x=0;x<W;x++) fscanf(f, "%d", &map[x + W*y]);
            }
            fclose(f);
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL init: %s\n", SDL_GetError());
        return 1;
    }

    int cell = 32;
    SDL_Window *win = SDL_CreateWindow("Map Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W*cell, H*cell, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    int paintVal = 1;
    int brush = 1;
    int complexity = 8;
    unsigned int gen_seed = (unsigned int)time(NULL);
    int prev_gx = -1, prev_gy = -1;
    int running = 1;
    // initial window title
    char titlebuf[128];
    snprintf(titlebuf, sizeof(titlebuf), "Map Editor - paint=%d brush=%d undo=%d redo=%d", paintVal, brush, undoCount, redoCount);
    SDL_SetWindowTitle(win, titlebuf);
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = 0;
                if (e.key.keysym.sym == SDLK_s) {
                    // require Ctrl+S to save
                    SDL_Keymod mods = SDL_GetModState();
                    if (mods & KMOD_CTRL) {
                        const char *savepath = outpath;
                        if (argc > 1) savepath = argv[1];
                        // ensure directory exists
                        char dir[512];
                        strncpy(dir, savepath, sizeof(dir)-1);
                        dir[sizeof(dir)-1] = '\0';
                        for (int i = (int)strlen(dir)-1; i>=0; --i) {
                            if (dir[i] == '/') { dir[i] = '\0'; break; }
                        }
                        if (strlen(dir) > 0) mkdir(dir, 0755);
                        FILE *f = fopen(savepath, "w");
                        if (f) {
                            fprintf(f, "%d %d\n", W, H);
                            for (int y=0;y<H;y++){
                                for (int x=0;x<W;x++) fprintf(f, "%d ", map[x + W*y]);
                                fprintf(f, "\n");
                            }
                            fclose(f);
                            printf("Saved %s\n", savepath);
                        } else {
                            fprintf(stderr, "Failed to save %s\n", savepath);
                        }
                    } else {
                        printf("Hold Ctrl and press S to save.\n");
                    }
                }
                    // Ctrl+Z undo, Ctrl+Y redo
                    SDL_Keymod mods = SDL_GetModState();
                    if ((mods & KMOD_CTRL) && e.key.keysym.sym == SDLK_z) {
                        if (do_undo(&map, &W, &H)) {
                            snprintf(titlebuf, sizeof(titlebuf), "Map Editor - paint=%d brush=%d undo=%d redo=%d", paintVal, brush, undoCount, redoCount);
                            SDL_SetWindowTitle(win, titlebuf);
                        }
                    }
                    if ((mods & KMOD_CTRL) && e.key.keysym.sym == SDLK_y) {
                        if (do_redo(&map, &W, &H)) {
                            snprintf(titlebuf, sizeof(titlebuf), "Map Editor - paint=%d brush=%d undo=%d redo=%d", paintVal, brush, undoCount, redoCount);
                            SDL_SetWindowTitle(win, titlebuf);
                        }
                    }
                    // brush size +/-
                    if (e.key.keysym.sym == SDLK_EQUALS) { if (brush < 16) brush++; snprintf(titlebuf, sizeof(titlebuf), "Map Editor - paint=%d brush=%d undo=%d redo=%d", paintVal, brush, undoCount, redoCount); SDL_SetWindowTitle(win, titlebuf); }
                    if (e.key.keysym.sym == SDLK_MINUS) { if (brush > 1) brush--; snprintf(titlebuf, sizeof(titlebuf), "Map Editor - paint=%d brush=%d undo=%d redo=%d", paintVal, brush, undoCount, redoCount); SDL_SetWindowTitle(win, titlebuf); }
                if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9) {
                    int n = e.key.keysym.sym - SDLK_0;
                    paintVal = n;
                    printf("paint set to %d\n", paintVal);
                }
                // BSP controls: G generate, [ ] adjust complexity, R randomize seed
                if (e.key.keysym.sym == SDLK_g) {
                    push_undo(map, W, H);
                    int *gen = generate_bsp(W, H, complexity, gen_seed);
                    if (gen) { free(map); map = gen; }
                    snprintf(titlebuf, sizeof(titlebuf), "Map Editor - paint=%d brush=%d complexity=%d seed=%u undo=%d redo=%d", paintVal, brush, complexity, gen_seed, undoCount, redoCount);
                    SDL_SetWindowTitle(win, titlebuf);
                }
                if (e.key.keysym.sym == SDLK_LEFTBRACKET) { if (complexity > 1) complexity--; snprintf(titlebuf, sizeof(titlebuf), "Map Editor - paint=%d brush=%d complexity=%d seed=%u undo=%d redo=%d", paintVal, brush, complexity, gen_seed, undoCount, redoCount); SDL_SetWindowTitle(win, titlebuf); }
                if (e.key.keysym.sym == SDLK_RIGHTBRACKET) { if (complexity < 64) complexity++; snprintf(titlebuf, sizeof(titlebuf), "Map Editor - paint=%d brush=%d complexity=%d seed=%u undo=%d redo=%d", paintVal, brush, complexity, gen_seed, undoCount, redoCount); SDL_SetWindowTitle(win, titlebuf); }
                if (e.key.keysym.sym == SDLK_r) { gen_seed = (unsigned int)time(NULL) ^ rand(); snprintf(titlebuf, sizeof(titlebuf), "Map Editor - paint=%d brush=%d complexity=%d seed=%u undo=%d redo=%d", paintVal, brush, complexity, gen_seed, undoCount, redoCount); SDL_SetWindowTitle(win, titlebuf); }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx,my;
                SDL_GetMouseState(&mx,&my);
                int winW, winH; SDL_GetWindowSize(win,&winW,&winH);
                int cols = winW / cell; int rows = winH / cell;
                int gx = mx / cell; int gy = my / cell;
                if (gx >=0 && gx < cols && gy >=0 && gy < rows) {
                        // push undo snapshot before making changes
                        push_undo(map, W, H);
                        // ensure map resized if window bigger
                        if (cols != W || rows != H) {
                            int *m = realloc(map, sizeof(int)*cols*rows);
                            if (m) {
                                // initialize new cells to 0
                                for (int y=0;y<rows;y++) for (int x=0;x<cols;x++) if (x>=W || y>=H) m[x + cols*y] = 0;
                                map = m; W = cols; H = rows;
                            }
                        }
                        int half = brush/2;
                        for (int oy = -half; oy <= half; oy++) for (int ox = -half; ox <= half; ox++) {
                            int tx = gx + ox; int ty = gy + oy;
                            if (tx>=0 && tx < W && ty>=0 && ty < H) {
                                if (e.button.button == SDL_BUTTON_LEFT) map[tx + W*ty] = paintVal;
                                else if (e.button.button == SDL_BUTTON_RIGHT) map[tx + W*ty] = 0;
                            }
                        }
                        prev_gx = gx; prev_gy = gy;
                        snprintf(titlebuf, sizeof(titlebuf), "Map Editor - paint=%d brush=%d undo=%d redo=%d", paintVal, brush, undoCount, redoCount);
                        SDL_SetWindowTitle(win, titlebuf);
                }
            } else if (e.type == SDL_MOUSEMOTION && (SDL_GetMouseState(NULL,NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))) {
                int mx,my;
                SDL_GetMouseState(&mx,&my);
                int winW, winH; SDL_GetWindowSize(win,&winW,&winH);
                int cols = winW / cell; int rows = winH / cell;
                int gx = mx / cell; int gy = my / cell;
                if (gx >=0 && gx < cols && gy >=0 && gy < rows) {
                        // first time dragging after press should have pushed undo; we push on buttondown only
                        if (cols != W || rows != H) {
                            int *m = realloc(map, sizeof(int)*cols*rows);
                            if (m) {
                                for (int y=0;y<rows;y++) for (int x=0;x<cols;x++) if (x>=W || y>=H) m[x + cols*y] = 0;
                                map = m; W = cols; H = rows;
                            }
                        }
                        // draw line from prev to current (Bresenham) for diagonal strokes
                        if (prev_gx < 0) { prev_gx = gx; prev_gy = gy; }
                        int x0 = prev_gx, y0 = prev_gy, x1 = gx, y1 = gy;
                        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
                        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
                        int err = dx + dy;
                        while (1) {
                            int half = brush/2;
                            for (int oy = -half; oy <= half; oy++) for (int ox = -half; ox <= half; ox++) {
                                int tx = x0 + ox; int ty = y0 + oy;
                                if (tx>=0 && tx < W && ty>=0 && ty < H) map[tx + W*ty] = paintVal;
                            }
                            if (x0 == x1 && y0 == y1) break;
                            int e2 = 2 * err;
                            if (e2 >= dy) { err += dy; x0 += sx; }
                            if (e2 <= dx) { err += dx; y0 += sy; }
                        }
                        prev_gx = gx; prev_gy = gy;
                }
            }
        }

        int winW, winH; SDL_GetWindowSize(win,&winW,&winH);
        SDL_SetRenderDrawColor(ren, 32,32,32,255);
        SDL_RenderClear(ren);
        int cols = winW / cell; int rows = winH / cell;
        for (int y=0;y<rows;y++){
            for (int x=0;x<cols;x++){
                int v = 0;
                if (x < W && y < H) v = map[x + W*y];
                if (v == 0) SDL_SetRenderDrawColor(ren, 50,50,50,255);
                else if (v == 1) SDL_SetRenderDrawColor(ren, 200,0,0,255);
                else if (v == 2) SDL_SetRenderDrawColor(ren, 0,200,0,255);
                else if (v == 3) SDL_SetRenderDrawColor(ren, 0,0,200,255);
                SDL_Rect r = { x*cell, y*cell, cell-1, cell-1 };
                SDL_RenderFillRect(ren, &r);
            }
        }
        // draw grid lines
        SDL_SetRenderDrawColor(ren, 24,24,24,255);
        for (int gx=0; gx<=cols; gx++) SDL_RenderDrawLine(ren, gx*cell, 0, gx*cell, rows*cell);
        for (int gy=0; gy<=rows; gy++) SDL_RenderDrawLine(ren, 0, gy*cell, cols*cell, gy*cell);
        // highlight hovered cell
        int mx, my; SDL_GetMouseState(&mx,&my);
        int hx = mx / cell; int hy = my / cell;
        if (hx >= 0 && hx < cols && hy >=0 && hy < rows) {
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 255,255,255,48);
            SDL_Rect hr = { hx*cell, hy*cell, cell-1, cell-1 };
            SDL_RenderFillRect(ren, &hr);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        }
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    // free undo/redo stacks
    for (int i=0;i<undoCount;i++) free_snap(&undoStack[i]);
    for (int i=0;i<redoCount;i++) free_snap(&redoStack[i]);
    free(map);
    return 0;
}
