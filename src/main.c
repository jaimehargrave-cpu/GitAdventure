#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#define SCREEN_W 960
#define SCREEN_H 640
#define TILE 32
#define WORLD_W (SCREEN_W / TILE)
#define WORLD_H (SCREEN_H / TILE)
#define HERB_COUNT 3
#define FONT_SCALE 2

typedef struct { float x, y; int hp; } Actor;
typedef struct { float x, y; int hp; bool alive; } Slime;
typedef struct { float x, y; bool found; } Herb;

static SDL_Rect elder = { 220, 220, 24, 28 };
static Actor player;
static Slime slime;
static Herb herbs[HERB_COUNT];
static bool walls[WORLD_H][WORLD_W];
static int face_x, face_y, herbs_found;
static float attack_timer, damage_flash;
static bool talking, quest_started, victory;
static int dialogue_page, dialogue_count;
static const char *dialogue[3];

static const uint8_t *glyph(char ch) {
    static const uint8_t blank[7] = {0};
    /* Five pixels wide, seven pixels tall; kept embedded so no font assets are needed. */
    static const uint8_t font[][7] = {
        {0,0,0,0,0,0,0}, /* space */
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
        {7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8}, {14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14}, {4,4,4,4,4,0,4}, {0,0,0,0,0,4,8},
        {0,4,4,31,4,4,0}, {0,0,0,0,0,0,31}, {0,0,14,1,15,17,15}
    };
    static const char chars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:!?'-./";
    ch = (char)toupper((unsigned char)ch);
    const char *found = strchr(chars, ch);
    if (!found) return blank;
    return font[found - chars];
}

static void color(SDL_Renderer *r, Uint8 red, Uint8 green, Uint8 blue) {
    SDL_SetRenderDrawColor(r, red, green, blue, 255);
}

static void fill(SDL_Renderer *r, int x, int y, int w, int h, Uint8 cr, Uint8 cg, Uint8 cb) {
    SDL_Rect rect = { x, y, w, h };
    color(r, cr, cg, cb);
    SDL_RenderFillRect(r, &rect);
}

static void draw_text(SDL_Renderer *r, int x, int y, const char *text, int scale, Uint8 red, Uint8 green, Uint8 blue) {
    color(r, red, green, blue);
    for (; *text; ++text, x += 6 * scale) {
        const uint8_t *rows = glyph(*text);
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (rows[row] & (1u << (4 - col))) {
                    SDL_Rect pixel = { x + col * scale, y + row * scale, scale, scale };
                    SDL_RenderFillRect(r, &pixel);
                }
    }
}

static void build_world(void) {
    for (int y = 0; y < WORLD_H; ++y)
        for (int x = 0; x < WORLD_W; ++x)
            walls[y][x] = (x == 0 || y == 0 || x == WORLD_W - 1 || y == WORLD_H - 1);
    for (int x = 4; x < 10; ++x) walls[4][x] = true;
    for (int y = 4; y < 8; ++y) { walls[y][4] = true; walls[y][9] = true; }
    for (int x = 4; x < 10; ++x) if (x != 6 && x != 7) walls[7][x] = true;
    walls[10][4] = walls[11][4] = walls[12][4] = true;
    walls[9][13] = walls[10][13] = true;
    walls[14][22] = walls[15][22] = walls[16][22] = true;
    walls[5][20] = walls[5][21] = walls[5][22] = true;
    walls[6][20] = walls[7][20] = walls[8][20] = true;
}

static void reset_game(void) {
    player = (Actor){ 480, 360, 5 };
    slime = (Slime){ 700, 390, 3, true };
    herbs[0] = (Herb){ 335, 170, false };
    herbs[1] = (Herb){ 590, 250, false };
    herbs[2] = (Herb){ 785, 510, false };
    face_x = 0; face_y = 1; herbs_found = 0;
    attack_timer = 0; damage_flash = 0; talking = false;
    quest_started = false; victory = false;
    dialogue_page = 0; dialogue_count = 0;
}

static bool blocked(float x, float y, int w, int h) {
    int left = (int)x / TILE, right = ((int)x + w - 1) / TILE;
    int top = (int)y / TILE, bottom = ((int)y + h - 1) / TILE;
    for (int ty = top; ty <= bottom; ++ty)
        for (int tx = left; tx <= right; ++tx)
            if (tx < 0 || ty < 0 || tx >= WORLD_W || ty >= WORLD_H || walls[ty][tx]) return true;
    return false;
}

static float distance_between(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2, dy = y1 - y2;
    return sqrtf(dx * dx + dy * dy);
}

static float player_distance(float x, float y) {
    return distance_between(player.x + 12, player.y + 14, x, y);
}

static void begin_dialogue(const char *a, const char *b, const char *c) {
    dialogue[0] = a; dialogue[1] = b; dialogue[2] = c;
    dialogue_count = c ? 3 : (b ? 2 : 1);
    dialogue_page = 0;
    talking = true;
}

static void talk_to_elder(void) {
    if (!quest_started) {
        quest_started = true;
        begin_dialogue("ELDER: TROUBLE IN THE VILLAGE!", "FIND 3 HERBS AND DEFEAT THE SLIME.", "BRING THE GOOD NEWS BACK TO ME.");
    } else if (herbs_found < HERB_COUNT) {
        begin_dialogue("ELDER: THE HERBS GLOW GREEN.", "LOOK AROUND THE VILLAGE PATH.", NULL);
    } else if (slime.alive) {
        begin_dialogue("ELDER: WELL DONE FINDING HERBS!", "NOW DEFEAT THE SLIME TO THE EAST.", NULL);
    } else {
        victory = true;
        begin_dialogue("ELDER: YOU SAVED OUR VILLAGE!", "TAKE THIS AS A TOKEN OF THANKS.", "PRESS R TO PLAY AGAIN.");
    }
}

static void interact(void) {
    if (talking) {
        if (++dialogue_page >= dialogue_count) talking = false;
        return;
    }
    if (player_distance(elder.x + 12, elder.y + 14) < 64.0f) {
        talk_to_elder();
        return;
    }
    if (!quest_started) return;
    for (int i = 0; i < HERB_COUNT; ++i) {
        if (!herbs[i].found && player_distance(herbs[i].x + 8, herbs[i].y + 8) < 32.0f) {
            herbs[i].found = true;
            ++herbs_found;
            char line[48];
            snprintf(line, sizeof(line), "HERB FOUND! %d OF 3 COLLECTED.", herbs_found);
            begin_dialogue(line, herbs_found == HERB_COUNT ? "NOW FIND THE SLIME AND ATTACK!" : "FIND THE OTHER HERBS!", NULL);
            return;
        }
    }
}

static void try_attack(void) {
    if (attack_timer > 0.0f || talking || victory || player.hp <= 0) return;
    attack_timer = 0.28f;
    if (!slime.alive) return;
    float px = player.x + 12 + face_x * 22, py = player.y + 14 + face_y * 22;
    if (distance_between(px, py, slime.x + 12, slime.y + 12) < 36.0f) {
        --slime.hp;
        if (slime.hp <= 0) slime.alive = false;
    }
}

static void update(float dt, const Uint8 *keys) {
    attack_timer = fmaxf(0.0f, attack_timer - dt);
    damage_flash = fmaxf(0.0f, damage_flash - dt);
    if (talking || victory || player.hp <= 0) return;

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
        float speed = 155.0f * dt;
        if (!blocked(player.x + dx * speed, player.y, 24, 26)) player.x += dx * speed;
        if (!blocked(player.x, player.y + dy * speed, 24, 26)) player.y += dy * speed;
    }

    if (!quest_started) return;
    if (slime.alive && player_distance(slime.x + 12, slime.y + 12) < 180.0f) {
        float ex = player.x - slime.x, ey = player.y - slime.y;
        float length = sqrtf(ex * ex + ey * ey);
        if (length > 0.01f) {
            ex /= length; ey /= length;
            float speed = 45.0f * dt;
            if (!blocked(slime.x + ex * speed, slime.y, 24, 24)) slime.x += ex * speed;
            if (!blocked(slime.x, slime.y + ey * speed, 24, 24)) slime.y += ey * speed;
        }
        if (player_distance(slime.x + 12, slime.y + 12) < 30.0f && damage_flash <= 0.0f) {
            --player.hp;
            damage_flash = 0.8f;
        }
    }
    for (int i = 0; i < HERB_COUNT; ++i)
        if (quest_started && !herbs[i].found && player_distance(herbs[i].x + 8, herbs[i].y + 8) < 18.0f)
            interact();
}

static void draw_world(SDL_Renderer *r) {
    color(r, 48, 112, 67);
    SDL_RenderClear(r);
    for (int y = 0; y < WORLD_H; ++y) for (int x = 0; x < WORLD_W; ++x) {
        int px = x * TILE, py = y * TILE;
        if ((x + y) % 2 == 0) fill(r, px, py, TILE, TILE, 53, 122, 72);
        if (walls[y][x]) {
            fill(r, px, py, TILE, TILE, 77, 65, 48);
            fill(r, px + 3, py + 3, TILE - 6, TILE - 6, 105, 82, 54);
        }
    }
    fill(r, 4 * TILE + 7, 3 * TILE + 4, 5 * TILE + 18, 19, 130, 55, 48);
    fill(r, 4 * TILE + 16, 5 * TILE + 13, 4 * TILE, 2 * TILE + 14, 181, 139, 86);
    fill(r, 6 * TILE, 6 * TILE + 8, 2 * TILE, TILE + 23, 89, 58, 39);

    for (int i = 0; i < HERB_COUNT; ++i) if (!herbs[i].found) {
        int x = (int)herbs[i].x, y = (int)herbs[i].y;
        fill(r, x + 6, y + 9, 5, 10, 48, 110, 44);
        fill(r, x + 1, y + 1, 6, 6, 177, 235, 119);
        fill(r, x + 10, y, 6, 6, 177, 235, 119);
        fill(r, x + 6, y - 3, 6, 6, 235, 236, 145);
    }
    fill(r, elder.x + 3, elder.y + 9, 18, 18, 66, 94, 184);
    fill(r, elder.x + 6, elder.y, 12, 12, 225, 180, 137);
    fill(r, elder.x + 5, elder.y - 2, 14, 5, 96, 59, 38);

    if (slime.alive) {
        fill(r, (int)slime.x, (int)slime.y + 7, 24, 17, 91, 220, 105);
        fill(r, (int)slime.x + 4, (int)slime.y + 3, 16, 7, 91, 220, 105);
        fill(r, (int)slime.x + 6, (int)slime.y + 10, 3, 3, 20, 35, 25);
        fill(r, (int)slime.x + 15, (int)slime.y + 10, 3, 3, 20, 35, 25);
        fill(r, (int)slime.x, (int)slime.y - 6, slime.hp * 8, 3, 230, 77, 72);
    }
    fill(r, (int)player.x + 3, (int)player.y + 9, 18, 17, damage_flash > 0 ? 235 : 70, 95, damage_flash > 0 ? 90 : 220);
    fill(r, (int)player.x + 6, (int)player.y, 12, 13, 232, 183, 142);
    fill(r, (int)player.x + 5, (int)player.y - 2, 14, 5, 75, 43, 29);
    if (attack_timer > 0.13f) {
        int sx = (int)player.x + 12 + face_x * 22;
        int sy = (int)player.y + 12 + face_y * 22;
        fill(r, sx - (face_y ? 3 : 1), sy - (face_x ? 3 : 1), face_y ? 6 : 22, face_x ? 6 : 22, 245, 231, 173);
    }

    fill(r, 12, 12, 304, 70, 25, 34, 39);
    fill(r, 18, 18, 292, 60, 45, 54, 57);
    char status[64];
    snprintf(status, sizeof(status), "HP %d/5   HERBS %d/3", player.hp, herbs_found);
    draw_text(r, 28, 27, status, FONT_SCALE, 255, 242, 206);
    if (!quest_started) draw_text(r, 28, 51, "TALK TO THE ELDER: E", FONT_SCALE, 204, 229, 193);
    else if (herbs_found < HERB_COUNT) draw_text(r, 28, 51, "FIND 3 GLOWING HERBS", FONT_SCALE, 204, 229, 193);
    else if (slime.alive) draw_text(r, 28, 51, "DEFEAT THE SLIME: SPACE", FONT_SCALE, 255, 193, 149);
    else draw_text(r, 28, 51, "RETURN TO THE ELDER: E", FONT_SCALE, 204, 229, 193);

    if (!talking && !victory && player.hp > 0 && player_distance(elder.x + 12, elder.y + 14) < 64.0f)
        draw_text(r, (int)elder.x - 12, (int)elder.y - 21, "E: TALK", FONT_SCALE, 255, 255, 220);
    if (talking) {
        fill(r, 76, 494, 808, 124, 24, 31, 40);
        fill(r, 82, 500, 796, 112, 68, 87, 100);
        draw_text(r, 104, 520, dialogue[dialogue_page], FONT_SCALE, 255, 246, 218);
        draw_text(r, 104, 550, "E: CONTINUE", FONT_SCALE, 215, 229, 205);
    }
    if (player.hp <= 0) {
        fill(r, 276, 270, 408, 92, 24, 31, 40);
        draw_text(r, 316, 289, "YOU FELL IN BATTLE", FONT_SCALE, 255, 209, 195);
        draw_text(r, 358, 320, "PRESS R TO RESTART", FONT_SCALE, 255, 246, 218);
    }
    if (victory) {
        fill(r, 276, 270, 408, 92, 24, 31, 40);
        draw_text(r, 364, 289, "QUEST COMPLETE!", FONT_SCALE, 255, 242, 180);
        draw_text(r, 358, 320, "PRESS R TO RESTART", FONT_SCALE, 255, 246, 218);
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("GitAdventure - The Village Quest", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                           SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    if (!window) { fprintf(stderr, "Window creation failed: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) { fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    build_world();
    reset_game();
    bool running = true;
    Uint64 previous = SDL_GetPerformanceCounter();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                SDL_Keycode key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE) running = false;
                if (key == SDLK_r && (player.hp <= 0 || victory)) reset_game();
                if (key == SDLK_SPACE) try_attack();
                if (key == SDLK_e) interact();
            }
        }
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - previous) / (float)SDL_GetPerformanceFrequency();
        previous = now;
        if (dt > 0.05f) dt = 0.05f;
        update(dt, SDL_GetKeyboardState(NULL));
        draw_world(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(1);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
