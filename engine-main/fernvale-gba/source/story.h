#ifndef STORY_H
#define STORY_H

enum {
    MODE_PLAY = 0,
    MODE_BENCH,
    MODE_WHEEL,
    MODE_DRIVE,
    MODE_MASH,
    MODE_STAGE
};

void story_init(void);
int story_mode(void);
int story_hide_player(void);
int story_can_leave_house(void);
int story_house_is_jib(void);
void story_after_script(int script_id);
void story_update(unsigned keys, unsigned pressed);
void story_draw(volatile unsigned short *page);
void story_draw_actors(volatile unsigned short *oam);
void story_remember_step(void);
int story_try_interact(int area, int world_x, int world_y);
void story_begin_drive(void);
void story_skip_drive(void);
void story_place_after_drive(void);

#endif
