#include "story.h"
#include "dialogue.h"
#include "assets.h"
#include "city_palette.h"

enum { KEY_A=1, KEY_B=2, KEY_START=8, KEY_RIGHT=16, KEY_LEFT=32 };

extern volatile int player_x, player_y, camera_x, camera_y, facing, walk_phase;
extern volatile int current_area;

#define NORDY_X 815
#define NORDY_Y 300
#define NORDY_R 56
#define MIKE_X 144
#define MIKE_Y 508
#define MIKE_R 64
#define CATHY_X 812
#define CATHY_Y 188
#define CATHY_R 56
#define TRAIL_LEN 16
#define HP_MAX 40

static int g_mode;
static int trail_x[TRAIL_LEN];
static int trail_y[TRAIL_LEN];
static int trail_f[TRAIL_LEN];
static int trail_n;

static int bench_x;
static int bench_w;
static int bench_hits;
static int bench_flash;

static int wheel_sel;
static int wheel_loose;
static int wheel_tight;
static int wheel_phase;

static int drive_step;
static int drive_x;
static int drive_y;

static int mash_hp;
static int mash_last;
static int mash_a;
static int mash_b;
static int mash_pulse;
static int mash_saw75;
static int mash_saw50;
static int mash_saw25;

static const unsigned short pal_jib[16] = {
    0,5285,3234,7399,11824,28639,14889,5281,7396,3234,5340,5285,11824,9512,10568,3234
};
static const unsigned short pal_nick[16] = {
    0,7332,5355,10570,14896,28639,10570,3234,7396,3234,5340,5285,14896,9512,10568,3234
};
static const unsigned short pal_mike[16] = {
    0,7332,5355,11833,18207,28639,14889,9668,11980,6355,8605,11560,14889,15197,16245,6568
};

static int near(int x, int y, int cx, int cy, int r)
{
    int dx = x - cx, dy = y - cy;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx + dy < r;
}

static int in_rect(int x, int y, int x0, int y0, int x1, int y1)
{
    return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}

static void copy16(volatile unsigned short *dst, const unsigned short *src, unsigned n)
{
    while (n--) *dst++ = *src++;
}

void story_init(void)
{
    int i;
    g_mode = MODE_PLAY;
    trail_n = 0;
    for (i = 0; i < TRAIL_LEN; ++i) {
        trail_x[i] = 0;
        trail_y[i] = 0;
        trail_f[i] = 0;
    }
    copy16((volatile unsigned short *)0x05000220, pal_jib, 16);
    copy16((volatile unsigned short *)0x05000240, pal_nick, 16);
    copy16((volatile unsigned short *)0x05000260, pal_mike, 16);
}

int story_mode(void) { return g_mode; }
int story_hide_player(void) { return g_mode == MODE_DRIVE || g_mode == MODE_STAGE; }
int story_can_leave_house(void) { return flags_has(F_HAS_LAPTOP); }
int story_house_is_jib(void)
{
    return flags_has(F_QUEST_JIB) && !flags_has(F_JIB_JOINED);
}

void story_remember_step(void)
{
    int i;
    int px = player_x >> 8, py = player_y >> 8;
    if (trail_n > 0 && trail_x[0] == px && trail_y[0] == py) return;
    for (i = TRAIL_LEN - 1; i > 0; --i) {
        trail_x[i] = trail_x[i - 1];
        trail_y[i] = trail_y[i - 1];
        trail_f[i] = trail_f[i - 1];
    }
    trail_x[0] = px;
    trail_y[0] = py;
    trail_f[0] = facing;
    if (trail_n < TRAIL_LEN) ++trail_n;
}

static void start_bench(void)
{
    g_mode = MODE_BENCH;
    bench_x = 20;
    bench_w = 40;
    bench_hits = 0;
    bench_flash = 0;
}

static void start_wheel(void)
{
    g_mode = MODE_WHEEL;
    wheel_sel = 0;
    wheel_loose = 0;
    wheel_tight = 0;
    wheel_phase = 0;
}

static void start_mash(void)
{
    g_mode = MODE_MASH;
    mash_hp = HP_MAX;
    mash_last = 0;
    mash_a = 0;
    mash_b = 0;
    mash_pulse = 0;
    mash_saw75 = mash_saw50 = mash_saw25 = 0;
}

void story_place_after_drive(void)
{
    player_x = CATHY_X << 8;
    player_y = CATHY_Y << 8;
    facing = 3;
    walk_phase = 0;
    camera_x = (CATHY_X - 120);
    if (camera_x < 0) camera_x = 0;
    if (camera_x > CITY_W - 240) camera_x = CITY_W - 240;
    camera_x &= ~1;
    camera_y = CATHY_Y - 96;
    if (camera_y < 0) camera_y = 0;
    if (camera_y > CITY_H - 160) camera_y = CITY_H - 160;
}

void story_begin_drive(void)
{
    g_mode = MODE_DRIVE;
    drive_step = 0;
    drive_x = MIKE_X;
    drive_y = MIKE_Y;
    current_area = 0;
    flags_set(F_TIRE_DONE);
}

void story_skip_drive(void)
{
    g_mode = MODE_PLAY;
    flags_set(F_DROVE);
    story_place_after_drive();
    dlg_start(SCRIPT_ABHEEK);
}

void story_after_script(int script_id)
{
    if (!script_id) return;
    if (script_id == SCRIPT_NICK_START) start_bench();
    else if (script_id == SCRIPT_MIKE_START) start_wheel();
    else if (script_id == SCRIPT_MIKE_DONE || script_id == SCRIPT_GET_IN)
        story_begin_drive();
    else if (script_id == SCRIPT_ABHEEK) start_mash();
    else if (script_id == SCRIPT_ABHEEK_YIELD) dlg_start(SCRIPT_WIN);
    else if (script_id == SCRIPT_WIN) g_mode = MODE_STAGE;
}

int story_try_interact(int area, int world_x, int world_y)
{
    if (area == 1) {
        if (in_rect(world_x, world_y, 155, 55, 230, 125)) {
            if (!flags_has(F_HAS_LAPTOP)) return SCRIPT_LAPTOP;
            return SCRIPT_NOTES;
        }
        if (in_rect(world_x, world_y, 10, 30, 75, 80)) {
            if (flags_has(F_QUEST_JIB) && !flags_has(F_HAS_DRINK))
                return SCRIPT_FRIDGE;
            if (flags_has(F_HAS_DRINK)) return SCRIPT_FRIDGE_EMPTY;
            return SCRIPT_BOOKS;
        }
        if (in_rect(world_x, world_y, 10, 50, 80, 130)) {
            if (flags_has(F_HAS_DRINK) && !flags_has(F_JIB_JOINED))
                return SCRIPT_JIB_WAKE;
            return SCRIPT_JIB_SLEEP;
        }
        return SCRIPT_NONE;
    }
    if (area == 2) {
        if (in_rect(world_x, world_y, 8, 50, 95, 135)) {
            if (!flags_has(F_JIB_JOINED)) return SCRIPT_NOT_YET;
            if (flags_has(F_NICK_JOINED)) return SCRIPT_HINT;
            return SCRIPT_NICK_START;
        }
        return SCRIPT_NONE;
    }
    if (near(world_x, world_y, NORDY_X, NORDY_Y, NORDY_R)) {
        if (flags_has(F_QUEST_JIB)) return SCRIPT_NORDY_REPEAT;
        return SCRIPT_NORDY;
    }
    if (near(world_x, world_y, MIKE_X, MIKE_Y, MIKE_R)) {
        if (!flags_has(F_NICK_JOINED)) return SCRIPT_NOT_YET;
        if (!flags_has(F_TIRE_DONE)) return SCRIPT_MIKE_START;
        if (!flags_has(F_DROVE)) return SCRIPT_GET_IN;
        return SCRIPT_HINT;
    }
    if (near(world_x, world_y, CATHY_X, CATHY_Y, CATHY_R)) {
        if (!flags_has(F_DROVE)) return SCRIPT_HINT;
        if (!flags_has(F_ABHEEK_DONE)) return SCRIPT_ABHEEK;
        return SCRIPT_WIN;
    }
    if (flags_has(F_INTRO_DONE)) return SCRIPT_HINT;
    return SCRIPT_NONE;
}

static void update_bench(unsigned pressed)
{
    int green0 = 118, green1 = 121;
    int red0, red1, hit;
    bench_x += 2;
    if (bench_x > 220 - bench_w) bench_x = 20;
    if (bench_flash) --bench_flash;
    if (!(pressed & KEY_A)) return;
    red0 = bench_x;
    red1 = bench_x + bench_w;
    hit = red0 <= green1 && red1 >= green0;
    if (hit) {
        bench_w -= 6;
        if (bench_w < 8) bench_w = 8;
        ++bench_hits;
        bench_flash = 10;
        if (bench_hits >= 6) {
            g_mode = MODE_PLAY;
            dlg_start(SCRIPT_NICK_DONE);
        }
    } else {
        bench_w += 10;
        if (bench_w > 100) bench_w = 100;
    }
}

static void update_wheel(unsigned pressed)
{
    if (pressed & KEY_LEFT) wheel_sel = wheel_sel ? wheel_sel - 1 : 4;
    if (pressed & KEY_RIGHT) wheel_sel = wheel_sel == 4 ? 0 : wheel_sel + 1;
    if (!(pressed & KEY_A)) return;
    if (wheel_phase == 0) {
        wheel_loose |= 1 << wheel_sel;
        if (wheel_loose == 31) wheel_phase = 1;
    } else {
        wheel_tight |= 1 << wheel_sel;
        if (wheel_tight == 31) {
            g_mode = MODE_PLAY;
            dlg_start(SCRIPT_MIKE_DONE);
        }
    }
}

static void update_drive(unsigned keys, unsigned pressed)
{
    static const int wx[] = {144, 180, 220, 220, 400, 620, 815, 812};
    static const int wy[] = {508, 500, 420, 332, 330, 330, 280, 188};
    int tx, ty, dx, dy, speed;
    if (pressed & KEY_START) {
        story_skip_drive();
        return;
    }
    if (drive_step >= 7) {
        g_mode = MODE_PLAY;
        flags_set(F_DROVE);
        story_place_after_drive();
        dlg_start(SCRIPT_ABHEEK);
        return;
    }
    tx = wx[drive_step + 1];
    ty = wy[drive_step + 1];
    dx = tx - drive_x;
    dy = ty - drive_y;
    speed = (keys & KEY_RIGHT) ? 3 : 2;
    if (dx > speed) dx = speed;
    else if (dx < -speed) dx = -speed;
    if (dy > speed) dy = speed;
    else if (dy < -speed) dy = -speed;
    drive_x += dx;
    drive_y += dy;
    if (drive_x == tx && drive_y == ty) ++drive_step;
    camera_x = drive_x - 80;
    if (camera_x < 0) camera_x = 0;
    if (camera_x > CITY_W - 240) camera_x = CITY_W - 240;
    camera_x &= ~1;
    camera_y = drive_y - 100;
    if (camera_y < 0) camera_y = 0;
    if (camera_y > CITY_H - 160) camera_y = CITY_H - 160;
    player_x = drive_x << 8;
    player_y = drive_y << 8;
}

static void update_mash(unsigned keys, unsigned pressed)
{
    int dmg = 0;
    ++mash_pulse;
    if (mash_a) --mash_a;
    if (mash_b) --mash_b;
    if (pressed & KEY_A) {
        dmg = (mash_last == KEY_B) ? 2 : 1;
        mash_last = KEY_A;
        mash_a = 8;
    } else if (pressed & KEY_B) {
        dmg = (mash_last == KEY_A) ? 2 : 1;
        mash_last = KEY_B;
        mash_b = 8;
    }
    if (!dmg) return;
    mash_hp -= dmg;
    if (mash_hp < 0) mash_hp = 0;
    if (mash_hp == 0) {
        g_mode = MODE_PLAY;
        dlg_start(SCRIPT_ABHEEK_YIELD);
        return;
    }
    if (!mash_saw75 && mash_hp <= 30) {
        mash_saw75 = 1;
        dlg_start(SCRIPT_ABHEEK_75);
    } else if (!mash_saw50 && mash_hp <= 20) {
        mash_saw50 = 1;
        dlg_start(SCRIPT_ABHEEK_50);
    } else if (!mash_saw25 && mash_hp <= 10) {
        mash_saw25 = 1;
        dlg_start(SCRIPT_ABHEEK_25);
    }
    (void)keys;
}

void story_update(unsigned keys, unsigned pressed)
{
    if (g_mode == MODE_BENCH) update_bench(pressed);
    else if (g_mode == MODE_WHEEL) update_wheel(pressed);
    else if (g_mode == MODE_DRIVE) update_drive(keys, pressed);
    else if (g_mode == MODE_MASH) update_mash(keys, pressed);
}

static void pixel(volatile unsigned short *page, int x, int y, unsigned c)
{
    volatile unsigned short *p;
    unsigned short old;
    if (x < 0 || y < 0 || x >= 240 || y >= 160) return;
    p = page + y * 120 + (x >> 1);
    old = *p;
    *p = (x & 1) ? (old & 255) | (c << 8) : (old & 0xff00) | c;
}

static void fill(volatile unsigned short *page, int x, int y, int w, int h, unsigned c)
{
    int yy, xx;
    for (yy = y; yy < y + h; ++yy)
        for (xx = x; xx < x + w; ++xx)
            pixel(page, xx, yy, c);
}

static void text_at(volatile unsigned short *page, int x, int y, const char *s)
{
    while (*s && x < 232) {
        unsigned c = (unsigned char)*s++;
        unsigned tile = font_tiles[c >= 32 && c < 96 ? c - 32 : 0];
        const unsigned char *glyph = (const unsigned char *)bg_tiles + tile * 64;
        int yy, xx;
        for (yy = 0; yy < 8; ++yy)
            for (xx = 0; xx < 8; ++xx)
                if (glyph[yy * 8 + xx] == 53)
                    pixel(page, x + xx, y + yy, 253);
        x += 8;
    }
}

static void draw_box(volatile unsigned short *page)
{
    char line[25];
    const char *name;
    fill(page, 0, 112, 240, 48, 252);
    fill(page, 2, 114, 236, 44, 252);
    fill(page, 4, 116, 232, 2, 253);
    name = dlg_speaker_name();
    if (name) {
        fill(page, 8, 104, 56, 12, 252);
        text_at(page, 12, 106, name);
        fill(page, 8, 102, 4, 2, 253);
        fill(page, 8, 100, 4, 2, 253);
    }
    dlg_visible_line(0, line, 25);
    text_at(page, 12, 124, line);
    dlg_visible_line(1, line, 25);
    text_at(page, 12, 136, line);
    if (dlg_page_done())
        fill(page, 224, 148, 6, 6, 253);
}

static void draw_bench(volatile unsigned short *page)
{
    unsigned red = (current_area == 2) ? 211u : 213u;
    fill(page, 16, 140, 208, 16, 252);
    fill(page, 20, 144, 200, 8, 255);
    fill(page, 118, 142, 4, 12, 254);
    fill(page, bench_x, 144, bench_w, 8, bench_flash ? 255 : red);
}

static void draw_wheel(volatile unsigned short *page)
{
    static const int lx[5] = {120, 147, 136, 104, 93};
    static const int ly[5] = {42, 62, 94, 94, 62};
    int i;
    fill(page, 70, 24, 100, 88, 252);
    text_at(page, 80, 28, wheel_phase ? "A: TIGHTEN" : "A: LOOSEN");
    for (i = 0; i < 5; ++i) {
        unsigned fillc = 255;
        if (wheel_phase == 0 && (wheel_loose & (1 << i))) fillc = 254;
        if (wheel_phase == 1 && (wheel_tight & (1 << i))) fillc = 253;
        fill(page, lx[i] - 4, ly[i] - 4, 8, 8, fillc);
        if (i == wheel_sel) {
            fill(page, lx[i] - 6, ly[i] - 6, 12, 2, 253);
            fill(page, lx[i] - 6, ly[i] + 4, 12, 2, 253);
            fill(page, lx[i] - 6, ly[i] - 6, 2, 12, 253);
            fill(page, lx[i] + 4, ly[i] - 6, 2, 12, 253);
        }
    }
}

static void draw_car(volatile unsigned short *page)
{
    int sx = drive_x - camera_x - 24;
    int sy = drive_y - camera_y - 10;
    fill(page, sx, sy + 4, 48, 10, 213);
    fill(page, sx + 18, sy, 18, 8, 213);
    fill(page, sx + 6, sy + 12, 8, 6, 252);
    fill(page, sx + 32, sy + 12, 8, 6, 252);
    fill(page, sx + 40, sy + 6, 8, 6, 253);
}

static void draw_mash(volatile unsigned short *page)
{
    int bar = mash_hp * 3;
    int a_y = mash_a ? 46 : 40;
    int b_y = mash_b ? 46 : 40;
    int pulse = ((mash_pulse >> 4) & 1);
    fill(page, 56, 16, 128, 10, 252);
    fill(page, 60, 18, bar, 6, 211);
    fill(page, 56, a_y, 32, 32, mash_a ? 254 : (mash_last != KEY_A && pulse ? 255 : 253));
    fill(page, 152, b_y, 32, 32, mash_b ? 254 : (mash_last != KEY_B && pulse ? 255 : 253));
    text_at(page, 66, a_y + 12, "A");
    text_at(page, 162, b_y + 12, "B");
}

static void draw_stage(volatile unsigned short *page)
{
    fill(page, 0, 0, 240, 160, 252);
    text_at(page, 48, 40, "STEELHACKS XIII");
}

void story_draw(volatile unsigned short *page)
{
    if (g_mode == MODE_STAGE) draw_stage(page);
    if (g_mode == MODE_DRIVE) draw_car(page);
    if (g_mode == MODE_BENCH) draw_bench(page);
    if (g_mode == MODE_WHEEL) draw_wheel(page);
    if (g_mode == MODE_MASH && !dlg_active()) draw_mash(page);
    if (dlg_active()) draw_box(page);
}

static void put_actor(volatile unsigned short *oam, int slot, int wx, int wy,
                      int face, int pal, int hide)
{
    int x, y;
    volatile unsigned short *s = oam + slot * 4;
    if (hide) {
        s[0] = 0x200;
        return;
    }
    x = wx - camera_x - 8;
    y = wy - camera_y - 30;
    s[0] = (y & 255) | (2 << 14);
    s[1] = (x & 511) | (2 << 14);
    s[2] = (unsigned short)(512 + (face * 4 + ((walk_phase >> 3) & 3)) * 8 + (pal << 12));
    s[3] = 0;
}

void story_draw_actors(volatile unsigned short *oam)
{
    int hide = story_hide_player() || current_area != 0;
    int jib_x = 0, jib_y = 0, jib_f = 0, jib_on = 0;
    int nick_x = 0, nick_y = 0, nick_f = 0, nick_on = 0;
    int npc_x = 0, npc_y = 0, npc_on = 0, npc_pal = 3;

    if (current_area == 1 && story_house_is_jib()) {
        hide = 0;
        jib_on = 1;
        jib_x = 36;
        jib_y = 90;
        jib_f = 0;
        camera_x = camera_y = 0;
    } else if (current_area == 2 && flags_has(F_JIB_JOINED) && !flags_has(F_NICK_JOINED)) {
        hide = 0;
        nick_on = 1;
        nick_x = 45;
        nick_y = 100;
        nick_f = 0;
        camera_x = camera_y = 0;
    } else if (current_area == 0 && !story_hide_player()) {
        if (flags_has(F_JIB_JOINED) && trail_n > 5) {
            jib_on = 1;
            jib_x = trail_x[5];
            jib_y = trail_y[5];
            jib_f = trail_f[5];
        }
        if (flags_has(F_NICK_JOINED) && trail_n > 10) {
            nick_on = 1;
            nick_x = trail_x[10];
            nick_y = trail_y[10];
            nick_f = trail_f[10];
        }
        if (!flags_has(F_QUEST_JIB) && !dlg_active()) {
            npc_on = 1;
            npc_x = NORDY_X;
            npc_y = NORDY_Y;
            npc_pal = 3;
        } else if (flags_has(F_NICK_JOINED) && !flags_has(F_DROVE)) {
            npc_on = 1;
            npc_x = MIKE_X;
            npc_y = MIKE_Y;
            npc_pal = 3;
        } else if (flags_has(F_DROVE) && !flags_has(F_ABHEEK_DONE)) {
            npc_on = 1;
            npc_x = CATHY_X;
            npc_y = CATHY_Y;
            npc_pal = 1;
        }
    }

    put_actor(oam, 1, jib_x, jib_y, jib_f, 1, !jib_on || hide);
    put_actor(oam, 2, nick_x, nick_y, nick_f, 2, !nick_on || hide);
    put_actor(oam, 3, npc_x, npc_y, 0, npc_pal, !npc_on || hide);
}
