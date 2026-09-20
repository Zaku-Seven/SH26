#include "dialogue.h"

typedef struct {
    unsigned char speaker;
    char line0[25];
    char line1[25];
    unsigned set_flags;
} Page;

static unsigned g_flags;
static int g_script;
static int g_page;
static int g_page_count;
static unsigned g_shown;
static int g_finished;
static const Page *g_pages;

static const Page k_intro[] = {
    { SPK_NONE, "STEELHACKS IS TODAY.", "I DONT HAVE A TEAM.", F_INTRO_DONE }
};

static const Page k_laptop[] = {
    { SPK_NONE, "GOT THE LAPTOP.", "", F_HAS_LAPTOP }
};

static const Page k_no_laptop[] = {
    { SPK_NONE, "NOT WITHOUT THE LAPTOP.", "", 0 }
};

static const Page k_notes[] = {
    { SPK_NONE, "JUST NOTES.", "", 0 }
};

static const Page k_books[] = {
    { SPK_NONE, "JUST BOOKS.", "", 0 }
};

static const Page k_fridge[] = {
    { SPK_NONE, "ONE ENERGY DRINK.", "COLD.", F_HAS_DRINK }
};

static const Page k_fridge_empty[] = {
    { SPK_NONE, "JUST CONDIMENTS.", "", 0 }
};

static const Page k_nordy[] = {
    { SPK_NORDY, "YOU LOOK WRECKED.", "", 0 },
    { SPK_NORDY, "NEED A TEAM?", "START WITH JIB.", 0 },
    { SPK_NORDY, "HES IN ATWOOD.", "PROBABLY ASLEEP.", 0 },
    { SPK_NORDY, "FRIDGE. ENERGY DRINK.", "THATS THE ONLY WAY.", F_QUEST_JIB }
};

static const Page k_nordy_repeat[] = {
    { SPK_NORDY, "ATWOOD. FRIDGE. JIB.", "", 0 }
};

static const Page k_jib_sleep[] = {
    { SPK_JIB, "ZZZZ.", "", 0 }
};

static const Page k_jib_wake[] = {
    { SPK_NONE, "HEY. STEELHACKS.", "", 0 },
    { SPK_JIB, "...HUH.", "", 0 },
    { SPK_JIB, "THAT SLAPS.", "", 0 },
    { SPK_JIB, "FINE. IM IN.", "", 0 },
    { SPK_NONE, "JIB JOINED THE PARTY!", "", 0 },
    { SPK_JIB, "NICKS AT THE REC.", "HES BENCHING.", F_JIB_JOINED }
};

static const Page k_nick_start[] = {
    { SPK_NICK, "ONE MORE.", "SPOT ME, CHAMP.", 0 },
    { SPK_JIB, "HES BEEN HERE.", "ALL MORNING.", 0 },
    { SPK_NONE, "A WHEN RED HITS GREEN!", "", 0 }
};

static const Page k_nick_done[] = {
    { SPK_NICK, "THATS A PUMP.", "IM READY.", 0 },
    { SPK_NONE, "NICK JOINED THE PARTY!", "", 0 },
    { SPK_NICK, "MIKES IN SCHENLEY.", "CARS DOWN.", F_NICK_JOINED }
};

static const Page k_mike_start[] = {
    { SPK_MIKE, "SHES DOWN A TIRE.", "", 0 },
    { SPK_MIKE, "LUGS OFF. NEW WHEEL.", "LUGS BACK ON.", 0 },
    { SPK_NICK, "EASY.", "", 0 },
    { SPK_JIB, "ILL WATCH.", "", 0 },
    { SPK_NONE, "LEFT OR RIGHT: NEXT LUG.", "A: LOOSEN.", 0 }
};

static const Page k_mike_done[] = {
    { SPK_MIKE, "GOOD.", "GET IN.", 0 },
    { SPK_NICK, "CATHY. LETS GO.", "", 0 },
    { SPK_JIB, "IF IT STARTS.", "", F_TIRE_DONE }
};

static const Page k_get_in[] = {
    { SPK_MIKE, "GET IN.", "", 0 }
};

static const Page k_not_yet[] = {
    { SPK_NONE, "NOT YET.", "", 0 }
};

static const Page k_abheek[] = {
    { SPK_ABHEEK, "CANT.", "IM IN A GAME.", 0 },
    { SPK_NONE, "STEELHACKS IS NOW.", "", 0 },
    { SPK_ABHEEK, "FORTNITE IS NOW.", "", 0 },
    { SPK_NICK, "CHAMP. NO.", "", 0 },
    { SPK_JIB, "BRO.", "", 0 },
    { SPK_ABHEEK, "LATER. I SWEAR.", "", 0 },
    { SPK_NONE, "THE TEAM GETS MAD.", "", 0 },
    { SPK_NONE, "MASH A AND B!", "", 0 }
};

static const Page k_abheek_75[] = {
    { SPK_ABHEEK, "OKAY WAIT--", "", 0 }
};

static const Page k_abheek_50[] = {
    { SPK_NICK, "GET OFF THE PHONE!", "", 0 }
};

static const Page k_abheek_25[] = {
    { SPK_JIB, "WERE LATE.", "", 0 }
};

static const Page k_abheek_yield[] = {
    { SPK_ABHEEK, "OKAY. OKAY.", "", 0 },
    { SPK_ABHEEK, "IM COMING.", "", 0 }
};

static const Page k_win[] = {
    { SPK_NONE, "WE WON STEELHACKS.", "", 0 },
    { SPK_NICK, "THATS A PUMP.", "", 0 },
    { SPK_JIB, "IM GOING BACK TO SLEEP.", "", F_ABHEEK_DONE }
};

static const Page k_hint_jib[] = {
    { SPK_NONE, "ATWOOD. FRIDGE. JIB.", "", 0 }
};
static const Page k_hint_nick[] = {
    { SPK_NONE, "FIND NICK AT THE REC.", "", 0 }
};
static const Page k_hint_mike[] = {
    { SPK_NONE, "MIKE IS IN SCHENLEY.", "CARS DOWN.", 0 }
};
static const Page k_hint_cathy[] = {
    { SPK_NONE, "HOLD RIGHT. START SKIPS.", "", 0 }
};
static const Page k_hint_nordy[] = {
    { SPK_NONE, "NORDY IS UP THE ROAD.", "", 0 }
};

static int slen(const char *s)
{
    int n = 0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
}

void flags_init(void) { g_flags = 0; }
int flags_has(unsigned mask) { return (g_flags & mask) == mask; }
void flags_set(unsigned mask) { g_flags |= mask; }

int dlg_active(void) { return g_script != SCRIPT_NONE; }
int dlg_page_done(void)
{
    if (!dlg_active()) return 1;
    return (int)g_shown >= slen(g_pages[g_page].line0) + slen(g_pages[g_page].line1);
}
int dlg_speaker(void) { return dlg_active() ? g_pages[g_page].speaker : SPK_NONE; }

const char *dlg_speaker_name(void)
{
    switch (dlg_speaker()) {
    case SPK_NORDY: return "NORDY";
    case SPK_JIB: return "JIB";
    case SPK_NICK: return "NICK";
    case SPK_MIKE: return "MIKE";
    case SPK_ABHEEK: return "ABHEEK";
    default: return 0;
    }
}

void dlg_visible_line(int row, char *out, int cap)
{
    const Page *p;
    const char *src;
    int start, take, i;
    if (!out || cap <= 0) return;
    out[0] = 0;
    if (!dlg_active() || (row != 0 && row != 1)) return;
    p = &g_pages[g_page];
    src = row ? p->line1 : p->line0;
    start = row ? slen(p->line0) : 0;
    if ((int)g_shown <= start) return;
    take = (int)g_shown - start;
    if (take > slen(src)) take = slen(src);
    if (take >= cap) take = cap - 1;
    for (i = 0; i < take; ++i) out[i] = src[i];
    out[take] = 0;
}

static void bind(int script, const Page *pages, int count)
{
    g_script = script;
    g_pages = pages;
    g_page_count = count;
    g_page = 0;
    g_shown = 0;
}

void dlg_start(int script_id)
{
    switch (script_id) {
    case SCRIPT_INTRO: bind(script_id, k_intro, 1); break;
    case SCRIPT_LAPTOP: bind(script_id, k_laptop, 1); break;
    case SCRIPT_NO_LAPTOP: bind(script_id, k_no_laptop, 1); break;
    case SCRIPT_NOTES: bind(script_id, k_notes, 1); break;
    case SCRIPT_BOOKS: bind(script_id, k_books, 1); break;
    case SCRIPT_FRIDGE: bind(script_id, k_fridge, 1); break;
    case SCRIPT_FRIDGE_EMPTY: bind(script_id, k_fridge_empty, 1); break;
    case SCRIPT_NORDY: bind(script_id, k_nordy, 4); break;
    case SCRIPT_NORDY_REPEAT: bind(script_id, k_nordy_repeat, 1); break;
    case SCRIPT_JIB_SLEEP: bind(script_id, k_jib_sleep, 1); break;
    case SCRIPT_JIB_WAKE: bind(script_id, k_jib_wake, 6); break;
    case SCRIPT_NICK_START: bind(script_id, k_nick_start, 3); break;
    case SCRIPT_NICK_DONE: bind(script_id, k_nick_done, 3); break;
    case SCRIPT_MIKE_START: bind(script_id, k_mike_start, 5); break;
    case SCRIPT_MIKE_DONE: bind(script_id, k_mike_done, 3); break;
    case SCRIPT_GET_IN: bind(script_id, k_get_in, 1); break;
    case SCRIPT_NOT_YET: bind(script_id, k_not_yet, 1); break;
    case SCRIPT_ABHEEK: bind(script_id, k_abheek, 8); break;
    case SCRIPT_ABHEEK_75: bind(script_id, k_abheek_75, 1); break;
    case SCRIPT_ABHEEK_50: bind(script_id, k_abheek_50, 1); break;
    case SCRIPT_ABHEEK_25: bind(script_id, k_abheek_25, 1); break;
    case SCRIPT_ABHEEK_YIELD: bind(script_id, k_abheek_yield, 2); break;
    case SCRIPT_WIN: bind(script_id, k_win, 3); break;
    case SCRIPT_HINT:
        if (flags_has(F_TIRE_DONE) && !flags_has(F_DROVE))
            bind(script_id, k_hint_cathy, 1);
        else if (flags_has(F_NICK_JOINED))
            bind(script_id, k_hint_mike, 1);
        else if (flags_has(F_JIB_JOINED))
            bind(script_id, k_hint_nick, 1);
        else if (flags_has(F_QUEST_JIB))
            bind(script_id, k_hint_jib, 1);
        else
            bind(script_id, k_hint_nordy, 1);
        break;
    default:
        g_script = SCRIPT_NONE;
        break;
    }
}

void dlg_update(unsigned pressed)
{
    const Page *p;
    int total;
    if (!dlg_active()) return;
    p = &g_pages[g_page];
    total = slen(p->line0) + slen(p->line1);
    if (!dlg_page_done()) {
        if (pressed & 3)
            g_shown = (unsigned)total;
        else
            g_shown += 2;
        if ((int)g_shown > total) g_shown = (unsigned)total;
        return;
    }
    if (pressed & 1) {
        flags_set(p->set_flags);
        if (g_page + 1 < g_page_count) {
            ++g_page;
            g_shown = 0;
        } else {
            g_finished = g_script;
            g_script = SCRIPT_NONE;
        }
    }
}

int dlg_take_finished(void)
{
    int id = g_finished;
    g_finished = 0;
    return id;
}
