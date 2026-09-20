/* FERNVALE: a small, original Game Boy Advance overworld.
 *
 * Hardware: ARM7TDMI, mode 0 tiled backgrounds, 4bpp OBJ character.
 * No operating system, desktop graphics library, or external game assets.
 * Edit this file in VS Code, then run: python3 build.py
 */
#include "assets.h"

typedef unsigned short u16;
typedef unsigned int u32;

/* GBA memory-mapped registers. Writes must be 16 or 32 bits, never bytes. */
#define REG16(address) (*(volatile u16 *)(address))
#define DISPCNT       REG16(0x04000000)
#define VCOUNT        REG16(0x04000006)
#define BG0CNT        REG16(0x04000008)
#define BG1CNT        REG16(0x0400000a)
#define BG2CNT        REG16(0x0400000c)
#define BG0HOFS       REG16(0x04000010)
#define BG0VOFS       REG16(0x04000012)
#define BG1HOFS       REG16(0x04000014)
#define BG1VOFS       REG16(0x04000016)
#define BG2HOFS       REG16(0x04000018)
#define BG2VOFS       REG16(0x0400001a)
#define KEYINPUT      REG16(0x04000130)
#define IME           REG16(0x04000208)
#define PALETTE_BG    ((volatile u16 *)0x05000000)
#define PALETTE_OBJ   ((volatile u16 *)0x05000200)
#define TILES_BG      ((volatile u16 *)0x06000000)
#define TILES_OBJ     ((volatile u16 *)0x06010000)
#define MAP_GROUND    ((volatile u16 *)0x0600a000) /* screenblocks 20-23 */
#define MAP_FRONT     ((volatile u16 *)0x0600c000) /* screenblocks 24-27 */
#define MAP_UI        ((volatile u16 *)0x0600f800) /* screenblock 31 */
#define OAM          ((volatile u16 *)0x07000000)

enum { KEY_A=1, KEY_B=2, KEY_SELECT=4, KEY_START=8,
       KEY_RIGHT=16, KEY_LEFT=32, KEY_UP=64, KEY_DOWN=128 };
enum { DOWN, UP, LEFT, RIGHT };

/* These are the useful first settings to experiment with. Positions use
 * 8 fractional bits so walking speed can be smooth without floating point. */
#define WORLD_SIZE   512
#define WALK_SPEED  256  /* 1 pixel/frame, about 60 pixels/second */
#define RUN_SPEED   448  /* 1.75 pixels/frame */
#define START_X     184
#define START_Y     248

/* Volatile keeps these visible to an emulator debugger in optimized builds. */
volatile int player_x, player_y;
volatile int camera_x, camera_y;
volatile unsigned frame_counter;
volatile int facing, walk_phase, message, paused;
volatile int current_area, entered_house;

static void copy16(volatile u16 *dest, const u16 *src, unsigned count)
{
    while (count--) *dest++ = *src++;
}

static void clear16(volatile u16 *dest, unsigned count)
{
    while (count--) *dest++ = 0;
}

static void wait_vblank(void)
{
    /* First leave any current blank; then wait for the next one. */
    while (VCOUNT >= 160) { }
    while (VCOUNT < 160) { }
}

static int clamp(int n, int minimum, int maximum)
{
    return n < minimum ? minimum : n > maximum ? maximum : n;
}

/* Collision is stored at 8x8 resolution, separate from the artwork. Only
 * the character's feet collide, letting their head overlap nearby scenery. */
static int solid_at(int x, int y)
{
    if (x < 0 || y < 0 || x >= WORLD_SIZE || y >= WORLD_SIZE) return 1;
    const unsigned char *map=current_area ? interior_collision_map : collision_map;
    return map[(y >> 3) * 64 + (x >> 3)] != 0;
}

static int can_stand(int x, int y)
{
    return !solid_at(x-4,y-5) && !solid_at(x+3,y-5)
        && !solid_at(x-4,y-1) && !solid_at(x+3,y-1);
}

static void text_at(int x, int y, const char *text)
{
    for (; *text && x < 30; ++text, ++x) {
        unsigned ch = (unsigned char)*text;
        MAP_UI[y*32+x] = font_tiles[(ch >= 32 && ch < 96) ? ch-32 : 0];
    }
}

static void panel(int x, int y, int w, int h)
{
    for (int yy=y; yy<y+h; ++yy)
        for (int xx=x; xx<x+w; ++xx)
            MAP_UI[yy*32+xx] = PANEL_TILE;
}

static void reset_player(void)
{
    if (current_area) {
        copy16(MAP_GROUND,ground_map,4096);
        copy16(MAP_FRONT,foreground_map,4096);
    }
    current_area=0;
    player_x = START_X << 8;
    player_y = START_Y << 8;
    facing = DOWN;
    walk_phase = 0;
    message = 0;
}

static int nearby_door(void)
{
    if (current_area || facing!=UP) return 0;
    int x=player_x>>8, y=player_y>>8;
    if (x>=88 && x<=104 && y>=199 && y<=216) return 1;
    if (x>=264 && x<=280 && y>=199 && y<=216) return 2;
    if (x>=104 && x<=120 && y>=383 && y<=400) return 3;
    return 0;
}

static void enter_house(int house)
{
    entered_house=house;
    current_area=1;
    copy16(MAP_GROUND,interior_ground_map,4096);
    copy16(MAP_FRONT,interior_foreground_map,4096);
    player_x=120<<8;
    player_y=132<<8;
    facing=UP;
    walk_phase=0;
}

static void leave_house(void)
{
    copy16(MAP_GROUND,ground_map,4096);
    copy16(MAP_FRONT,foreground_map,4096);
    current_area=0;
    if (entered_house==1) { player_x=96<<8; player_y=208<<8; }
    else if (entered_house==2) { player_x=272<<8; player_y=208<<8; }
    else { player_x=112<<8; player_y=392<<8; }
    facing=DOWN;
    walk_phase=0;
}

static int nearby_sign(void)
{
    /* Look at the tile directly in front of the player. */
    int x = (player_x >> 8), y = (player_y >> 8) - 3;
    if (facing == UP) y -= 14;
    if (facing == DOWN) y += 14;
    if (facing == LEFT) x -= 14;
    if (facing == RIGHT) x += 14;
    if (x >= 200 && x < 232 && y >= 220 && y < 244) return 1;
    if (x >= 344 && x < 376 && y >= 236 && y < 260) return 2;
    if (x >= 216 && x < 248 && y >= 380 && y < 404) return 3;
    return 0;
}

static void draw_ui(void)
{
    clear16(MAP_UI, 1024);
    if (paused) {
        panel(3,4,24,12);
        text_at(10,5,"FERNVALE");
        text_at(5,7,"D-PAD  WALK");
        text_at(5,9,"B      RUN");
        text_at(5,10,"A      READ A SIGN");
        text_at(5,11,"A      ENTER A HOUSE");
        text_at(5,12,"SELECT RETURN TO TOWN");
        text_at(5,14,"START  RESUME");
    } else if (message) {
        panel(1,14,28,5);
        if (message==1) {
            text_at(2,15,"WELCOME TO FERNVALE!");
            text_at(2,16,"A QUIET PLACE TO BEGIN.");
            text_at(2,17,"A / B TO CLOSE");
        } else if (message==2) {
            text_at(2,15,"WILLOW CREEK");
            text_at(2,16,"CROSS THE WOODEN BRIDGE.");
            text_at(2,17,"A / B TO CLOSE");
        } else {
            text_at(2,15,"THE SOUTH GARDEN");
            text_at(2,16,"TAKE THE LONG WAY HOME.");
            text_at(2,17,"A / B TO CLOSE");
        }
    } else if (frame_counter < 210) {
        panel(1,1,11,2);
        text_at(2,1,"FERNVALE");
        text_at(1,18,"D-PAD WALK  B RUN  START HELP");
    }
}

static void move_player(unsigned keys)
{
    int dx = ((keys & KEY_RIGHT)!=0) - ((keys & KEY_LEFT)!=0);
    int dy = ((keys & KEY_DOWN)!=0) - ((keys & KEY_UP)!=0);
    /* Classic four-direction movement; no faster diagonal walking. */
    if (dx) dy=0;
    if (!dx && !dy) { walk_phase=0; return; }
    facing = dx<0 ? LEFT : dx>0 ? RIGHT : dy<0 ? UP : DOWN;
    int speed = (keys & KEY_B) ? RUN_SPEED : WALK_SPEED;
    int nx=player_x+dx*speed, ny=player_y+dy*speed;
    if (can_stand(nx>>8,ny>>8)) {
        player_x=nx;
        player_y=ny;
        walk_phase += (keys & KEY_B) ? 2 : 1;
        if (current_area && facing==DOWN && (player_y>>8)>=140
            && (player_x>>8)>=108 && (player_x>>8)<=132) leave_house();
    } else {
        walk_phase=0;
    }
}

static void draw_player(void)
{
    /* The sprite is 16x32. Its feet sit 30 pixels below its top. */
    int x=(player_x>>8)-camera_x-8;
    int y=(player_y>>8)-camera_y-30;
    int anim=(walk_phase>>3)&3;
    int tile=(facing*4+anim)*8;
    OAM[0]=(u16)((y&255) | (2<<14)); /* vertical rectangle */
    OAM[1]=(u16)((x&511) | (2<<14)); /* size 2: 16x32 */
    OAM[2]=(u16)(tile | (2<<10));    /* priority 2, palette 0 */
    OAM[3]=0;
}

int main(void)
{
    IME=0;
    DISPCNT=0x0080; /* forced blank while the graphics are loaded */
    copy16(PALETTE_BG,bg_palette,256);
    copy16(PALETTE_OBJ,player_palette,16);
    copy16(TILES_BG,bg_tiles,BG_TILE_HALFWORDS);
    copy16(TILES_OBJ,player_tiles,PLAYER_TILE_HALFWORDS);
    copy16(MAP_GROUND,ground_map,4096);
    copy16(MAP_FRONT,foreground_map,4096);
    clear16(MAP_UI,1024);
    for (unsigned i=0; i<128; ++i) {
        OAM[i*4]=0x0200; /* hide every unused hardware sprite */
        OAM[i*4+1]=OAM[i*4+2]=OAM[i*4+3]=0;
    }
    BG0CNT=(31<<8) | (1<<7);               /* UI: priority 0, 256x256 */
    BG1CNT=(24<<8) | (1<<7) | (3<<14) | 1; /* tree tops: priority 1 */
    BG2CNT=(20<<8) | (1<<7) | (3<<14) | 3; /* ground: priority 3 */
    BG0HOFS=BG0VOFS=0;
    frame_counter=0;
    paused=0;
    reset_player();
    unsigned previous=0;
    /* Mode 0, three backgrounds, objects, and linear sprite tile layout. */
    DISPCNT=0x1740;

    for (;;) {
        wait_vblank();
        unsigned keys=(~KEYINPUT)&0x03ff;
        unsigned pressed=keys & ~previous;
        previous=keys;
        ++frame_counter;

        if (pressed & KEY_START) paused=!paused;
        if (!paused) {
            if (pressed & KEY_SELECT) reset_player();
            if (message) {
                if (pressed & (KEY_A|KEY_B)) message=0;
            } else if (pressed & KEY_A) {
                int door=nearby_door();
                if (door) enter_house(door);
                else message=nearby_sign();
            } else {
                move_player(keys);
            }
        }
        camera_x=current_area ? 0 : clamp((player_x>>8)-120,0,WORLD_SIZE-240);
        camera_y=current_area ? 0 : clamp((player_y>>8)-96,0,WORLD_SIZE-160);
        BG1HOFS=BG2HOFS=(u16)camera_x;
        BG1VOFS=BG2VOFS=(u16)camera_y;
        /* Cycle two highlight colors. The map itself stays in VRAM. */
        unsigned wave=(frame_counter>>4)&1;
        PALETTE_BG[20]=bg_palette[wave?21:20];
        PALETTE_BG[21]=bg_palette[wave?20:21];
        draw_player();
        draw_ui();
    }
}
