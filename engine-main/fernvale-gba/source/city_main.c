/* Native GBA mode-4 city exploration, with the original trainer and controls.
 * Double buffering keeps ROM-to-screen map copies off the visible page. */
#include "assets.h"
#include "city_palette.h"
#include "title_palette.h"
#include "interior_palette.h"
#include "gym_palette.h"
#include "dialogue.h"
#include "story.h"
typedef unsigned short u16;
typedef unsigned int u32;
#define R16(a) (*(volatile u16 *)(a))
#define R32(a) (*(volatile u32 *)(a))
#define DISPCNT R16(0x04000000)
#define VCOUNT R16(0x04000006)
#define KEYINPUT R16(0x04000130)
#define OAM ((volatile u16 *)0x07000000)
#define WALK_SPEED 256
#define RUN_SPEED 448
#define START_X 815
#define START_Y 330
enum { KEY_A=1,KEY_B=2,KEY_SELECT=4,KEY_START=8,
       KEY_RIGHT=16,KEY_LEFT=32,KEY_UP=64,KEY_DOWN=128 };
enum { DOWN,UP,LEFT,RIGHT };
extern const u16 city_pixels[];
extern const unsigned char city_collision[];
extern const u16 title_pixels[];
extern const unsigned char car_frames[];
extern const u16 interior_pixels[];
extern const unsigned char interior_collision[];
extern const u16 gym_pixels[];
extern const unsigned char gym_collision[];
volatile int player_x,player_y,camera_x,camera_y,facing,walk_phase,paused;
volatile unsigned frame_counter;
volatile int current_area;
static void copy16(volatile u16 *dst,const u16 *src,unsigned n)
{ while(n--) *dst++=*src++; }
static void vblank(void)
{ while(VCOUNT>=160) {} while(VCOUNT<160) {} }
static int clamp(int v,int lo,int hi)
{ return v<lo?lo:v>hi?hi:v; }
static int solid_at(int x,int y)
{
    if(current_area==1) {
        if(x<0||y<0||x>=INTERIOR_W||y>=INTERIOR_H) return 1;
        unsigned i=(y>>2)*(INTERIOR_W>>2)+(x>>2);
        return (interior_collision[i>>3]>>(i&7))&1;
    } else if(current_area==2) {
        if(x<0||y<0||x>=GYM_W||y>=GYM_H) return 1;
        unsigned i=(y>>2)*(GYM_W>>2)+(x>>2);
        return (gym_collision[i>>3]>>(i&7))&1;
    } else {
        if(x<0||y<0||x>=CITY_W||y>=CITY_H) return 1;
        unsigned i=(y>>2)*(CITY_W>>2)+(x>>2);
        return (city_collision[i>>3]>>(i&7))&1;
    }
}
static int can_stand(int x,int y)
{
    return !solid_at(x-4,y-5)&&!solid_at(x+3,y-5)
        &&!solid_at(x-4,y-1)&&!solid_at(x+3,y-1);
}
static void reset_player(void)
{
    player_x=START_X<<8; player_y=START_Y<<8;
    facing=DOWN; walk_phase=0;
}
static void move_player(unsigned keys)
{
    int dx=((keys&KEY_RIGHT)!=0)-((keys&KEY_LEFT)!=0);
    int dy=((keys&KEY_DOWN)!=0)-((keys&KEY_UP)!=0);
    if(dx) dy=0;
    if(!dx&&!dy) { walk_phase=0;return; }
    facing=dx<0?LEFT:dx>0?RIGHT:dy<0?UP:DOWN;
    int speed=keys&KEY_B?RUN_SPEED:WALK_SPEED;
    int nx=player_x+dx*speed,ny=player_y+dy*speed;
    if(can_stand(nx>>8,ny>>8)) {
        player_x=nx;player_y=ny;walk_phase+=keys&KEY_B?2:1;
    } else walk_phase=0;
}
static void draw_player(void)
{
    if(paused) { OAM[0]=0x200; return; }
    int x=(player_x>>8)-camera_x-8,y=(player_y>>8)-camera_y-30;
    OAM[0]=(y&255)|(2<<14); OAM[1]=(x&511)|(2<<14);
    /* Bitmap modes reserve the bottom 16 KiB of OBJ memory. */
    OAM[2]=512+(facing*4+((walk_phase>>3)&3))*8; OAM[3]=0;
}
static void pixel(volatile u16 *page,int x,int y,unsigned c)
{
    volatile u16 *p=page+y*120+(x>>1); u16 old=*p;
    *p=(x&1)?(old&255)|(c<<8):(old&0xff00)|c;
}
static void panel(volatile u16 *page,int x,int y,int w,int h)
{
    for(int yy=y;yy<y+h;yy++)
        for(int xx=x;xx<x+w;xx+=2) page[yy*120+(xx>>1)]=0xfcfc;
}
static void text_at(volatile u16 *page,int x,int y,const char *s)
{
    while(*s&&x<232) {
        unsigned c=(unsigned char)*s++;
        unsigned tile=font_tiles[c>=32&&c<96?c-32:0];
        const unsigned char *glyph=(const unsigned char *)bg_tiles+tile*64;
        for(int yy=0;yy<8;yy++) for(int xx=0;xx<8;xx++)
            if(glyph[yy*8+xx]==53) pixel(page,x+xx,y+yy,253);
        x+=8;
    }
}
static void draw_map(volatile u16 *page)
{
    if(current_area) {
        R32(0x040000d4)=(u32)(current_area==1?interior_pixels:gym_pixels);
        R32(0x040000d8)=(u32)page;
        R32(0x040000dc)=0x80000000|19200;
    } else {
        /* DMA3 copies visible rows to the hidden framebuffer. Camera X is even. */
        const u16 *src=city_pixels+camera_y*(CITY_W/2)+(camera_x>>1);
        for(int row=0;row<160;row++) {
            R32(0x040000d4)=(u32)(src+row*(CITY_W/2));
            R32(0x040000d8)=(u32)(page+row*120);
            R32(0x040000dc)=0x80000000|120;
        }
    }
    if(paused) {
        panel(page,24,24,192,112);
        text_at(page,72,32,current_area==1?"RED HOUSE":current_area==2?"CITY GYM":"PITTSBURGH");
        text_at(page,40,52,"D-PAD  WALK");
        text_at(page,40,68,"B      RUN");
        text_at(page,40,84,"SELECT RETURN");
        text_at(page,40,100,"START  RESUME");
        text_at(page,40,116,"UP/DOWN DOORS");
    } else if(!current_area&&frame_counter<210) {
        panel(page,8,8,104,16);text_at(page,16,12,"PITTSBURGH");
    }
}
static void fade_to(int from,int to)
{
    int step=from<to?1:-1;
    for(int level=from;;level+=step) {
        R16(0x04000054)=(u16)level;
        vblank();vblank();
        if(level==to) break;
    }
}
static void enter_red_house(void)
{
    R16(0x04000050)=0x00bf; fade_to(0,16);
    current_area=1; paused=0;
    if(!flags_has(F_HAS_LAPTOP)) {
        player_x=72<<8; player_y=100<<8;
    } else {
        player_x=120<<8; player_y=136<<8;
    }
    camera_x=camera_y=0; facing=UP; walk_phase=0;
    copy16((volatile u16 *)0x05000000,interior_palette,256);
    draw_map((volatile u16 *)0x06000000);
    draw_map((volatile u16 *)0x0600a000);
    draw_player(); fade_to(16,0);
    R16(0x04000050)=0;R16(0x04000054)=0;
}
static void leave_red_house(void)
{
    R16(0x04000050)=0x00bf; fade_to(0,16);
    current_area=0; paused=0; player_x=168<<8; player_y=266<<8;
    facing=DOWN; walk_phase=0;
    camera_x=clamp(168-120,0,CITY_W-240)&~1;
    camera_y=clamp(266-96,0,CITY_H-160);
    copy16((volatile u16 *)0x05000000,city_palette,256);
    draw_map((volatile u16 *)0x06000000);
    draw_map((volatile u16 *)0x0600a000);
    draw_player(); fade_to(16,0);
    R16(0x04000050)=0;R16(0x04000054)=0;
}
static void enter_gym(void)
{
    R16(0x04000050)=0x00bf; fade_to(0,16);
    current_area=2; paused=0; player_x=120<<8; player_y=142<<8;
    camera_x=camera_y=0; facing=UP; walk_phase=0;
    copy16((volatile u16 *)0x05000000,gym_palette,256);
    draw_map((volatile u16 *)0x06000000);
    draw_map((volatile u16 *)0x0600a000);
    draw_player(); fade_to(16,0);
    R16(0x04000050)=0;R16(0x04000054)=0;
}
static void leave_gym(void)
{
    R16(0x04000050)=0x00bf; fade_to(0,16);
    current_area=0; paused=0; player_x=956<<8; player_y=168<<8;
    facing=DOWN; walk_phase=0;
    camera_x=clamp(956-120,0,CITY_W-240)&~1;
    camera_y=clamp(168-96,0,CITY_H-160);
    copy16((volatile u16 *)0x05000000,city_palette,256);
    draw_map((volatile u16 *)0x06000000);
    draw_map((volatile u16 *)0x0600a000);
    draw_player(); fade_to(16,0);
    R16(0x04000050)=0;R16(0x04000054)=0;
}
static void draw_title(volatile u16 *page,unsigned tick)
{
    unsigned sweep=(tick>>2)&127;
    int pan=(int)(sweep>64?128-sweep:sweep)&~1;
    const u16 *src=title_pixels+(pan>>1);
    for(int row=0;row<160;row++) {
        R32(0x040000d4)=(u32)(src+row*(TITLE_W/2));
        R32(0x040000d8)=(u32)(page+row*120);
        R32(0x040000dc)=0x80000000|120;
    }
    /* Bright road streaks move left while the car stays near the camera. */
    for(int i=0;i<4;i++) {
        int x=((i*67-(int)(tick*3))&255)-20;
        for(int j=0;j<22;j++) if(x+j>=0&&x+j<240)
            pixel(page,x+j,130+i*2,254);
    }
    int car_x=tick<80?(int)tick*2-CAR_W:48+(int)((tick>>4)&1);
    int car_y=91+(int)((tick>>3)&1);
    const unsigned char *car=car_frames+((tick>>2)&3)*CAR_W*CAR_H;
    for(int y=0;y<CAR_H;y++) for(int x=0;x<CAR_W;x++) {
        unsigned c=car[y*CAR_W+x];
        int sx=car_x+x,sy=car_y+y;
        if(c!=255&&sx>=0&&sx<240&&sy>=0&&sy<160)
            pixel(page,sx,sy,c);
    }
    panel(page,40,139,160,18);
    text_at(page,44,144,"PRESS START TO PLAY");
}
static void title_screen(void)
{
    copy16((volatile u16 *)0x05000000,title_palette,256);
    OAM[0]=0x200;
    R16(0x04000050)=0x00bf; R16(0x04000054)=16;
    draw_title((volatile u16 *)0x06000000,0);
    DISPCNT=0x0404;
    fade_to(16,0);
    unsigned tick=1,previous=(~KEYINPUT)&1023,back=1;
    for(;;) {
        volatile u16 *page=(volatile u16 *)(back?0x0600a000:0x06000000);
        draw_title(page,tick++);
        vblank();
        DISPCNT=0x0404|(back?16:0);back^=1;
        unsigned keys=(~KEYINPUT)&1023;
        if((keys&~previous)&KEY_START) break;
        previous=keys;
    }
    fade_to(0,16);
}
int main(void)
{
    R16(0x04000208)=0; DISPCNT=0x80;
    copy16((volatile u16 *)0x05000000,city_palette,256);
    copy16((volatile u16 *)0x05000200,player_palette,16);
    copy16((volatile u16 *)0x06014000,player_tiles,PLAYER_TILE_HALFWORDS);
    for(int i=0;i<128;i++) { OAM[i*4]=0x200;OAM[i*4+1]=OAM[i*4+2]=OAM[i*4+3]=0; }
    R16(0x0400000c)=2;
    R16(0x04000020)=256;R16(0x04000022)=0;
    R16(0x04000024)=0;R16(0x04000026)=256;
    R32(0x04000028)=R32(0x0400002c)=0;
    title_screen();
    flags_init();
    story_init();
    current_area=0;
    reset_player();
    DISPCNT=0x1444;
    enter_red_house();
    dlg_start(SCRIPT_INTRO);
    unsigned previous=0,back=1;
    for(;;) {
        unsigned keys=(~KEYINPUT)&1023,pressed=keys&~previous;
        int finished;
        int was_talking=dlg_active();
        previous=keys;
        if(was_talking)
            dlg_update(pressed);
        else if(pressed&KEY_START && story_mode()==MODE_PLAY)
            paused=!paused;
        /* Closing a box with A must not immediately reopen the same line. */
        if(!dlg_active()&&!paused&&!was_talking) {
            if(story_mode()!=MODE_PLAY)
                story_update(keys,pressed);
            else if(!current_area&&(keys&KEY_UP)
                    &&(player_x>>8)>=164&&(player_x>>8)<=172
                    &&(player_y>>8)>=259&&(player_y>>8)<=272)
                enter_red_house();
            else if(!current_area&&(keys&KEY_UP)
                    &&(player_x>>8)>=949&&(player_x>>8)<=963
                    &&(player_y>>8)>=154&&(player_y>>8)<=174)
                enter_gym();
            else if(current_area==1&&(keys&KEY_DOWN)
                    &&(player_x>>8)>=108&&(player_x>>8)<=132
                    &&(player_y>>8)>=134) {
                if(!story_can_leave_house()) {
                    if(pressed&(KEY_DOWN|KEY_A)) dlg_start(SCRIPT_NO_LAPTOP);
                } else leave_red_house();
            }
            else if(current_area==2&&(keys&KEY_DOWN)
                    &&(player_x>>8)>=104&&(player_x>>8)<=142
                    &&(player_y>>8)>=138)
                leave_gym();
            else if(pressed&KEY_A) {
                int script=story_try_interact(current_area,player_x>>8,player_y>>8);
                if(script) dlg_start(script);
            } else {
                move_player(keys);
                story_remember_step();
            }
        }
        finished=dlg_take_finished();
        story_after_script(finished);
        if(story_mode()!=MODE_DRIVE) {
            if(current_area) camera_x=camera_y=0;
            else {
                camera_x=clamp((player_x>>8)-120,0,CITY_W-240)&~1;
                camera_y=clamp((player_y>>8)-96,0,CITY_H-160);
            }
        }
        {
            volatile u16 *page=(volatile u16 *)(back?0x0600a000:0x06000000);
            draw_map(page);
            story_draw(page);
            vblank();
            if(story_hide_player()) OAM[0]=0x200;
            else draw_player();
            story_draw_actors(OAM);
        }
        DISPCNT=0x1444|(back?16:0); back^=1; ++frame_counter;
    }
}
