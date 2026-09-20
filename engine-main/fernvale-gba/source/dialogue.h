/* Quest flags + linear dialogue. Font is 8x8 ASCII 32-95 (uppercase). */
#ifndef DIALOGUE_H
#define DIALOGUE_H

enum {
    SPK_NONE = 0,
    SPK_NORDY,
    SPK_JIB,
    SPK_NICK,
    SPK_MIKE,
    SPK_ABHEEK
};

enum {
    F_INTRO_DONE  = 1u << 0,
    F_HAS_LAPTOP  = 1u << 1,
    F_QUEST_JIB   = 1u << 2,
    F_HAS_DRINK   = 1u << 3,
    F_JIB_JOINED  = 1u << 4,
    F_NICK_JOINED = 1u << 5,
    F_TIRE_DONE   = 1u << 6,
    F_DROVE       = 1u << 7,
    F_ABHEEK_DONE = 1u << 8
};

enum {
    SCRIPT_NONE = 0,
    SCRIPT_INTRO,
    SCRIPT_LAPTOP,
    SCRIPT_NO_LAPTOP,
    SCRIPT_NOTES,
    SCRIPT_BOOKS,
    SCRIPT_FRIDGE,
    SCRIPT_FRIDGE_EMPTY,
    SCRIPT_NORDY,
    SCRIPT_NORDY_REPEAT,
    SCRIPT_HINT,
    SCRIPT_JIB_SLEEP,
    SCRIPT_JIB_WAKE,
    SCRIPT_NICK_START,
    SCRIPT_NICK_DONE,
    SCRIPT_MIKE_START,
    SCRIPT_MIKE_DONE,
    SCRIPT_GET_IN,
    SCRIPT_NOT_YET,
    SCRIPT_ABHEEK,
    SCRIPT_ABHEEK_75,
    SCRIPT_ABHEEK_50,
    SCRIPT_ABHEEK_25,
    SCRIPT_ABHEEK_YIELD,
    SCRIPT_WIN
};

void flags_init(void);
int flags_has(unsigned mask);
void flags_set(unsigned mask);

int dlg_active(void);
int dlg_page_done(void);
int dlg_speaker(void);
const char *dlg_speaker_name(void);
void dlg_visible_line(int row, char *out, int cap);

void dlg_start(int script_id);
void dlg_update(unsigned pressed);
int dlg_take_finished(void);

#endif
