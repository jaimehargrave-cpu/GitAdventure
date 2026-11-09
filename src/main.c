#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define SCREEN_W 960
#define SCREEN_H 640
#define TILE 32
#define WORLD_W (SCREEN_W / TILE)
#define WORLD_H (SCREEN_H / TILE)

/* A compact first playable slice: village exploration, one NPC, one enemy. */
typedef struct { float x, y; int hp; } Actor;
typedef struct { float x, y; int hp; bool alive; } Slime;

static SDL_Rect elder = { 220, 220, 24, 28 };
static Actor player = { 480, 360, 5 };
static Slime slime = { 700, 390, 3, true };
static bool walls[WORLD_H][WORLD_W];
static int face_x = 0, face_y = 1;
static float attack_timer = 0.0f;
static float damage_flash = 0.0f;
static bool talking = false;
static int dialogue_page = 0;

static void color(SDL_Renderer *r, Uint8 red, Uint8 green, Uint8 blue) {
    SDL_SetRenderDrawColor(r, red, green, blue, 255);
}

static void fill(SDL_Renderer *r, int x, int y, int w, int h, Uint8 cr, Uint8 cg, Uint8 cb) {
    SDL_Rect rect = { x, y, w, h };
    color(r, cr, cg, cb);
    SDL_RenderFillRect(r, &rect);
}

static void build_world(void) {
    for (int y = 0; y < WORLD_H; ++y)
        for (int x = 0; x < WORLD_W; ++x)
            walls[y][x] = (x == 0 || y == 0 || x == WORLD_W - 1 || y == WORLD_H - 1);
    /* A small house and a few trees frame the village path. */
    for (int x = 4; x < 10; ++x) walls[4][x] = true;
    for (int y = 4; y < 8; ++y) { walls[y][4] = true; walls[y][9] = true; }
    for (int x = 4; x < 10; ++x) if (x != 6 && x != 7) walls[7][x] = true;
    walls[10][4] = walls[11][4] = walls[12][4] = true;
    walls[9][13] = walls[10][13] = true;
    walls[14][22] = walls[15][22] = walls[16][22] = true;
    walls[5][20] = walls[5][21] = walls[5][22] = true;
    walls[6][20] = walls[7][20] = walls[8][20] = true;
}

static bool blocked(float x, float y, int w, int h) {
    int left = (int)x / TILE, right = ((int)x + w - 1) / TILE;
    int top = (int)y / TILE, bottom = ((int)y + h - 1) / TILE;
    for (int ty = top; ty <= bottom; ++ty)
        for (int tx = left; tx <= right; ++tx)
            if (tx < 0 || ty < 0 || tx >= WORLD_W || ty >= WORLD_H || walls[ty][tx]) return true;
    return false;
}

static float distance_to(float x, float y) {
    float dx = (player.x + 12) - x, dy = (player.y + 14) - y;
    return sqrtf(dx * dx + dy * dy);
}

static void try_attack(void) {
    if (attack_timer > 0.0f || talking) return;
    attack_timer = 0.32f;
    if (!slime.alive) return;
    float sx = slime.x + 12, sy = slime.y + 12;
    float px = player.x + 12 + face_x * 20, py = player.y + 14 + face_y * 20;
    float dx = sx - px, dy = sy - py;
    if (sqrtf(dx * dx + dy * dy) < 34.0f) {
        --slime.hp;
        if (slime.hp <= 0) slime.alive = false;
    }
}

static void update(float dt, const Uint8 *keys) {
    attack_timer = fmaxf(0.0f, attack_timer - dt);
    damage_flash = fmaxf(0.0f, damage_flash - dt);
    if (talking) return;

    float dx = 0, dy = 0;
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) dx -= 1;
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) dx += 1;
    if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) dy -= 1;
    if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) dy += 1;
    if (dx != 0 || dy != 0) {
        float length = sqrtf(dx * dx + dy * dy);
        dx /= length; dy /= length;
        if (fabsf(dx) > fabsf(dy)) { face_x = dx > 0 ? 1 : -1; face_y = 0; }
        else { face_y = dy > 0 ? 1 : -1; face_x = 0; }
        float speed = 150.0f * dt;
        if (!blocked(player.x + dx * speed, player.y, 24, 26)) player.x += dx * speed;
        if (!blocked(player.x, player.y + dy * speed, 24, 26)) player.y += dy * speed;
    }

    if (slime.alive && distance_to(slime.x + 12, slime.y + 12) < 150.0f) {
        float ex = player.x - slime.x, ey = player.y - slime.y;
        float length = sqrtf(ex * ex + ey * ey);
        if (length > 0.01f) {
            ex /= length; ey /= length;
            float speed = 48.0f * dt;
            if (!blocked(slime.x + ex * speed, slime.y, 24, 24)) slime.x += ex * speed;
            if (!blocked(slime.x, slime.y + ey * speed, 24, 24)) slime.y += ey * speed;
        }
        if (distance_to(slime.x + 12, slime.y + 12) < 30.0f && damage_flash <= 0.0f) {
            if (player.hp > 0) --player.hp;
            damage_flash = 0.75f;
        }
    }
}

static void draw_world(SDL_Renderer *r) {
    color(r, 48, 112, 67);
    SDL_RenderClear(r);
    for (int y = 0; y < WORLD_H; ++y) {
        for (int x = 0; x < WORLD_W; ++x) {
            int px = x * TILE, py = y * TILE;
            if ((x + y) % 2 == 0) fill(r, px, py, TILE, TILE, 53, 122, 72);
            if (walls[y][x]) {
                fill(r, px, py, TILE, TILE, 77, 65, 48);
                fill(r, px + 3, py + 3, TILE - 6, TILE - 6, 105, 82, 54);
            }
        }
    }
    /* House roof and doorway. */
    fill(r, 4 * TILE + 7, 3 * TILE + 4, 5 * TILE + 18, 19, 130, 55, 48);
    fill(r, 4 * TILE + 16, 5 * TILE + 13, 4 * TILE, 2 * TILE + 14, 181, 139, 86);
    fill(r, 6 * TILE, 6 * TILE + 8, 2 * TILE, TILE + 23, 89, 58, 39);
    /* Elder */
    fill(r, elder.x + 3, elder.y + 9, 18, 18, 66, 94, 184);
    fill(r, elder.x + 6, elder.y, 12, 12, 225, 180, 137);
    fill(r, elder.x + 5, elder.y - 2, 14, 5, 96, 59, 38);
    /* Slime */
    if (slime.alive) {
        fill(r, (int)slime.x, (int)slime.y + 7, 24, 17, 91, 220, 105);
        fill(r, (int)slime.x + 4, (int)slime.y + 3, 16, 7, 91, 220, 105);
        fill(r, (int)slime.x + 6, (int)slime.y + 10, 3, 3, 20, 35, 25);
        fill(r, (int)slime.x + 15, (int)slime.y + 10, 3, 3, 20, 35, 25);
        fill(r, (int)slime.x, (int)slime.y - 6, slime.hp * 8, 3, 230, 77, 72);
    }
    /* Player */
    fill(r, (int)player.x + 3, (int)player.y + 9, 18, 17, damage_flash > 0 ? 235 : 70, 95, damage_flash > 0 ? 90 : 220);
    fill(r, (int)player.x + 6, (int)player.y, 12, 13, 232, 183, 142);
    fill(r, (int)player.x + 5, (int)player.y - 2, 14, 5, 75, 43, 29);
    /* HUD panels and hearts/health indicators. */
    fill(r, 12, 12, 204, 48, 25, 34, 39);
    fill(r, 18, 18, 192, 36, 45, 54, 57);
    for (int i = 0; i < 5; ++i) fill(r, 24 + i * 35, 28, 26, 13, i < player.hp ? 210 : 89, i < player.hp ? 63 : 70, i < player.hp ? 68 : 70);
    fill(r, 20, 70, 148, 8, 34, 45, 47);
    fill(r, 20, 70, slime.alive ? 86 : 0, 8, 105, 224, 120);

    if (talking) {
        fill(r, 126, 500, 708, 108, 24, 31, 40);
        fill(r, 132, 506, 696, 96, 68, 87, 100);
        fill(r, 150, 528, 390, 8, 233, 227, 202);
        fill(r, 150, 546, 580, 6, 210, 218, 210);
        fill(r, 150, 563, 485, 6, 210, 218, 210);
        fill(r, 760, 570, 42, 8, 240, 199, 98);
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("GitAdventure - Village", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                           SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    if (!window) { fprintf(stderr, "Window creation failed: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) { fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    build_world();
    bool running = true;
    Uint64 previous = SDL_GetPerformanceCounter();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                SDL_Keycode key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE) running = false;
                if ((key == SDLK_SPACE) && player.hp > 0) try_attack();
                if (key == SDLK_e) {
                    if (talking) { talking = false; dialogue_page = 0; }
                    else if (distance_to(elder.x + 12, elder.y + 14) < 65.0f) {
                        talking = true; dialogue_page = (dialogue_page + 1) % 2;
                    }
                }
            }
        }
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - previous) / (float)SDL_GetPerformanceFrequency();
        previous = now;
        if (dt > 0.05f) dt = 0.05f;
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if (player.hp > 0) update(dt, keys);
        char title[128];
        snprintf(title, sizeof(title), "GitAdventure | HP %d/5 | %s | WASD move, E talk, Space attack, Esc quit",
                 player.hp, slime.alive ? "Slime nearby" : "Village safe");
        SDL_SetWindowTitle(window, title);
        draw_world(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(1);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
