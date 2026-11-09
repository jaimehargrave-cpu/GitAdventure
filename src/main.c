#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>
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
#define ENEMY_COUNT 9
#define FONT_SCALE 2
#define AREA_VILLAGE 0
#define AREA_FOREST 1
#define AREA_CAVE 2

typedef struct { float x, y; int hp; } Actor;
typedef struct { float x, y; int hp, max_hp, kind, area; float speed; bool alive; } Enemy;
typedef struct { float x, y; int area; bool found; } Herb;

static SDL_Rect elder = { 220, 220, 24, 28 };
static Actor player;
static Enemy enemies[ENEMY_COUNT];
static Herb herbs[HERB_COUNT];
static bool walls[WORLD_H][WORLD_W];
static int area, face_x, face_y, herbs_found, monsters_defeated, quest_stage;
static float attack_timer, damage_flash;
static bool talking, victory;
static int dialogue_page, dialogue_count;
static const char *dialogue[3];
static const char *area_names[] = { "SUNLIT VILLAGE", "WHISPERING WOODS", "OLD MINE" };

static const uint8_t *glyph(char ch) {
    static const uint8_t blank[7] = {0};
    static const uint8_t font[][7] = {
        {0,0,0,0,0,0,0},{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},
        {17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},
        {16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},
        {31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},
        {17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},{14,17,19,21,25,17,14},
        {4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},
        {31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14},{4,4,4,4,4,0,4},{0,0,0,0,0,4,8},{0,4,4,31,4,4,0},
        {0,0,0,0,0,0,31},{0,0,14,1,15,17,15}
    };
    static const char chars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:!?'-./";
    ch = (char)toupper((unsigned char)ch);
    const char *found = strchr(chars, ch);
    return found ? font[found - chars] : blank;
}

static void color(SDL_Renderer *r, Uint8 red, Uint8 green, Uint8 blue) { SDL_SetRenderDrawColor(r, red, green, blue, 255); }
static void fill(SDL_Renderer *r, int x, int y, int w, int h, Uint8 cr, Uint8 cg, Uint8 cb) {
    SDL_Rect rect = { x, y, w, h }; color(r, cr, cg, cb); SDL_RenderFillRect(r, &rect);
}
static void text(SDL_Renderer *r, int x, int y, const char *s, int scale, Uint8 red, Uint8 green, Uint8 blue) {
    color(r, red, green, blue);
    for (; *s; ++s, x += 6 * scale) {
        const uint8_t *rows = glyph(*s);
        for (int row = 0; row < 7; ++row) for (int col = 0; col < 5; ++col)
            if (rows[row] & (1u << (4 - col))) { SDL_Rect p = { x + col*scale, y + row*scale, scale, scale }; SDL_RenderFillRect(r, &p); }
    }
}

static void set_obstacle(int x, int y) { if (x > 0 && y > 0 && x < WORLD_W-1 && y < WORLD_H-1) walls[y][x] = true; }
static void build_area(void) {
    memset(walls, 0, sizeof(walls));
    for (int y=0; y<WORLD_H; ++y) for (int x=0; x<WORLD_W; ++x)
        walls[y][x] = (x==0 || y==0 || x==WORLD_W-1 || y==WORLD_H-1);
    /* Open the gates that connect the three explorable rooms. */
    if (area==AREA_VILLAGE) { walls[9][WORLD_W-1]=walls[10][WORLD_W-1]=false; }
    if (area==AREA_FOREST) {
        walls[9][0]=walls[10][0]=false;
        walls[WORLD_H-1][14]=walls[WORLD_H-1][15]=false;
        int trees[][2]={{3,4},{3,5},{7,3},{8,3},{12,5},{17,3},{18,3},{25,4},{26,4},{6,13},{7,13},{11,16},{20,14},{24,12},{25,12}};
        for (unsigned i=0;i<sizeof(trees)/sizeof(trees[0]);++i) set_obstacle(trees[i][0],trees[i][1]);
    }
    if (area==AREA_CAVE) {
        walls[0][14]=walls[0][15]=false;
        int rocks[][2]={{3,4},{4,4},{9,3},{10,3},{21,4},{22,4},{6,9},{7,9},{19,10},{20,10},{4,15},{5,15},{24,15},{25,15}};
        for (unsigned i=0;i<sizeof(rocks)/sizeof(rocks[0]);++i) set_obstacle(rocks[i][0],rocks[i][1]);
    }
    if (area==AREA_VILLAGE) {
        for (int x=4;x<10;++x) walls[4][x]=true;
        for (int y=4;y<8;++y) { walls[y][4]=true; walls[y][9]=true; }
        for (int x=4;x<10;++x) if(x!=6 && x!=7) walls[7][x]=true;
        walls[10][4]=walls[11][4]=walls[12][4]=true;
        walls[5][20]=walls[5][21]=walls[5][22]=true;
        walls[6][20]=walls[7][20]=walls[8][20]=true;
    }
}

static void reset_game(void) {
    area=AREA_VILLAGE; player=(Actor){480,360,6}; face_x=0; face_y=1;
    herbs_found=0; monsters_defeated=0; quest_stage=0; attack_timer=0; damage_flash=0; talking=false; victory=false;
    dialogue_page=dialogue_count=0;
    herbs[0]=(Herb){335,170,AREA_FOREST,false}; herbs[1]=(Herb){590,250,AREA_FOREST,false}; herbs[2]=(Herb){785,510,AREA_FOREST,false};
    enemies[0]=(Enemy){300,300,2,2,0,AREA_FOREST,55,true};
    enemies[1]=(Enemy){620,370,2,2,0,AREA_FOREST,55,true};
    enemies[2]=(Enemy){790,230,2,2,0,AREA_FOREST,55,true};
    enemies[3]=(Enemy){400,450,3,3,1,AREA_FOREST,75,true};
    enemies[4]=(Enemy){700,460,3,3,1,AREA_FOREST,75,true};
    enemies[5]=(Enemy){285,250,3,3,2,AREA_CAVE,38,true};
    enemies[6]=(Enemy){570,360,3,3,2,AREA_CAVE,38,true};
    enemies[7]=(Enemy){745,255,4,4,1,AREA_CAVE,70,true};
    enemies[8]=(Enemy){760,420,12,12,3,AREA_CAVE,42,true};
    build_area();
}

static bool blocked(float x,float y,int w,int h) {
    int l=(int)x/TILE, rr=((int)x+w-1)/TILE, t=(int)y/TILE, b=((int)y+h-1)/TILE;
    for(int ty=t;ty<=b;++ty) for(int tx=l;tx<=rr;++tx)
        if(tx<0||ty<0||tx>=WORLD_W||ty>=WORLD_H||walls[ty][tx]) return true;
    return false;
}
static float dist(float x1,float y1,float x2,float y2) { float x=x1-x2,y=y1-y2; return sqrtf(x*x+y*y); }
static float player_dist(float x,float y) { return dist(player.x+12,player.y+14,x,y); }
static void begin_dialogue(const char *a,const char *b,const char *c) {
    dialogue[0]=a;dialogue[1]=b;dialogue[2]=c;dialogue_count=c?3:(b?2:1);dialogue_page=0;talking=true;
}
static void talk_elder(void) {
    if(quest_stage==0) { quest_stage=1; begin_dialogue("ELDER: THE WOODS ARE OVERRUN!","GATHER 3 HERBS IN THE FOREST.","THEN DEFEAT 3 MONSTERS."); }
    else if(quest_stage==1) begin_dialogue("ELDER: FIND THE GLOWING HERBS.","THE WOODS LIE EAST OF THE VILLAGE.",NULL);
    else if(quest_stage==2) begin_dialogue("ELDER: THE HERBS WILL HELP US.","DEFEAT 3 MONSTERS IN THE WOODS.",NULL);
    else if(quest_stage==3) begin_dialogue("ELDER: A CAVE LIES SOUTH OF THE WOODS.","DEFEAT THE CAVE GUARDIAN!",