/* All YomiGo screens. Desktop and HTPC share the flow and differ in chrome, layout and input hints. */
#include "yg.h"
#include "ui.h"
#include "version_gen.h"
#include "strokes_gen.h"

#ifdef YG_HTPC
#define HTPC 1
#else
#define HTPC 0
#endif

/* Sakura palette. HTPC: deep night and gold. Desktop: cream paper and wine. */
#if HTPC
#define C_BG    0x1B1033
#define C_PANEL 0x35205A
#define C_HI    0x6A4A9A
#define C_LO    0x120A24
#define C_INK   0xF4EEDC
#define C_DIM   0xB6A8C8
#define C_ACC   0xFFC24A
#define C_PINK  0xFF8FB0
#define C_GOOD  0x7CE08A
#define C_BAD   0xFF6A6A
#else
#define C_BG    0xF7EFE0
#define C_PANEL 0xFBF5E8
#define C_HI    0xFFF7EE
#define C_LO    0xA88A84
#define C_INK   0x2E1B20
#define C_DIM   0x7A6468
#define C_ACC   0x9C2A44
#define C_PINK  0xF2B6C4
#define C_GOOD  0x2E7D4F
#define C_BAD   0xB3242C
#endif
#define C_WINE  0x7A1F38
#define C_PLUM  0x4A1A2E

/* layout */
#if HTPC
#define LH 17
#define SAFE (crt_mode() ? 18 : 10)
#else
#define LH 20
#define SAFE 0
#define RAIL 104
#define HEAD 40
#define FOOT 22
#endif
#if HTPC
#define FBY_BTN (FBH - SAFE - 50)
#else
#define FBY_BTN (FBH - FOOT - 38)
#endif

typedef enum { S_SPLASH, S_PROFILES, S_NAME, S_PLACE_INTRO, S_PLACE_RESULT, S_HOME, S_CLASS, S_TEACH, S_QUIZ, S_RESULT, S_CHAT, S_TASK, S_CARDS, S_EXAMS, S_QUEST, S_LIBRARY, S_SETTINGS, S_BIND, S_FIRST, S_GENKI, S_TYPING, S_READ, S_MYWORDS, S_MYADD, S_SHADOW, S_TEXT } Screen;
typedef enum { QM_WARMUP, QM_DRILL, QM_DAILY, QM_HW, QM_REVIEW, QM_EXAM, QM_PLACE, QM_PRACTICE, QM_LISTEN } QuizMode;

static Screen scr = S_SPLASH;
static Profile P;
static int have_profile;
static int anim;

static Quiz Z;
static QuizMode qm;
static int qi, qright, qanswered, qchoice, qok;
static char input[128];
static int exam_id;
static int place_section, place_passed;

static int page, tab, teach_card;
static int chat_id, chat_turn, chat_fail, chat_show;
static char result_title[96], result_body[256];
static int result_good;
static Screen result_next;
static int task_kind;
static int note_skip, note_more;
static char name_buf[YG_NAME];
static int new_material;
static int kb_x, kb_y;
static int q_idk, flash_t;
static const char *flash_msg = "";
static void flash(const char *m) { flash_msg = m; flash_t = 40; plat_sound(0); }
static void flash_add(int r) { flash(r > 0 ? "Added!" : r == 0 ? "Already added" : "Deck is full"); }
static int my_sel, my_confirm, add_step;
static int q_said = -1, sh_i, sh_n, sh_said;
#define TX_MAX 400 /* room for an episode's transcript */
static char tx_lines[TX_MAX][256];
static int tx_n, tx_sel, tx_pos, tx_len, tx_res[8], tx_nres, tx_watch, tx_seq, tx_top, tx_ocr, tx_page;
static int tx_listen, tx_ask; /* 1 transcribing, 2 downloading the speech model; tx_ask: the download was explained */
static const char *tx_msg = "";
/* New Japanese lines go on top. Anything without Japanese, or the same as the newest line, is ignored. */
static int tx_add(const char *t) {
    int added = 0;
    while (*t) {
        char l[256]; int n = 0, jp = 0, i, cut = 0;
        while (*t && *t != '\n' && *t != '\r') { if (n < 250) l[n++] = *t; else cut = 1; t++; }
        if (cut) { /* a long line is cut, but never in the middle of a character */
            while (n > 0 && ((unsigned char)l[n - 1] & 0xC0) == 0x80) n--;
            if (n > 0) n--;
        }
        l[n] = 0;
        while (*t == '\n' || *t == '\r') t++;
        for (i = 0; i < n; i++) if ((unsigned char)l[i] >= 0xE3) jp = 1;
        if (!jp || (tx_n && !ys_cmp(tx_lines[0], l))) continue;
        for (i = (tx_n < TX_MAX ? tx_n : TX_MAX - 1); i > 0; i--) ys_cpy(tx_lines[i], tx_lines[i - 1], 256);
        ys_cpy(tx_lines[0], l, 256);
        if (tx_n < TX_MAX) tx_n++;
        tx_sel = 0; tx_page = 0; tx_pos = 0; tx_nres = 0; tx_len = 0; added++;
    }
    return added;
}
static void tx_poll(void) {
    static char clip[8192];
    int s, code;
    if (tx_watch) {
        s = plat_clip_seq();
        if (s != tx_seq) { tx_seq = s; plat_clip_get(clip, sizeof clip); tx_add(clip); }
    }
    if (tx_listen && (code = plat_listen_poll()) != -2) {
        static char out[262144], done[160];
        int get = tx_listen == 2, n;
        tx_listen = 0;
        if (get) tx_msg = code == 0 ? "The speech model is ready. Press Transcribe and pick a file." : "The download failed or didn't check out, so nothing was kept. Try again later.";
        else if (code == 0 || code == 8) {
            n = plat_read("listen_out.txt", out, sizeof out - 1);
            out[n > 0 ? n : 0] = 0;
            ys_fmt(done, sizeof done, code == 0 ? "Done: %d lines. Japanese subtitles are saved beside the file, as .ja.srt." : "Done: %d lines. A subtitle file was already beside it, so it was left alone.", tx_add(out), 0);
            tx_msg = done;
        }
        else if (code == 2) tx_msg = "The speech model didn't load. Close some programs and try again.";
        else if (code == 3) tx_msg = "Couldn't get the sound out of that file. MP3, M4A, WAV and MP4 work.";
        else if (code == 4 || code == (int)0xC000001D) tx_msg = "Transcribing needs a processor with AVX2, from about 2013 or later.";
        else if (code == 6) tx_msg = "No speech found in that file.";
        else if (code == 7) tx_msg = "";
        else tx_msg = "Transcribing needs 64-bit Windows 10 or later.";
    }
    if (tx_ocr && (code = plat_ocr_poll()) != -2) {
        tx_ocr = 0;
        if (code == 0) { static char out[16384]; int n = plat_read("ocr_out.txt", out, sizeof out - 1); if (n > 0) { out[n] = 0; tx_msg = tx_add(out) ? "" : "No Japanese text found in that picture."; } else tx_msg = "No text found in that picture."; }
        else if (code == 2) tx_msg = "Windows has no Japanese text recognition. Add Japanese under Settings > Time & language > Language & region, then check its optical character recognition is installed.";
        else if (code == 4) tx_msg = "That picture is too large to read. Try a smaller part of the screen.";
        else if (code == 3) tx_msg = "Couldn't read that picture.";
        else tx_msg = "Reading pictures needs Windows 10 or later.";
    }
}
static ShadowLine sh_lines[4];
static char add_word[128], add_reading[128];
static const char *copy_src;
static int field_on, field_x, field_y;
#define TY_MAX 10
#if HTPC
#define TY_REPS 2 /* each letter is several button presses on a controller */
#else
#define TY_REPS 3
#endif
static short ty_cards[TY_MAX];
static int ty_n, ty_i, ty_rep, ty_bad;
static int rd_id, rd_q, rd_right, rd_show_en, rd_skip, rd_pick, rd_order[3];

/* ---- immersion: labels move into Japanese as the learner can read them ---- */
static const char *L(const char *en, const char *ja, int from_stage) {
    if (have_profile && !P.force_en && profile_stage(&P) >= from_stage) return ja;
    return en;
}

static void save_profile(void) {
    static char b[YG_SAVE_CAP], fn[64];
    int n;
    if (!have_profile) return;
    n = profile_save_text(&P, b, sizeof b);
    if (n <= 0) return;
    ys_cpy(fn, P.name, sizeof fn);
    ys_cat(fn, ".sav", sizeof fn);
    plat_write(fn, b, n);
}
static void go(Screen s) { scr = s; ui_reset_focus(); page = 0; anim = 0; }

/* ---- widgets ---- */
static int button(int x, int y, int w, int h, const char *label) {
    int hit = ui_hit(x, y, w, h), f = ui_focused(), tw = text_w(label);
    if (tw > w - 4) { tw = w - 4; }
    if (HTPC) {
        fill(x, y, w, h, f ? C_ACC : C_PANEL);
        frame(x, y, w, h, f ? 0xFFFFFF & 0xFFF0C0 : C_HI);
        wrap(x + (w - tw) / 2, y + (h - 16) / 2, tw, label, f ? 0x1B1033 : C_INK, 16, 0, 1);
    } else {
        int press = f && IN.mdown == 1;
        bevel(x, y, w, h, press ? C_LO : C_HI, f ? 0xF8D4DC : 0xEFD9D0, press ? C_HI : C_LO);
        frame(x - 1, y - 1, w + 2, h + 2, C_PLUM);
        wrap(x + (w - tw) / 2 + press, y + (h - 16) / 2 + press, tw, label, C_INK, 16, 0, 1);
        if (f) { int i; for (i = 3; i < w - 3; i += 2) { fill(x + i, y + 3, 1, 1, C_PLUM); fill(x + i, y + h - 4, 1, 1, C_PLUM); } }
    }
    if (hit) plat_sound(1);
    return hit;
}
static void panel(int x, int y, int w, int h) {
    if (HTPC) { fill(x, y, w, h, C_PANEL); frame(x, y, w, h, C_HI); }
    else { bevel(x, y, w, h, C_LO, C_PANEL, C_HI); }
}
static void progress(int x, int y, int w, int h, int v, int max, unsigned c) {
    int f = max > 0 ? (v > max ? max : v) * (w - 2) / max : 0;
    fill(x, y, w, h, HTPC ? C_LO : 0xD9C8BC);
    frame(x, y, w, h, HTPC ? C_HI : C_PLUM);
    fill(x + 1, y + 1, f, h - 2, c);
}
static void prompt_btn(int *x, int y, int action, const char *label) {
    /* Button glyphs follow the connected controller */
    static const char *const xb[] = {"A", "B", "X", "Y", "LB", "RB", "Menu"};
    static const char *const ps[] = {"\xC3\x97", "O", "[]", "/\\", "L1", "R1", "Opt"};
    static const char *const kb[] = {"Ent", "Esc", "F3", "F1", "PgUp", "PgDn", "Spc"};
    static const unsigned col[] = {0x2FA84F, 0xC8323C, 0x3A6FD8, 0xD8A820, 0x707070, 0x707070, 0x707070};
    int st = pad_style();
    const char *g = (st == 2 ? ps : st == 0 ? kb : xb)[action];
    int w = text_w(g) + 8;
    if (st == 4) {
        /* N64 colours, as in the launcher: A blue, B green, Start red, Z grey, L and R silver, C yellow */
        static const char *const n[] = {"A", "B", "Z", "", "L", "R", "S"};
        static const unsigned c64[] = {0x2B5FD9, 0x1FA84A, 0x5A606B, 0xF6C41B, 0xC8CCD4, 0xC8CCD4, 0xE02E3A};
        static const signed char disc[16] = {5, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 5};
        int j, square = action == 2 || action == 4 || action == 5;
        w = 16;
        for (j = 0; j < 16; j++) { int in = square ? 0 : disc[j]; fill(*x + in, y + j, 16 - 2 * in, 1, c64[action]); }
        if (action == 3) tri3d(*x + 8, y + 3, *x + 3, y + 12, *x + 13, y + 12, 0x2A2010);
        else text(*x + 4, y, n[action], action == 4 || action == 5 ? 0x20242C : 0xFFFFFF);
    } else {
    fill(*x, y, w, 16, col[action]);
    text(*x + 4, y, g, 0xF4F4F4);
    }
    *x += w + 3;
    *x += text(*x, y, label, C_INK) + 8;
}

/* Double-size when it fits the screen, normal size when it doesn't. Returns the height used. */
static int big_text(int x, int y, const char *s, unsigned c, int w) {
    if (2 * text_w(s) <= w) { text2(x, y, s, c); return 33; }
    return LH * wrap(x, y, w, s, c, LH, 0, 2) + 1;
}
static void sakura(int x, int y, int s) {
    static const char *const R[] = {
        "....11...11....", "...1221.1221...", "...12221.2221..", "..1122212221...", ".122222232221..",
        "12222223222221.", "122222333222221", ".1122333433211.", "..122233322221.", ".12222232222221",
        "1222221.1222221", ".12221...12221.", "..121.....121..", "...1.......1..."};
    static const unsigned pal[] = {0, 0xC85A7C, 0xF9C3D2, 0xE8789A, 0x9C2A44};
    bitmap(x, y, R, 14, pal, s);
}

/* ---- chrome ---- */
#if !HTPC
static const struct { const char *en, *ja; int st; Screen s; } NAV[] = {
    {"Today", "きょう", 2, S_HOME}, {"Flashcards", "カード", 3, S_CARDS}, {"Exams", "しけん", 2, S_EXAMS},
    {"Quest", "クエスト", 3, S_QUEST}, {"Library", "本だな", 5, S_LIBRARY}, {"Text", "テキスト", 3, S_TEXT}, {"Settings", "せってい", 2, S_SETTINGS}};
#define NNAV 7
static void chrome(const char *title, const char *sub) {
    int i;
    washi(0, 0, FBW, FBH, C_BG, 0xEADFCC);
    gradient(0, 0, FBW, HEAD, C_PLUM, C_WINE);
    { int a; for (a = 0; a < FBW; a += 16) fill(a, HEAD - 3, 8, 1, C_PINK); }
    sakura(8, 12, 1);
    text2(30, 4, title, 0xFFF4E0);
    if (sub) text(FBW - 12 - text_w(sub), 12, sub, C_PINK);
    fill(0, HEAD, FBW, 1, C_PLUM);
    gradient(0, HEAD + 1, RAIL, FBH - HEAD - FOOT - 1, 0x5A2238, 0x3A1420);
    fill(RAIL, HEAD, 1, FBH - HEAD - FOOT, C_PLUM);
    sakura(RAIL / 2 - 15, FBH - FOOT - 40, 2);
    bevel(0, FBH - FOOT, FBW, FOOT, C_HI, 0xEFD9D0, C_LO);
    if (have_profile) {
        char b[64]; int x = 8;
        x += text(x, FBH - FOOT + 3, P.name, C_INK) + 10;
        x += text(x, FBH - FOOT + 3, rank_name(profile_rank(&P), profile_stage(&P) >= 3 && !P.force_en), C_ACC) + 10;
        ys_fmt(b, sizeof b, "XP %d", P.xp, 0); x += text(x, FBH - FOOT + 3, b, C_INK) + 10;
        ys_fmt(b, sizeof b, "Streak %d", P.streak, 0); text(x, FBH - FOOT + 3, b, C_INK);
    }
    text(FBW - 8 - text_w("SENKOSOFT"), FBH - FOOT + 3, "SENKOSOFT", C_DIM);
}
/* Drawn last so the main content gets focus first. Only on main pages: a class has no side doors. */
static void rail(void) {
    int i, main_page = 0;
    for (i = 0; i < NNAV; i++) if (scr == NAV[i].s) main_page = 1;
    if (!have_profile || !main_page) return;
    for (i = 0; i < NNAV; i++) {
        int y = HEAD + 10 + i * 42, cur = scr == NAV[i].s, hit;
        hit = ui_hit(4, y, RAIL - 8, 36);
        if (cur) bevel(4, y, RAIL - 8, 36, 0xFFF7EE, C_BG, C_LO);
        else bevel(4, y, RAIL - 8, 36, 0x8A4A5E, ui_focused() ? 0x8C3A52 : 0x5E2A3C, 0x2A0E18);
        text_c(4 + (RAIL - 8) / 2, y + 10, L(NAV[i].en, NAV[i].ja, NAV[i].st), cur ? C_INK : 0xFFF4E0);
        if (hit) { go(NAV[i].s); plat_sound(1); return; }
    }
}
#define CX (RAIL + 12)
#define CY (HEAD + 10)
#define CW (FBW - RAIL - 24)
#define CH (FBH - HEAD - FOOT - 20)
#else
/* Low-poly torii gate, drawn fresh each frame with flat shading */
static void torii(int cx, int cy, int size, int spin) {
    static const short B[][6] = { /* boxes: x,y,z half sizes and centres in 1/16 */
        {-6, 0, 0, 1, 8, 1}, {6, 0, 0, 1, 8, 1}, {0, -8, 0, 11, 1, 1}, {0, -5, 0, 8, 1, 1}};
    static const unsigned shade[3] = {0xD23A3A, 0x9E2424, 0xE86060};
    int b;
    for (b = 0; b < 4; b++) {
        int s, f;
        static const signed char faces[3][4][3] = {
            {{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},
            {{1,-1,1},{1,-1,-1},{1,1,-1},{1,1,1}},
            {{-1,-1,-1},{1,-1,-1},{1,-1,1},{-1,-1,1}}};
        for (s = 0; s < 2; s++) for (f = 0; f < 3; f++) {
            int X[4], Y[4], i, sx = s ? -1 : 1;
            if (s && f != 1) continue;
            for (i = 0; i < 4; i++) {
                int x = B[b][0] + faces[f][i][0] * sx * B[b][3], y = B[b][1] + faces[f][i][1] * B[b][4], z = B[b][2] + faces[f][i][2] * B[b][5];
                int rx = (x * icos(spin) - z * isin(spin)) / 256, rz = (x * isin(spin) + z * icos(spin)) / 256;
                int d = 64 + rz;
                X[i] = cx + rx * size * 32 / d; Y[i] = cy + y * size * 32 / d;
            }
            { int cross = (X[1] - X[0]) * (Y[2] - Y[0]) - (Y[1] - Y[0]) * (X[2] - X[0]);
              if (cross > 0) { tri3d(X[0], Y[0], X[1], Y[1], X[2], Y[2], shade[f]); tri3d(X[0], Y[0], X[2], Y[2], X[3], Y[3], shade[f]); } }
        }
    }
}
static void chrome(const char *title, const char *sub) {
    int s = SAFE;
    gradient(0, 0, FBW, FBH, 0x24103F, 0x4A1A4A);
    fill(0, 0, FBW, s + 20, 0x180A2C);
    text(s + 2, s + 3, title, C_ACC);
    if (sub) text(FBW - s - 2 - text_w(sub), s + 3, sub, C_PINK);
    fill(0, s + 20, FBW, 1, C_ACC);
}
#define CX (SAFE + 2)
#define CY (SAFE + 25)
#define CW (FBW - 2 * SAFE - 4)
#define CH (FBH - 2 * SAFE - 44)
static void footer(const char *ok, const char *back) {
    int x = SAFE + 2, y = FBH - SAFE - 17;
    if (ok) prompt_btn(&x, y, 0, ok);
    if (back) prompt_btn(&x, y, 1, back);
    if (!pad_connected() && pad_style() != 0) text(FBW - SAFE - 70, y, "No pad!", C_BAD);
}
#endif

/* ---- result page ---- */
static void show_result(const char *title, const char *body, int good, Screen next) {
    ys_cpy(result_title, title, sizeof result_title);
    ys_cpy(result_body, body, sizeof result_body);
    result_good = good; result_next = next;
    go(S_RESULT);
    plat_sound(good ? 3 : 4);
}
static void scr_result(void) {
    chrome(result_good ? L("Well done", "やった！", 2) : L("Try again", "もう一度", 2), 0);
    panel(CX, CY, CW, CH - (HTPC ? 0 : 40));
    if (result_good) sakura(CX + 10, CY + 10, HTPC ? 2 : 3);
    wrap(CX + 10, CY + (HTPC ? 44 : 60), CW - 20, result_title, result_good ? C_GOOD : C_BAD, LH, 0, 2);
    wrap(CX + 10, CY + (HTPC ? 44 : 60) + 2 * LH, CW - 20, result_body, C_INK, LH, 0, 0);
    if (button(CX + CW / 2 - 70, FBY_BTN, 140, 28, L("Continue", "つぎへ", 2))) go(result_next);
#if HTPC
    footer("Continue", 0);
#endif
}

/* ---- class flow ---- */
static const char *const STEP_EN[] = {"Warm-up review", "New lesson", "Typing practice", "Practice drill", "Conversation", "Reading", "Listening", "Daily quiz", "Homework", "Immersion"};
static const char *const STEP_JA[] = {"ふくしゅう", "じゅぎょう", "タイピング", "れんしゅう", "かいわ", "どっかい", "ききとり", "テスト", "しゅくだい", "イマージョン"};

static void start_quiz(QuizMode m) {
    qm = m; qi = 0; qright = 0; qanswered = 0; qchoice = 0; q_idk = 0; input[0] = 0; q_said = -1;
    go(S_QUIZ);
}
static void next_step(void);
/* Words only: typing a kanji or grammar pattern alone doesn't teach much */
static int day_word_cards(const YgDay *d, short *out) {
    int i, n = 0;
    for (i = 0; i < d->nnew && n < TY_MAX; i++) if (YG_CARDS[d->cards[i]].type == 'w') { if (out) out[n] = d->cards[i]; n++; }
    return n;
}
static void run_step(void) {
    const YgDay *d = &YG_DAYS[P.day];
    switch (P.step) {
    case STEP_WARMUP:
        quiz_warmup(&Z, &P);
        if (Z.n == 0) { P.step++; run_step(); return; }
        start_quiz(QM_WARMUP); break;
    case STEP_TEACH: teach_card = -1; page = 0; note_skip = 0; note_more = 0; go(S_TEACH); break;
    case STEP_TYPING:
        ty_n = day_word_cards(d, ty_cards); ty_i = 0; ty_rep = 0; ty_bad = 0; input[0] = 0;
        if (!ty_n) { P.step++; run_step(); return; }
        go(S_TYPING); break;
    case STEP_DRILL:
        quiz_new(&Z);
        quiz_add_cards(&Z, d->cards, d->nnew > 20 ? 20 : d->nnew, day_romaji_mode(P.day), 1);
        quiz_shuffle(&Z);
        if (Z.n == 0) { P.step++; run_step(); return; }
        start_quiz(QM_DRILL); break;
    case STEP_CHAT:
        if (d->chat < 0) { P.step++; run_step(); return; }
        chat_id = d->chat; chat_turn = 0; chat_fail = 0; chat_show = 0; input[0] = 0; go(S_CHAT); break;
    case STEP_READ:
        if (d->read < 0) { P.step++; run_step(); return; }
        rd_id = d->read; rd_q = -1; rd_right = 0; rd_show_en = 0; rd_skip = 0; rd_pick = -1; go(S_READ); break;
    case STEP_LISTEN:
        /* Hearing needs a voice. Without one the class carries on, and Settings shows how to add it. */
        if (!plat_has_tts()) { P.step++; run_step(); return; }
        quiz_listen(&Z, P.day, HTPC ? 34 : 0); /* the TV's text is large, so it keeps to sentences that fit */
        if (!Z.n) { P.step++; run_step(); return; }
        start_quiz(QM_LISTEN); break;
    case STEP_QUIZ: quiz_day(&Z, &P, P.day); start_quiz(QM_DAILY); break;
    case STEP_HOMEWORK: task_kind = 0; go(S_TASK); break;
    case STEP_IMMERSE: task_kind = 1; go(S_TASK); break;
    default: {
        char b[160];
        profile_learn_day(&P, P.day, plat_today());
        profile_complete_day(&P, plat_today());
        save_profile();
        ys_fmt(b, sizeof b, "+100 XP. Streak: %d day(s). Your new cards are now in the flashcard deck.", P.streak, 0);
        show_result("Class complete!", b, 1, S_HOME);
    }
    }
}
static void next_step(void) { P.step++; save_profile(); run_step(); }

static void scr_home(void) {
    int today = plat_today(), due = profile_due(&P, today), gate = profile_exam_gate(&P);
    char b[128];
    profile_touch_day(&P, today);
#if HTPC
    {
        static const char *const items_en[] = {"Today's Class", "Flashcards", "Exams", "Quest", "Library", "Text Reader", "Options"};
        static const char *const items_ja[] = {"きょうのクラス", "カード", "しけん", "クエスト", "本だな", "テキスト", "せってい"};
        static const int from[] = {2, 3, 2, 3, 5, 3, 2};
        static const Screen dest[] = {S_CLASS, S_CARDS, S_EXAMS, S_QUEST, S_LIBRARY, S_TEXT, S_SETTINGS};
        static int sel; static int slide;
        int s = SAFE, i;
        gradient(0, 0, FBW, FBH, 0x1B0F40, 0x8A2A5A);
        /* low sun and ground */
        fill(0, 150, FBW, FBH - 150, 0x2A1036);
        for (i = 0; i < 8; i++) fill(0, 150 + i * i, FBW, 1, 0x4A2050);
        torii(FBW / 2, 96, 6, anim * 2);
        text2(s + 2, s, "YomiGo", C_ACC);
        ys_fmt(b, sizeof b, "XP %d  Streak %d", P.xp, P.streak);
        text(FBW - s - 2 - text_w(b), s + 6, b, C_INK);
        text(s + 2, s + 32, rank_name(profile_rank(&P), profile_stage(&P) >= 3 && !P.force_en), C_PINK);
        if (IN.pressed & (A_LEFT)) { sel = (sel + 6) % 7; slide = -40; plat_sound(0); }
        if (IN.pressed & (A_RIGHT)) { sel = (sel + 1) % 7; slide = 40; plat_sound(0); }
        slide = slide * 3 / 4;
        for (i = -2; i <= 2; i++) {
            int k = (sel + i + 7) % 7, x = FBW / 2 + i * 122 + slide - 58, y = 166 + (i < 0 ? -i : i) * 6, w = 116, h = 40 - (i < 0 ? -i : i) * 4;
            const char *lab = L(items_en[k], items_ja[k], from[k]);
            fill(x, y, w, h, i == 0 ? C_ACC : C_PANEL);
            frame(x, y, w, h, i == 0 ? 0xFFF0C0 : C_HI);
            text_c(x + w / 2, y + (h - 16) / 2, lab, i == 0 ? 0x1B1033 : C_INK);
        }
        if (due > 0 && sel == 1) { ys_fmt(b, sizeof b, "%d cards due", due, 0); text_c(FBW / 2, 148, b, C_INK); }
        if (sel == 0) { text_c(FBW / 2, 148, P.day < YG_NDAYS ? YG_DAYS[P.day].title : "Course complete!", C_INK); }
        if (IN.pressed & A_OK) { plat_sound(1); go(dest[sel]); }
        if (IN.pressed & A_BACK) { go(S_PROFILES); have_profile = 0; }
        footer("Select", "Profiles");
        return;
    }
#else
    chrome(L("Today's Class", "きょうのクラス", 2), 0);
    panel(CX, CY, CW, 148);
    if (P.day < YG_NDAYS) {
        const YgDay *d = &YG_DAYS[P.day];
        ys_fmt(b, sizeof b, "Class %d of %d", P.day + 1, YG_NDAYS);
        text(CX + 10, CY + 8, YG_STAGES[d->stage].name, C_ACC);
        text(CX + CW - 10 - text_w(b), CY + 8, b, C_DIM);
        text2(CX + 10, CY + 30, d->ja, C_INK);
        text(CX + 10, CY + 66, d->title, C_DIM);
        progress(CX + 10, CY + 90, CW - 20, 12, P.step, STEP_DONE, C_ACC);
        if (gate >= 0) {
            ys_fmt(b, sizeof b, "Pass this exam to continue (80%%)", 0, 0);
            text(CX + 10, CY + 106, YG_EXAMS[gate].name, C_BAD);
            if (button(CX + CW - 180, CY + 110, 170, 28, L("Take the exam", "しけんを受ける", 5))) { exam_id = gate; quiz_exam(&Z, exam_id); start_quiz(QM_EXAM); }
        } else if (P.done_today >= P.pace && P.last_day == today) {
            text(CX + 10, CY + 114, "Today's class is done. Rest, review, or immerse.", C_GOOD);
        } else if (button(CX + CW - 180, CY + 110, 170, 28, P.step ? L("Continue class", "つづける", 2) : L("Start class", "はじめる", 2))) go(S_CLASS);
    } else if (gate >= 0) {
        text2(CX + 10, CY + 30, YG_EXAMS[gate].name, C_INK);
        if (button(CX + CW - 180, CY + 110, 170, 28, L("Take the exam", "しけんを受ける", 5))) { exam_id = gate; quiz_exam(&Z, exam_id); start_quiz(QM_EXAM); }
    } else {
        text2(CX + 10, CY + 30, "N4 complete!", C_GOOD);
        wrap(CX + 10, CY + 70, CW - 20, "N3 and beyond arrive in a future update. Keep reviewing your cards, and read and listen to real Japanese every day.", C_INK, LH, 0, 3);
    }
    { int y = CY + 158;
      panel(CX, y, (CW - 8) / 2, 120);
      text(CX + 8, y + 6, L("Flashcards due", "カード", 3), C_ACC);
      ys_fmt(b, sizeof b, "%d", due, 0); text2(CX + 8, y + 28, b, C_INK);
      if (button(CX + 8, y + 80, (CW - 8) / 2 - 16, 28, L("Review now", "ふくしゅう", 2))) go(S_CARDS);
      panel(CX + (CW + 8) / 2, y, (CW - 8) / 2, 120);
      text(CX + (CW + 8) / 2 + 8, y + 6, L("Today's immersion", "イマージョン", 3), C_ACC);
      if (P.day < YG_NDAYS) wrap(CX + (CW + 8) / 2 + 8, y + 26, (CW - 8) / 2 - 16, YG_DAYS[P.day].imm, C_INK, LH - 2, 0, 4);
    }
    if (new_material > 0) {
        ys_fmt(b, sizeof b, "%d new file(s) in your Material folder. See the Library.", new_material, 0);
        text(CX, CY + CH - 16, b, C_ACC);
    }
#endif
}
static void scr_class(void) {
    const YgDay *d; int i, y, gate = profile_exam_gate(&P), today = plat_today(); char b[64];
    if (gate >= 0) {
        chrome(L("Exam required", "しけん", 2), 0);
        panel(CX, CY, CW, HTPC ? 90 : 150);
        text(CX + 8, CY + 8, YG_EXAMS[gate].name, C_ACC);
        wrap(CX + 8, CY + 8 + LH, CW - 16, "Pass this trial with 80% to open the next class. You can retake it as often as you need.", C_INK, LH, 0, 0);
        if (button(CX + CW / 2 - 80, FBY_BTN, 160, 28, L("Challenge", "ちょうせん", 2))) { exam_id = gate; quiz_exam(&Z, gate); start_quiz(QM_EXAM); return; }
        if (IN.pressed & A_BACK) go(S_HOME);
#if HTPC
        footer("Challenge", "Menu");
#endif
        return;
    }
    if (P.day >= YG_NDAYS) { go(S_HOME); return; }
    if (P.done_today >= P.pace && P.last_day == today) {
        chrome(L("Class is over", "おわり", 2), 0);
        panel(CX, CY, CW, HTPC ? 90 : 150);
        wrap(CX + 8, CY + 8, CW - 16, "That's today's class at your chosen pace. Review your flashcards, immerse, and come back tomorrow. You can change your pace in Settings.", C_INK, LH, 0, 0);
        if (button(CX + CW / 2 - 70, FBY_BTN, 140, 28, L("OK", "OK", 0)) || (IN.pressed & A_BACK)) go(S_HOME);
#if HTPC
        footer("OK", 0);
#endif
        return;
    }
    d = &YG_DAYS[P.day];
    chrome(d->ja, YG_STAGES[d->stage].ja);
    if (HTPC) text(CX, CY, d->title, C_INK); else { text(CX, CY, d->title, C_DIM); ys_fmt(b, sizeof b, "Class %d", P.day + 1, 0); text(CX + CW - text_w(b), CY, b, C_DIM); }
    y = CY + LH + 2;
    for (i = 0; i < STEP_DONE; i++) {
        unsigned col = i < P.step ? C_GOOD : i == P.step ? (HTPC ? C_ACC : C_ACC) : C_DIM;
        const char *mark = i < P.step ? "\xE2\x97\x8B" : i == P.step ? "\xE2\x86\x92" : "\xE3\x83\xBB";
        if (i == STEP_CHAT && d->chat < 0) { continue; }
        if (i == STEP_TYPING && !day_word_cards(d, 0)) { continue; }
        if (i == STEP_READ && d->read < 0) { continue; }
        if (i == STEP_LISTEN && !plat_has_tts()) { continue; }
        if (HTPC && i < P.step - 2) { continue; } /* so the list always fits the TV */
        text(CX + 4, y, mark, col);
        text(CX + 24, y, L(STEP_EN[i], STEP_JA[i], 3), col);
        y += HTPC ? 14 : 24;
    }
    if (button(CX + CW / 2 - 70, FBY_BTN, 140, 28, L("Go", "はじめる", 2))) { run_step(); return; }
    if (!HTPC && button(CX, FBY_BTN, 90, 28, L("Later", "あとで", 2))) { go(S_HOME); return; }
    if (IN.pressed & A_BACK) go(S_HOME);
#if HTPC
    footer("Go", "Menu");
#endif
}
/* Draws strokes one at a time, the current one highlighted and started with a dot. Returns the width taken, or 0. */
static int stroke_box(int x, int y, const char *s) {
    const char *p = s; int cp = utf8_next(&p), i, size = HTPC ? 76 : 110;
    if (*p) return 0;
    for (i = 0; i < YG_ST_N; i++) if (YG_ST_CP[i] == cp) {
        int a = YG_ST_OFF[i], b = YG_ST_OFF[i + 1], k, n = 0, cur, stroke = 0, has_last = 0, lx = 0, ly = 0;
        unsigned paper = HTPC ? 0xF4EEDC : 0xFFFFFF;
        for (k = a; k < b; k++) if (YG_ST_PTS[k] == 255) n++; else k++;
        cur = (anim / 16) % (n + 3);
        x -= size;
        fill(x, y, size, size, paper);
        frame(x, y, size, size, C_ACC);
        fill(x + size / 2, y + 1, 1, size - 2, 0xE0D8D0); fill(x + 1, y + size / 2, size - 2, 1, 0xE0D8D0);
        for (k = a; k < b; ) {
            if (YG_ST_PTS[k] == 255) { stroke++; has_last = 0; k++; continue; }
            { int px = x + 2 + YG_ST_PTS[k] * (size - 4) / 109, py = y + 2 + YG_ST_PTS[k + 1] * (size - 4) / 109;
              if (stroke <= cur) {
                  unsigned col = stroke == cur ? 0xD02050 : 0x201018;
                  if (has_last) line(lx, ly, px, py, col, HTPC ? 2 : 3);
                  else if (stroke == cur) fill(px - 3, py - 3, 7, 7, 0xD02050);
              }
              lx = px; ly = py; has_last = 1; k += 2; }
        }
        return size + 8;
    }
    return 0;
}
static void scr_teach(void) {
    const YgDay *d = &YG_DAYS[P.day];
    int total = d->nnotes + d->nnew, rom = day_romaji_mode(P.day);
    chrome(d->ja, 0);
    { char b[32]; ys_fmt(b, sizeof b, "%d/%d", page + 1, total); text(CX + CW - text_w(b), CY, b, C_DIM); }
    if (page < d->nnotes) {
        text(CX, CY, L("Sensei's notes", "せんせいのノート", 3), C_ACC);
        panel(CX, CY + 20, CW, FBY_BTN - CY - 24);
        /* A long note scrolls in steps, so nothing runs out of its panel */
        { int per = (FBY_BTN - CY - 40) / LH, lines = wrap(CX + 8, CY + 28, CW - 16, d->notes[page], C_INK, LH, note_skip, per);
          note_more = note_skip + per < lines; }
    } else {
        const YgCard *c = &YG_CARDS[d->cards[page - d->nnotes]];
        const char *big = (rom && c->type == 'w') ? c->romaji : c->front;
        copy_src = c->front;
        int cy = CY + 20;
        panel(CX, cy, CW, FBY_BTN - cy - 4);
        big_text(CX + 10, cy + 6, big, C_ACC, CW - 20);
        cy += 42;
        if (c->type == 'g') {
            /* The TV has room for five tight lines, so a long explanation gives up the translation */
            int lh = HTPC ? 15 : LH, nl = wrap(CX + 10, cy + lh, CW - 20, c->note, C_DIM, lh, 0, HTPC ? 3 : 0);
            text(CX + 10, cy, c->en, C_INK);
            if (HTPC && nl > 3) nl = 3;
            cy += lh * (nl + 1);
            { static char s[128]; int k = 0; const char *p = c->sentence;
              while (*p && k < 120) { if (!ys_ncmp(p, "＿", 3)) { ys_cpy(s + k, c->reading, 120 - k); k += ys_len(s + k); p += 3; } else s[k++] = *p++; }
              s[k] = 0; text(CX + 10, cy, s, C_ACC); if (!HTPC || nl < 3) wrap(CX + 10, cy + lh, CW - 20, c->trans, C_DIM, lh, 0, 1); }
        } else {
            int room = (c->type == 'h' || c->type == 'k' || c->type == 'j') ? stroke_box(CX + CW - 6, cy - 38, c->front) : 0;
            if (c->type != 'h' && c->type != 'k') {
                if (rom) { text(CX + 10, cy, c->reading, C_DIM); }
                else if (ys_cmp(c->front, c->reading)) { text(CX + 10, cy, c->reading, C_INK); }
                if (!rom && profile_stage(&P) < 4) text(CX + 10 + text_w(c->reading) + 16, cy, c->romaji, C_DIM);
                cy += LH;
            }
            wrap(CX + 10, cy, CW - 20 - room, c->en, C_INK, LH, 0, 2);
            cy += LH;
            if (*c->note) wrap(CX + 10, cy + 2, CW - 20 - room, c->note, C_DIM, LH, 0, HTPC ? 4 : 0);
        }
        if (plat_has_tts() && (IN.pressed & A_Y)) plat_speak(c->reading);
    }
    if (button(CX + CW - 120, FBY_BTN, 120, 28, page + 1 < total ? L("Next", "つぎ", 2) : L("Practice!", "れんしゅう", 3))) {
        if (page < d->nnotes && note_more) note_skip += (FBY_BTN - CY - 40) / LH;
        else if (page + 1 < total) { page++; note_skip = 0; note_more = 0; }
        else next_step();
    } else if (button(CX, FBY_BTN, 90, 28, L("Back", "もどる", 2)) || (IN.pressed & A_BACK)) {
        if (note_skip > 0) note_skip = 0;
        else if (page > 0) { page--; note_more = 0; }
    }
#if HTPC
    footer("Next", "Back");
#endif
}

/* ---- quiz runner ---- */
static void quiz_finished(void) {
    int pct = Z.n ? qright * 100 / Z.n : 0;
    char b[180];
    switch (qm) {
    case QM_WARMUP:
        next_step(); break;
    case QM_DRILL:
        P.xp += qright; next_step(); break;
    case QM_DAILY:
        if (pct >= 80) {
            P.xp += qright * 2;
            if (pct == 100) P.perfects++;
            ach_check(&P);
            next_step();
        } else {
            ys_fmt(b, sizeof b, "You scored %d%%. You need 80%% to continue. Look at your notes and try again.", pct, 0);
            P.step = STEP_TEACH; save_profile();
            show_result("Not yet!", b, 0, S_CLASS);
        }
        break;
    case QM_HW:
        P.homeworks++; P.xp += 30; ach_check(&P); next_step(); break;
    case QM_REVIEW:
        save_profile();
        ys_fmt(b, sizeof b, "%d of %d correct. Cards you missed will come back soon.", qright, Z.n);
        show_result("Review done", b, qright * 2 >= Z.n, S_HOME);
        break;
    case QM_EXAM: {
        int pass = profile_exam_pass(&P, exam_id, pct, plat_today());
        save_profile();
        ys_fmt(b, sizeof b, pass ? "You scored %d%%. The way forward is open." : "You scored %d%%. You need 80%%. Review your cards and challenge it again.", pct, 0);
        show_result(YG_EXAMS[exam_id].name, b, pass, S_HOME);
        break; }
    case QM_LISTEN:
        P.xp += qright; P.listens++; ach_check(&P);
        sh_n = shadow_lines(P.day, sh_lines, 4); sh_i = 0; sh_said = -1;
        if (sh_n) go(S_SHADOW); else next_step();
        break;
    case QM_PRACTICE:
        /* Extra practice is free to repeat, so it earns no XP */
        ys_fmt(b, sizeof b, "%d of %d correct.", qright, Z.n);
        show_result("Practice done", b, qright * 2 >= Z.n, S_GENKI);
        break;
    case QM_PLACE: {
        /* Sections passed: romaji, hiragana, katakana, words, grammar, kanji. Learning always starts at romaji. */
        static const signed char map[7] = {0, 2, 3, 4, 4, 5, 6};
        if (qright >= 4 && place_section < 5) {
            place_passed = place_section + 1; place_section++;
            placement_section(&Z, place_section);
            start_quiz(QM_PLACE);
        } else {
            if (qright >= 4) place_passed = 6;
            profile_place(&P, map[place_passed], plat_today());
            save_profile();
            go(S_PLACE_RESULT);
        }
        break; }
    }
}
static void answer_given(int ok) {
    qanswered = 1; qok = ok;
    if (!ok) plat_sound(4);
    if (ok) { qright++; plat_sound(2); }
    if (qm == QM_REVIEW) {
        int id = Z.q[qi].card;
        CardState *c = id >= YG_NCARDS ? &P.my[id - YG_NCARDS].st : &P.cards[id];
        srs_grade(c, ok ? GRADE_GOOD : GRADE_AGAIN, plat_today());
        P.reviews++; if (ok) P.xp += 2;
        ach_check(&P);
        save_profile();
    }
    /* Missed drill cards come straight back: repetition is the point */
    if (!ok && qm == QM_DRILL && Z.n < YG_MAXQ) { Z.q[Z.n] = Z.q[qi]; Z.n++; }
}
static void do_paste(void) {
    static char clip[512]; const char *s = clip; int n = ys_len(input), c;
    plat_clip_get(clip, sizeof clip);
    /* whole characters only, and no line breaks or tabs from the source */
    while ((c = utf8_next(&s)) != 0 && n < 120) if (c >= 32 && c != 0x7F) n += utf8_put(c, input + n);
    input[n] = 0;
    plat_sound(0);
}
static void text_entry(void) {
    int n = ys_len(input), i;
    for (i = 0; IN.text[i] && n < 120; i++) input[n++] = IN.text[i];
    input[n] = 0;
    while (IN.backspace-- > 0 && n > 0) { n--; while (n > 0 && (input[n] & 0xC0) == 0x80) n--; input[n] = 0; }
    if (IN.paste) do_paste();
}
/* The answer box: bright frame, a label when empty, and a blinking cursor show where typing goes */
static void input_box(int x, int y, int w, int active) {
    const char *s = input, *comp = active ? plat_ime_comp() : ""; int tw, cw;
    unsigned bg = HTPC ? C_LO : 0xFFFFFF;
    fill(x, y, w, 24, bg);
    frame(x, y, w, 24, active ? C_ACC : C_DIM);
    if (active) frame(x + 1, y + 1, w - 2, 22, C_ACC);
    cw = text_w(comp);
    /* a long answer scrolls so its end stays visible */
    while (*s && text_w(s) + cw > w - 18) utf8_next(&s);
    tw = text(x + 6, y + 4, s, C_INK);
    if (active) {
        field_on = 1; field_x = x + 6 + tw; field_y = y;
        if (*comp) { /* unfinished Japanese, underlined as Windows does */
            text(x + 6 + tw, y + 4, comp, C_ACC);
            fill(x + 6 + tw, y + 20, cw, 2, C_ACC);
            tw += cw;
        }
        if (!*s && !*comp) text(x + 12, y + 4, "Type here", C_DIM);
        if ((anim / 8) & 1 || !anim) fill(x + 6 + tw + 1, y + 4, 2, 16, C_ACC);
    }
}
#if !HTPC
static void clip_buttons(int x, int y) {
    text(x, y + 2, "Ctrl+C  Ctrl+V", C_DIM);
    if (button(x + 122, y, 52, 20, "Copy")) IN.copy = 1;
    if (button(x + 178, y, 52, 20, "Paste")) do_paste();
}
#endif
/* On-screen keyboard for the TV. Romaji typed here becomes kana in the answer check. */
#if HTPC
static void osk(int y) {
    static const char *const R[] = {"qwertyuiop", "asdfghjkl'", "zxcvbnm-  "};
    int i;
    text_entry(); /* a real keyboard works here too */
    if (IN.pressed & A_LEFT) kb_x = (kb_x + 9) % 10;
    if (IN.pressed & A_RIGHT) kb_x = (kb_x + 1) % 10;
    if (IN.pressed & A_UP) kb_y = (kb_y + 2) % 3;
    if (IN.pressed & A_DOWN) kb_y = (kb_y + 1) % 3;
    for (i = 0; i < 3; i++) { int j; for (j = 0; j < 10; j++) {
        char t[2]; int x = CX + 20 + j * 26, yy = y + i * 19, f = (i == kb_y && j == kb_x);
        t[0] = R[i][j]; t[1] = 0;
        fill(x, yy, 24, 17, f ? C_ACC : C_PANEL); frame(x, yy, 24, 17, C_HI);
        if (t[0] != ' ') text_c(x + 12, yy, t, f ? 0x1B1033 : C_INK);
    } }
    if (IN.pressed & A_OK) { char c = R[kb_y][kb_x]; int n = ys_len(input); if (c != ' ' && n < 120) { input[n] = c; input[n + 1] = 0; plat_sound(0); } }
    if (IN.pressed & A_BACK) { int n = ys_len(input); if (n) input[n - 1] = 0; }
}
#endif
static void scr_quiz(void) {
    Question *q = &Z.q[qi];
    char b[64];
    int hearing = *q->say != 0, hidden = hearing && !qanswered;
    static const char *const hdr[] = {"Warm-up", "Practice", "Daily quiz", "Homework check", "Flashcards", "Exam", "Entrance exam", "Genki practice", "Listening"};
    if (!hidden) copy_src = q->prompt;
    if (hearing && q_said != qi) { plat_speak(q->say); q_said = qi; }
    if (hearing && (IN.pressed & A_Y)) plat_speak(q->say);
    chrome(hdr[qm], 0);
    ys_fmt(b, sizeof b, "%d / %d", qi + 1, Z.n);
    text(CX + CW - text_w(b), CY, b, C_DIM);
    if (qm == QM_EXAM) {
        /* The exam is a duel: every right answer hits the boss, who falls at 80% */
        int need = (Z.n * 4 + 4) / 5;
        text(CX, CY, YG_EXAMS[exam_id].name, C_ACC);
        progress(CX, CY + (HTPC ? 16 : 18), CW, HTPC ? 8 : 10, need - qright, need, C_BAD);
    } else {
        progress(CX, CY + (HTPC ? 16 : 18), CW, HTPC ? 8 : 10, qi, Z.n, C_ACC);
    }
    /* The TV skips the gap above the question, so its on-screen keyboard fits underneath */
    { int y = CY + (HTPC ? 28 : 32);
      text(CX, y, q->tag, C_DIM); y += HTPC ? 15 : LH;
      { const char *shown = hidden ? "Listen..." : q->prompt; int tight = 0, c_;
        if (HTPC && q->kind == Q_CHOICE) for (c_ = 0; c_ < 4; c_++) if (text_w(q->choices[c_]) > (CW - 8) / 2 - 8) tight = 1;
        if (tight || 2 * text_w(shown) > CW) y += LH * wrap(CX, y, CW, shown, hidden ? C_ACC : C_INK, LH, 0, tight ? 1 : 2);
        else { text2(CX + (CW - 2 * text_w(shown)) / 2, y, shown, hidden ? C_ACC : C_INK); y += HTPC ? 32 : 34; } }
      if (*q->sub) { wrap(CX, y, CW, q->sub, C_DIM, LH, 0, 1); y += LH; }
      if (q->kind == Q_CHOICE) {
        int bw = (CW - 8) / 2, bh = HTPC ? 26 : 34, i, one = 0;
        for (i = 0; i < 4; i++) if (text_w(q->choices[i]) > bw - 8) one = 1;
        if (one) { bw = CW; if (HTPC) bh = 18; }
        for (i = 0; i < 4; i++) {
            int x = one ? CX : CX + (i % 2) * (bw + 8), yy = one ? y + 2 + i * (bh + (HTPC ? 1 : 4)) : y + 4 + (i / 2) * (bh + 6);
            if (qanswered) {
                unsigned f = i == q->answer ? C_GOOD : (i == qchoice ? C_BAD : (HTPC ? C_PANEL : 0xEFD9D0));
                fill(x, yy, bw, bh, f); frame(x, yy, bw, bh, C_LO);
                wrap(x + 4, yy + (bh - 16) / 2, bw - 8, q->choices[i], (i == q->answer || i == qchoice) ? (HTPC ? 0x1B1033 : 0xFFFFFF) : C_INK, LH, 0, 1);
            } else if (button(x, yy, bw, bh, q->choices[i]) || IN.num == i + 1) {
                qchoice = i; answer_given(i == q->answer); IN.pressed &= ~A_OK;
            }
        }
      } else {
        char pre[128];
        input_box(CX, y + 2, CW, !qanswered);
        norm_kana(input, pre, sizeof pre);
        text(CX + 6, y + 27, pre, C_ACC);
        if (qanswered) {
            int tw = text(CX, y + 50, q->want, qok ? C_GOOD : C_BAD);
            if (*q->want2) text(CX + tw + 16, y + 50, q->want2, C_DIM);
        } else {
#if HTPC
            osk(FBH - SAFE - 75);
            if (IN.pressed & A_START) { answer_given(answer_ok(input, q->want, q->want2)); IN.pressed &= ~A_OK; }
#else
            text_entry();
            if (button(CX + CW - 90, y + 46, 90, 28, L("Check", "チェック", 3))) { answer_given(answer_ok(input, q->want, q->want2)); IN.pressed &= ~A_OK; }
            else clip_buttons(CX, y + 50);
#endif
        }
      }
      if (qanswered) {
        const char *why = q->card >= YG_NCARDS ? P.my[q->card - YG_NCARDS].sent : YG_CARDS[q->card].note;
        if (!qok && *why && !HTPC) wrap(CX, FBY_BTN - 40, CW, why, C_DIM, 18, 0, 2);
        text(CX, FBY_BTN + 6, qok ? L("Correct!", "正解！", 5) : q_idk ? (HTPC ? "That's okay." : "That's okay. Here's the answer.") : L("Not quite.", "ちがいます", 3), qok ? C_GOOD : q_idk ? C_DIM : C_BAD);
        if (button(CX + CW - 120, FBY_BTN, 120, 28, L("Next", "つぎ", 2))) {
            qi++; qanswered = 0; q_idk = 0; input[0] = 0; ui_reset_focus();
            if (qi >= Z.n) quiz_finished();
            return;
        }
      } else {
        /* Not knowing is a normal answer. It counts as a miss, and shows you the right one. */
#if HTPC
        if (IN.pressed & A_X) { qchoice = -1; q_idk = 1; answer_given(0); IN.pressed &= ~A_OK; }
#else
        if (button(CX, FBY_BTN, 130, 28, L("I don't know", "わかりません", 3))) { qchoice = -1; q_idk = 1; answer_given(0); IN.pressed &= ~A_OK; }
#endif
      }
#if !HTPC
      if (hearing && button(CX + 136, FBY_BTN, 110, 28, L("Hear again", "もう一度", 5))) plat_speak(q->say);
#endif
    }
#if HTPC
    if (qanswered) { int x = SAFE + 2; prompt_btn(&x, FBH - SAFE - 17, 0, "OK"); if (hearing) prompt_btn(&x, FBH - SAFE - 17, 3, "Again"); }
    else if (q->kind == Q_TYPE) { int x = SAFE + 2; if (!hearing) prompt_btn(&x, FBH - SAFE - 17, 0, "Key"); prompt_btn(&x, FBH - SAFE - 17, 1, "Del"); prompt_btn(&x, FBH - SAFE - 17, 6, "OK"); prompt_btn(&x, FBH - SAFE - 17, 2, hearing ? "?" : "Don't know"); if (hearing) prompt_btn(&x, FBH - SAFE - 17, 3, "Again"); }
    else { int x = SAFE + 2; prompt_btn(&x, FBH - SAFE - 17, 0, "OK"); prompt_btn(&x, FBH - SAFE - 17, 2, "Don't know"); if (hearing) prompt_btn(&x, FBH - SAFE - 17, 3, "Again"); }
#endif
    /* Leaving a lesson quiz is fine: the class resumes at this step */
    if (qm != QM_EXAM && qm != QM_PLACE && ((!HTPC && (IN.pressed & A_BACK)) || (HTPC && (IN.pressed & A_START) && q->kind != Q_TYPE))) go(S_HOME);
}

/* ---- conversation ---- */
static void scr_chat(void) {
    const YgChat *c = &YG_CHATS[chat_id];
    const YgTurn *t;
    int y = CY, rom = day_romaji_mode(P.day), stage = profile_stage(&P);
    if (chat_turn >= c->nturns) {
        P.chats++; P.xp += 30; ach_check(&P);
        next_step();
        return;
    }
    t = &c->turns[chat_turn];
    copy_src = t->say;
    chrome(c->ja, c->title);
    panel(CX, y, CW, HTPC ? 62 : 92);
    text(CX + 6, y + 4, c->who, C_ACC);
    if (*t->say) {
        wrap(CX + 6, y + 22, CW - 12, rom ? t->romaji : t->say, C_INK, LH, 0, 2);
        if (rom || stage < 4) wrap(CX + 6, y + 22 + LH, CW - 12, rom ? t->say : t->romaji, C_DIM, LH, 0, 1);
        if (stage < 5 || chat_fail) wrap(CX + 6, y + 22 + 2 * LH, CW - 12, t->en, C_DIM, LH, 0, 1);
    } else text(CX + 6, y + 22, "(You speak first.)", C_DIM);
    y += HTPC ? 66 : 100;
    input_box(CX, y, CW, 1);
    { char pre[128]; norm_kana(input, pre, sizeof pre); text(CX + 6, y + 26, pre, C_ACC); }
    y += 44;
    {
        /* From the third miss, or on "I don't know", show a reply to copy, in letters if kana isn't taught yet */
        char s[128];
        ys_cpy(s, "Try: ", sizeof s);
        ys_cat(s, rom ? t->want_ro : t->want[0], sizeof s);
#if HTPC
        if (chat_fail) {
            fill(CX + 1, CY + 22, CW - 2, 39, C_PANEL);
            if (chat_fail >= 3) text(CX + 6, CY + 30, s, C_ACC);
            else wrap(CX + 6, CY + 22, CW - 12, t->hint, C_INK, LH, 0, 2);
        }
#else
        if (chat_fail) {
            text(CX, y, L("Here's a hint:", "ヒント:", 3), C_DIM);
            wrap(CX, y + LH, CW, t->hint, C_INK, LH, 0, 2);
            if (chat_fail >= 3) text(CX, y + 3 * LH, s, C_ACC);
        }
#endif
    }
#if HTPC
    osk(FBH - SAFE - 80);
    if (IN.pressed & A_X) { chat_fail = 3; input[0] = 0; }
    if (IN.pressed & A_START) {
        if (chat_ok(input, t)) { chat_turn++; chat_fail = 0; input[0] = 0; plat_sound(2); }
        else { chat_fail++; plat_sound(4); }
    }
    { int x = SAFE + 2; prompt_btn(&x, FBH - SAFE - 17, 0, "Key"); prompt_btn(&x, FBH - SAFE - 17, 1, "Del"); prompt_btn(&x, FBH - SAFE - 17, 6, "Say"); prompt_btn(&x, FBH - SAFE - 17, 2, "Don't know"); }
#else
    text_entry();
    if (button(CX + CW - 120, FBY_BTN, 120, 28, L("Say it", "言う", 5))) {
        if (chat_ok(input, t)) { chat_turn++; chat_fail = 0; input[0] = 0; plat_sound(2); }
        else { chat_fail++; plat_sound(4); }
    }
    if (button(CX + 124, FBY_BTN, 130, 28, L("I don't know", "わかりません", 3))) { chat_fail = 3; input[0] = 0; }
    if (plat_has_tts() && *t->say && button(CX, FBY_BTN, 120, 28, L("Hear it", "聞く", 5))) plat_speak(t->say);
    clip_buttons(CX + CW - 230, CY + 124);
#endif
}

/* ---- typing practice: every new word, several times ---- */
static void scr_typing(void) {
    const YgCard *c = &YG_CARDS[ty_cards[ty_i]];
    int rom = day_romaji_mode(P.day), y = CY, submit;
    char b[160], pre[128];
    copy_src = c->front;
    chrome(L("Typing practice", "タイピング", 3), 0);
    ys_fmt(b, sizeof b, "Word %d of %d", ty_i + 1, ty_n);
    text(CX, y, b, C_DIM);
    { int i; for (i = 0; i < TY_REPS; i++) fill(CX + CW - (TY_REPS - i) * 18, y + 2, 14, 12, i < ty_rep ? C_GOOD : (HTPC ? C_LO : 0xD9C8BC)); }
    y += LH;
    big_text(CX, y, rom ? c->romaji : c->front, C_ACC, CW);
    y += HTPC ? 33 : 36;
    { /* one line: whatever helps most, in order, while it still fits */
      int x = CX, right = CX + CW;
      const char *parts[3]; int i;
      parts[0] = (rom && HTPC) ? "" : ys_cmp(c->front, c->reading) || rom ? c->reading : "";
      parts[1] = (!rom && profile_stage(&P) < 4) ? c->romaji : "";
      parts[2] = c->en;
      for (i = 0; i < 3; i++) if (*parts[i] && x + text_w(parts[i]) <= right) x += text(x, y, parts[i], i == 2 ? C_INK : C_DIM) + 10; }
    y += LH;
    input_box(CX, y, CW, 1);
    y += 26;
    norm_kana(input, pre, sizeof pre);
    if (ty_bad && !input[0]) { ys_cpy(b, "Not quite. Type ", sizeof b); ys_cat(b, c->romaji, sizeof b); ys_cat(b, " or ", sizeof b); ys_cat(b, c->reading, sizeof b); text(CX + 6, y, b, C_BAD); }
    else text(CX + 6, y, pre, C_ACC);
#if HTPC
    osk(FBH - SAFE - 80);
    submit = (IN.pressed & A_START) != 0;
    { int x = SAFE + 2; prompt_btn(&x, FBH - SAFE - 17, 0, "Key"); prompt_btn(&x, FBH - SAFE - 17, 1, "Del"); prompt_btn(&x, FBH - SAFE - 17, 6, "Enter"); }
    if (!submit && !input[0] && (IN.pressed & A_BACK)) { go(S_CLASS); return; }
#else
    text_entry();
    submit = button(CX + CW - 120, FBY_BTN, 120, 28, "Enter");
    if (button(CX, FBY_BTN, 90, 28, L("Later", "あとで", 2))) { save_profile(); go(S_CLASS); return; }
    clip_buttons(CX + CW - 230, FBY_BTN - 26);
#endif
    if (submit && input[0]) {
        if (answer_ok(input, c->reading, c->romaji)) {
            plat_sound(2); ty_bad = 0; input[0] = 0; ty_rep++;
            if (ty_rep >= TY_REPS) {
                ty_rep = 0; ty_i++; P.xp++;
                if (ty_i >= ty_n) { next_step(); return; }
            }
        } else { plat_sound(4); ty_bad = 1; input[0] = 0; }
    }
}

/* ---- shadowing: hear a line, then say it aloud ---- */
static void scr_shadow(void) {
    const ShadowLine *l = &sh_lines[sh_i];
    int y = CY, stage = profile_stage(&P);
    char b[40];
    copy_src = l->jp;
    if (sh_said != sh_i) { plat_speak(l->jp); sh_said = sh_i; }
    chrome(L("Shadowing", "シャドーイング", 3), 0);
    ys_fmt(b, sizeof b, "Line %d of %d", sh_i + 1, sh_n);
    text(CX, y, b, C_DIM); y += LH;
    y += LH * wrap(CX, y, CW, HTPC ? "Listen, then say it out loud." : "Listen, then say it out loud, copying the rhythm and tone. Do it as many times as you like.", C_DIM, LH, 0, 2) + 4;
    panel(CX, y, CW, HTPC ? 62 : 100);
    y += 4;
    y += LH * wrap(CX + 8, y, CW - 16, l->jp, C_ACC, LH, 0, 2);
    if (*l->ro && stage < 4) y += LH * wrap(CX + 8, y, CW - 16, l->ro, C_DIM, LH, 0, 1);
    wrap(CX + 8, y, CW - 16, l->en, C_INK, LH, 0, 1);
    /* there's no way to check this here, so it earns no points: it's between you and your mouth */
    if (button(CX + CW - 120, FBY_BTN, 120, 28, "I said it")) { if (++sh_i >= sh_n) { next_step(); return; } ui_reset_focus(); return; }
    if (button(CX, FBY_BTN, 110, 28, L("Hear again", "もう一度", 5)) || (IN.pressed & A_Y)) plat_speak(l->jp);
#if HTPC
    { int x = SAFE + 2; prompt_btn(&x, FBH - SAFE - 17, 0, "OK"); prompt_btn(&x, FBH - SAFE - 17, 3, "Again"); }
#endif
}

/* ---- reading: a short passage, then questions about it ---- */
static void rd_shuffle(void) {
    int i;
    for (i = 0; i < 3; i++) rd_order[i] = i;
    for (i = 2; i > 0; i--) { int j = yg_rand() % (i + 1), t = rd_order[i]; rd_order[i] = rd_order[j]; rd_order[j] = t; }
    rd_pick = -1;
}
static void scr_read(void) {
    const YgRead *r = &YG_READ[rd_id];
    int y = CY, lines, per;
    copy_src = r->jp;
    chrome(r->ja, r->title);
    if (rd_q < 0) {
        const char *body = rd_show_en ? r->en : r->jp;
        per = (FBY_BTN - CY - 26) / LH;
        panel(CX, y, CW, FBY_BTN - y - 4);
        lines = wrap(CX + 8, y + 6, CW - 16, body, rd_show_en ? C_DIM : C_INK, LH, rd_skip, per);
        if (button(CX + CW - 120, FBY_BTN, 120, 28, rd_skip + per < lines ? L("More", "つぎ", 2) : L("Questions", "しつもん", 3))) {
            if (rd_skip + per < lines) rd_skip += per;
            else { rd_q = 0; rd_shuffle(); }
            IN.pressed &= ~A_OK;
            return;
        }
        if (button(CX, FBY_BTN, 110, 28, rd_show_en ? "Japanese" : "English")) { rd_show_en = !rd_show_en; rd_skip = 0; }
        if (plat_has_tts() && (IN.pressed & A_Y)) plat_speak(r->jp);
#if HTPC
        footer("OK", 0);
#endif
        return;
    }
    {
        const char *const *q = r->q + rd_q * 4;
        int bh = HTPC ? 24 : 34, i;
        char b[32];
        ys_fmt(b, sizeof b, "Question %d of %d", rd_q + 1, r->nq);
        text(CX, y, b, C_DIM); y += LH;
        y += LH * wrap(CX, y, CW, q[0], C_INK, LH, 0, 2) + 4;
        for (i = 0; i < 3; i++) {
            int idx = rd_order[i], yy = y + i * (bh + 4);
            if (rd_pick >= 0) {
                unsigned f = idx == 0 ? C_GOOD : (idx == rd_pick ? C_BAD : (HTPC ? C_PANEL : 0xEFD9D0));
                fill(CX, yy, CW, bh, f); frame(CX, yy, CW, bh, C_LO);
                wrap(CX + 4, yy + (bh - 16) / 2, CW - 8, q[1 + idx], (idx == 0 || idx == rd_pick) ? (HTPC ? 0x1B1033 : 0xFFFFFF) : C_INK, LH, 0, 1);
            } else if (button(CX, yy, CW, bh, q[1 + idx]) || IN.num == i + 1) {
                rd_pick = idx; if (idx == 0) { rd_right++; plat_sound(2); } else plat_sound(4);
                IN.pressed &= ~A_OK;
            }
        }
        if (rd_pick >= 0 && button(CX + CW - 120, FBY_BTN, 120, 28, L("Next", "つぎ", 2))) {
            ui_reset_focus();
            if (++rd_q >= r->nq) { P.reads++; P.xp += 5 * rd_right; ach_check(&P); next_step(); return; }
            rd_shuffle();
        }
    }
#if HTPC
    footer("OK", 0);
#endif
}

/* ---- homework and immersion ---- */
static void scr_task(void) {
    const YgDay *d = &YG_DAYS[P.day];
    chrome(task_kind ? L("Immersion", "イマージョン", 3) : L("Homework", "しゅくだい", 3), 0);
    panel(CX, CY, CW, HTPC ? 110 : 180);
    wrap(CX + 8, CY + 8, CW - 16, task_kind ? d->imm : d->hw, C_INK, LH, 0, 0);
    if (button(CX + CW - 170, FBY_BTN, 170, 28, task_kind ? L("I'll do it today", "やります！", 2) : L("I did it!", "できた！", 2))) {
        if (task_kind) next_step();
        else {
            /* A short check, so homework is more than a tick box */
            int i;
            quiz_new(&Z);
            for (i = 0; i < 3 && d->nnew; i++) q_make(&Z.q[Z.n++], d->cards[yg_rand() % d->nnew], 0, day_romaji_mode(P.day));
            if (Z.n) start_quiz(QM_HW);
            else { P.homeworks++; P.xp += 30; ach_check(&P); next_step(); }
        }
        return;
    }
    if (task_kind && *d->mat) {
        int i, n = plat_scan_material();
        for (i = 0; i < n; i++) if (ys_contains_ci(plat_material(i), d->mat)) {
            text(CX + 8, CY + (HTPC ? 76 : 100), L("From your Material folder:", "教材:", 5), C_ACC);
            wrap(CX + 8, CY + (HTPC ? 92 : 120), CW - 16, plat_material(i), C_DIM, LH, 0, 2);
            if (button(CX, FBY_BTN, 110, 28, L("Open it", "ひらく", 2))) plat_open(plat_material_full(i));
            break;
        }
    }
#if HTPC
    footer("Confirm", 0);
#endif
}

/* ---- flashcards ---- */
static void scr_cards(void) {
    int due = profile_due(&P, plat_today());
    char b[64];
    chrome(L("Flashcards", "カード", 3), 0);
    panel(CX, CY, CW, HTPC ? 100 : 160);
    ys_fmt(b, sizeof b, due == 1 ? "%d card is due" : "%d cards are due", due, 0);
    text2(CX + 10, CY + 8, b, due ? C_ACC : C_GOOD);
    { int seen = P.nmy, weak = 0, i; for (i = 0; i < YG_NCARDS; i++) if (P.cards[i].seen) { seen++; if (P.cards[i].lapses) weak++; }
      for (i = 0; i < P.nmy; i++) if (P.my[i].st.lapses) weak++;
      ys_fmt(b, sizeof b, "Deck: %d cards. Struggling with: %d", seen, weak); text(CX + 10, CY + 46, b, C_INK); }
    wrap(CX + 10, CY + 66, CW - 20, "Right answers come back later, wrong ones tomorrow.", C_DIM, LH, 0, 2);
    { int bw = (CW - 8) / 3;
      if (due && button(CX, FBY_BTN, bw, 28, L("Review", "ふくしゅう", 2))) { quiz_review(&Z, &P, plat_today(), 30); start_quiz(QM_REVIEW); return; }
      if (button(CX + bw + 4, FBY_BTN, bw, 28, "My words")) { my_sel = 0; my_confirm = 0; go(S_MYWORDS); return; }
      if (P.genki && button(CX + 2 * (bw + 4), FBY_BTN, bw, 28, HTPC ? "Genki" : "Genki practice")) { go(S_GENKI); return; } }
#if HTPC
    if (IN.pressed & A_BACK) go(S_HOME);
    footer(due ? "Review" : 0, "Menu");
#endif
}

/* ---- text reader: text from games, the clipboard, or a screenshot; click a word to look it up ---- */
static void scr_text(void) {
    int y = CY, bw = HTPC ? (CW - 4) / 2 : (CW - 16) / 5, i, rows = HTPC ? 2 : 4, nres = HTPC ? 2 : 3, pages = (tx_n + rows - 1) / rows;
    tx_poll();
    if (pages > 1 && (IN.pressed & (A_L | A_R))) { tx_page = (tx_page + (IN.pressed & A_R ? 1 : pages - 1)) % pages; tx_sel = tx_page * rows; tx_pos = 0; tx_nres = 0; tx_len = 0; }
    chrome(L("Text Reader", "テキスト", 3), 0);
    if (button(CX, y, bw, 22, "Paste")) { static char clip[8192]; plat_clip_get(clip, sizeof clip); tx_msg = tx_add(clip) ? "" : "There's no Japanese text on the clipboard."; }
    if (button(CX + bw + 4, y, bw, 22, tx_watch ? "Watch: On" : "Watch: Off")) { tx_watch = !tx_watch; tx_seq = plat_clip_seq(); tx_msg = tx_watch ? "Japanese copied by any program now appears here." : ""; }
#if !HTPC
    if (button(CX + 2 * (bw + 4), y, bw, 22, tx_ocr ? "Reading..." : "Screenshot") && !tx_ocr) {
        if (!plat_has("YomiGoOCR.exe") && plat_can_update()) { plat_fetch_helpers(); tx_msg = "Downloading the screenshot reader. Try again in a minute."; }
        else if (!plat_clip_image("ocr_in.bmp")) tx_msg = "First take a screenshot with Win+Shift+S, then press this.";
        else if (!plat_ocr_start("ocr_in.bmp", "ocr_out.txt")) tx_msg = "Couldn't start the text reader. It needs YomiGoOCR.exe, beside YomiGo, and Windows 10 or later.";
        else { tx_ocr = 1; tx_msg = "Reading the screenshot..."; }
    }
    { static char lbl[32]; int get = !plat_has(YG_SPEECH_MODEL);
      if (tx_listen) { char b[8]; int n = plat_read("listen_pct.txt", b, 7), v = 0, k; for (k = 0; k < n && b[k] >= '0' && b[k] <= '9'; k++) v = v * 10 + b[k] - '0'; ys_fmt(lbl, sizeof lbl, "Working %d%%", v, 0); }
      if (button(CX + 3 * (bw + 4), y, bw, 22, tx_listen ? lbl : "Transcribe") && !tx_listen) {
        if (!plat_has("YomiGoListen.exe") && plat_can_update()) { plat_fetch_helpers(); tx_msg = "Downloading the transcriber. Try again in a minute."; }
        else if (get && !tx_ask) { tx_ask = 1; tx_msg = "Transcribing needs a 574 MB speech model, downloaded once from Hugging Face and checked before use. Press Transcribe again to get it."; }
        else if (!plat_listen_start(get)) tx_msg = "Couldn't start the transcriber. It needs YomiGoListen.exe, beside YomiGo, and 64-bit Windows 10 or later.";
        else { tx_listen = get ? 2 : 1; tx_msg = get ? "Downloading the speech model..." : "Pick a file. Long videos take a while: keep studying, and the lines show up here when it's done."; }
      } }
    if (button(CX + 4 * (bw + 4), y, bw, 22, tx_top ? "On top: On" : "On top: Off")) { tx_top = !tx_top; plat_topmost(tx_top); }
#endif
    y += 28;
    if (!tx_n) {
        wrap(CX, y, CW, *tx_msg ? tx_msg : (HTPC ? "Copy Japanese from any program, then press Paste, or turn on Watch." : "Copy Japanese from a game or web page, or take a screenshot with Win+Shift+S. Then press Paste or Screenshot. With Watch on, text copied by a text hooker such as Textractor shows up by itself. Transcribe writes down what's said in an audio or video file."), C_DIM, LH, 0, 0);
#if HTPC
        if (IN.pressed & A_BACK) go(S_HOME);
        footer("OK", "Menu");
#endif
        return;
    }
    for (i = tx_page * rows; i < tx_n && i < (tx_page + 1) * rows; i++) {
        int h = LH, hit = ui_hit(CX, y, CW, h);
        if (i == tx_sel) fill(CX, y, CW, h, HTPC ? C_PANEL : 0xF8D4DC);
        wrap(CX + 4, y + 1, CW - 8, tx_lines[i], i == tx_sel ? C_INK : C_DIM, LH, 0, 1);
        if (hit) { tx_sel = i; tx_pos = 0; tx_nres = 0; tx_len = 0; }
        y += h + 1;
    }
    y += 6;
    /* The chosen line, one character at a time, so any word can be picked */
    {
        const char *s = tx_lines[tx_sel], *p = s; int x = CX, c, lines = 0;
        copy_src = s;
        panel(CX, y - 2, CW, (HTPC ? 2 : 3) * LH + 4);
        while (*p && lines < (HTPC ? 2 : 3)) {
            const char *start = p; int w, at = (int)(p - s), hit, in_word;
            c = utf8_next(&p);
            { char one[8]; int k = utf8_put(c, one); one[k] = 0; w = text_w(one);
              if (x + w > CX + CW - 4) { x = CX; y += LH; lines++; if (lines >= (HTPC ? 2 : 3)) break; }
              hit = ui_hit(x, y, w, LH);
              in_word = tx_len && at >= tx_pos && (int)(start - s) < tx_pos + 3 * tx_len && at >= tx_pos;
              if (in_word) { const char *q = s + tx_pos; int k2; for (k2 = 0; k2 < tx_len && *q; k2++) utf8_next(&q); in_word = start < q; }
              if (in_word) fill(x, y, w, LH - 2, C_PINK);
              if (ui_focused()) fill(x, y + LH - 3, w, 2, C_ACC);
              text(x, y, one, C_INK);
              if (hit) { tx_pos = at; tx_nres = lookup_at(s, at, tx_res, 8, &tx_len); }
              x += w; }
        }
        y += LH + 8;
        if (!HTPC && lines < 2) y += (2 - lines) * LH;
    }
    if (!tx_nres) wrap(CX, y, CW, tx_len || !tx_pos ? (HTPC ? "Pick a character to look it up." : "Click a character, or move to it and press Enter, to look up the word starting there.") : "No match there.", C_DIM, LH, 0, 2);
    for (i = 0; i < tx_nres && i < nres; i++) {
        static char w[120], rd[120], g[120]; char row[300];
        dict_row(tx_res[i], w, rd, g, 120);
        ys_cpy(row, *w ? w : rd, sizeof row);
        if (*w) { ys_cat(row, " ", sizeof row); ys_cat(row, rd, sizeof row); }
        ys_cat(row, "  ", sizeof row); ys_cat(row, g, sizeof row);
        wrap(CX, y, CW - (HTPC ? 0 : 50), row, i ? C_INK : C_ACC, LH, 0, 1);
#if !HTPC
        if (button(CX + CW - 46, y, 44, 20, "Add")) { flash_add(my_add(&P, w, rd, g, tx_lines[tx_sel], plat_today())); save_profile(); }
#else
        if (i == 0 && (IN.pressed & A_X)) { flash_add(my_add(&P, w, rd, g, tx_lines[tx_sel], plat_today())); save_profile(); }
#endif
        y += LH + 2;
    }
#if !HTPC
    if (pages > 1) { char b[32]; ys_fmt(b, sizeof b, "Page %d/%d", tx_page + 1, pages);
      if (button(CX + CW - 100, FBY_BTN, 100, 28, b)) { tx_page = (tx_page + 1) % pages; tx_sel = tx_page * rows; tx_pos = 0; tx_nres = 0; tx_len = 0; } }
    if (*tx_msg) wrap(CX, FBY_BTN - LH + 6, pages > 1 ? CW - 108 : CW, tx_msg, C_DIM, LH, 0, 2);
#else
    if (IN.pressed & A_BACK) go(S_HOME);
    { int x = SAFE + 2; prompt_btn(&x, FBH - SAFE - 17, 0, "Pick"); prompt_btn(&x, FBH - SAFE - 17, 2, "Add"); prompt_btn(&x, FBH - SAFE - 17, 1, "Menu"); if (pages > 1) prompt_btn(&x, FBH - SAFE - 17, 5, "More"); }
#endif
}

/* ---- your own words ---- */
static void scr_mywords(void) {
    int per = HTPC ? 6 : 11, h = HTPC ? 19 : 24, y = CY + LH, i, bw = (CW - 16) / 5, total = (P.nmy + per - 1) / per;
    char b[160];
    chrome("My words", 0);
    ys_fmt(b, sizeof b, HTPC ? "%d of %d" : "%d of %d. They review with your flashcards.", P.nmy, YG_MY_MAX);
    text(CX, CY, b, C_DIM);
    if (my_sel >= P.nmy) my_sel = P.nmy ? P.nmy - 1 : 0;
    if (!P.nmy) wrap(CX, y + 4, CW, "Nothing here yet. Add a word, or save one from the dictionary in the Library. To bring in words you saved in the browser, copy them from YomiGo Reader and press Import.", C_INK, LH, 0, 0);
    for (i = page * per; i < P.nmy && i < (page + 1) * per; i++) {
        const MyCard *m = &P.my[i]; int hit = ui_hit(CX, y, CW, h), sel = i == my_sel;
        if (ui_focused() || sel) fill(CX, y, CW, h, HTPC ? C_PANEL : 0xF8D4DC);
        if (sel) frame(CX, y, CW, h, C_ACC);
        ys_cpy(b, m->word, sizeof b);
        if (ys_cmp(m->word, m->reading)) { ys_cat(b, " ", sizeof b); ys_cat(b, m->reading, sizeof b); }
        ys_cat(b, "  ", sizeof b); ys_cat(b, m->gloss, sizeof b);
        wrap(CX + 4, y + (h - 16) / 2, CW - 8, b, C_INK, LH, 0, 1);
        if (hit) { my_sel = i; my_confirm = 0; }
        y += h + 1;
    }
    if (button(CX, FBY_BTN, bw, 28, "Add")) { add_step = 0; input[0] = 0; add_word[0] = add_reading[0] = 0; go(S_MYADD); return; }
    if (button(CX + bw + 4, FBY_BTN, bw, 28, "Import")) {
        static char clip[400000]; int got;
        plat_clip_get(clip, sizeof clip);
        got = my_import(&P, clip, plat_today());
        save_profile();
        flash(got ? "Imported!" : "Nothing new to import");
    }
    if (button(CX + 2 * (bw + 4), FBY_BTN, bw, 28, "Copy")) {
        /* Tab-separated, which Anki imports as it stands */
        static char all[YG_SAVE_CAP];
        if (my_export(&P, all, sizeof all) > 0) { plat_clip_set(all); flash("Copied all!"); }
    }
    if (P.nmy && button(CX + 3 * (bw + 4), FBY_BTN, bw, 28, my_confirm ? "Sure?" : "Delete")) {
        /* There's no undo, so it takes a second press */
        if (my_confirm) { my_delete(&P, my_sel); save_profile(); my_confirm = 0; flash("Deleted"); }
        else my_confirm = 1;
    }
    if (total > 1 && button(CX + 4 * (bw + 4), FBY_BTN, bw, 28, "Page")) { page = (page + 1) % total; my_sel = page * per; }
    if (IN.pressed & A_BACK) { my_confirm = 0; go(S_CARDS); return; }
#if HTPC
    footer("OK", "Back");
#endif
}
static void scr_myadd(void) {
    static const char *const ask[] = {"The word, as it's written:", "Its reading, in kana or romaji. Leave it empty if it's the same:", "What it means:"};
    int submit;
    chrome("Add a word", 0);
    wrap(CX, CY, CW, ask[add_step], C_INK, LH, 0, 2);
    input_box(CX, CY + 2 * LH, CW, 1);
#if HTPC
    osk(FBH - SAFE - 75);
    submit = (IN.pressed & A_START) != 0;
    { int x = SAFE + 2; prompt_btn(&x, FBH - SAFE - 17, 0, "Key"); prompt_btn(&x, FBH - SAFE - 17, 1, "Del"); prompt_btn(&x, FBH - SAFE - 17, 6, "Next"); }
    if ((IN.pressed & A_BACK) && !input[0] && add_step == 0) { go(S_MYWORDS); return; }
#else
    text_entry();
    submit = button(CX + CW - 120, FBY_BTN, 120, 28, add_step < 2 ? "Next" : "Add");
    if (button(CX, FBY_BTN, 90, 28, "Cancel") || (IN.pressed & A_BACK)) { input[0] = 0; go(S_MYWORDS); return; }
    clip_buttons(CX + CW - 230, FBY_BTN - 26);
#endif
    if (!submit) return;
    if (add_step == 0) {
        if (!input[0]) return;
        ys_cpy(add_word, input, sizeof add_word); input[0] = 0; add_step = 1;
    } else if (add_step == 1) {
        /* romaji is stored as the kana it spells, so answers can be checked later */
        if (input[0]) norm_kana(input, add_reading, sizeof add_reading); else add_reading[0] = 0;
        input[0] = 0; add_step = 2;
    } else if (input[0]) {
        flash_add(my_add(&P, add_word, add_reading, input, "", plat_today()));
        save_profile();
        input[0] = 0;
        go(S_MYWORDS);
    }
}

/* ---- exams ---- */
static void scr_exams(void) {
    int i, y = CY, gate = profile_exam_gate(&P);
    chrome(L("Exams", "しけん", 2), 0);
    for (i = 0; i < YG_NEXAMS; i++) {
        const YgExam *e = &YG_EXAMS[i];
        int open = e->day < P.day, passed = P.exam_score[i] >= 80, h = HTPC ? 22 : 34;
        char b[32];
        if (open) {
            if (button(CX, y, HTPC ? 190 : 270, h, e->name)) { exam_id = i; quiz_exam(&Z, i); start_quiz(QM_EXAM); }
        } else { fill(CX, y, HTPC ? 190 : 270, h, HTPC ? C_LO : 0xE4D8CC); text(CX + 8, y + (h - 16) / 2, e->name, C_DIM); }
        if (passed) ys_fmt(b, sizeof b, "Best %d%%", P.exam_score[i], 0);
        else ys_cpy(b, open ? (i == gate ? "Required" : "Open") : "Locked", sizeof b);
        text(CX + (HTPC ? 198 : 282), y + (h - 16) / 2, b, passed ? C_GOOD : i == gate ? C_BAD : C_DIM);
        y += h + (HTPC ? 2 : 6);
    }
    if (!HTPC) wrap(CX, y + 4, CW, "These are YomiGo's own exams, not official JLPT tests. N3, N2, N1, and Beyond N1 arrive in updates. There's no official N0, so YomiGo will offer its own Beyond N1 stage.", C_DIM, 18, 0, 4);
#if HTPC
    if (IN.pressed & A_BACK) go(S_HOME);
    footer("Challenge", "Menu");
#endif
}

/* ---- quest log ---- */
static void scr_quest(void) {
    int r = profile_rank(&P), i, y = CY, ja = profile_stage(&P) >= 3 && !P.force_en;
    char b[64];
    chrome(L("Quest", "クエスト", 3), 0);
    text2(CX, y, rank_name(r, ja), C_ACC); y += 34;
    ys_fmt(b, sizeof b, "XP %d   Best streak %d", P.xp, P.best_streak); text(CX, y, b, C_INK); y += LH;
    { int rr; int bw = CW / rank_count(); for (rr = 0; rr < rank_count(); rr++) {
        fill(CX + rr * bw, y, bw - 2, 10, rr <= r ? C_ACC : (HTPC ? C_LO : 0xD9C8BC)); } y += 16; }
    if (r + 1 < rank_count()) { ys_cpy(b, "Next: ", sizeof b); ys_cat(b, rank_name(r + 1, ja), sizeof b); text(CX, y, b, C_DIM); y += LH; }
    y += 4;
    { int per = HTPC ? 5 : ACH_COUNT, cols = HTPC ? 1 : 1, shown = 0;
      (void)cols;
      if (IN.pressed & (A_R | A_RIGHT)) page = (page + 1) % 3;
      if (IN.pressed & (A_L | A_LEFT)) page = (page + 2) % 3;
      for (i = HTPC ? page * 5 : 0; i < ACH_COUNT && shown < per; i++, shown++) {
        int got = (P.ach >> i) & 1;
        text(CX, y, got ? "\xE2\x98\x85" : "\xE2\x98\x86", got ? C_ACC : C_DIM);
        text(CX + 20, y, ja ? YG_ACH[i].ja : YG_ACH[i].en, got ? C_INK : C_DIM);
        if (!HTPC) text(CX + 170, y, YG_ACH[i].desc, C_DIM);
        y += HTPC ? 15 : 18;
      } }
#if HTPC
    if (IN.pressed & A_BACK) go(S_HOME);
    footer(0, "Menu");
#endif
}

/* ---- library ---- */
static void scr_library(void) {
    static const char *const tabs[] = {"Media", "Links", "Files", "Dict", "Guide"};
    static int gsec;
    int i, y, per = HTPC ? 6 : 11, n, stage = profile_stage(&P), had = input[0];
    char b[96];
    chrome(L("Library", "本だな", 5), 0);
    for (i = 0; i < 5; i++) {
        int w = CW / 5;
        /* On the TV the search box's keyboard needs OK, so its tabs change with L and R only */
        if (HTPC && tab == 3) { fill(CX + i * w, CY, w - 4, 22, C_PANEL); text_c(CX + i * w + (w - 4) / 2, CY + 3, tabs[i], C_INK); }
        else if (button(CX + i * w, CY, w - 4, 22, tabs[i])) { tab = i; page = 0; input[0] = 0; IN.pressed &= ~A_OK; }
        if (tab == i) fill(CX + i * w, CY + 22, w - 4, 2, C_ACC);
    }
    /* focus follows, so pressing OK can't jump back to the tab that had it */
    if (IN.pressed & A_R) { tab = (tab + 1) % 5; page = 0; input[0] = 0; ui_set_focus(tab); }
    if (IN.pressed & A_L) { tab = (tab + 4) % 5; page = 0; input[0] = 0; ui_set_focus(tab); }
    y = CY + 30;
    if (tab == 0) {
        static short list[YG_NMEDIA]; int m = 0;
        for (i = 0; i < YG_NMEDIA; i++) if (YG_MEDIA[i].stage <= stage + 1) list[m++] = (short)i;
        n = m; per = HTPC ? 3 : 5;
        for (i = page * per; i < n && i < (page + 1) * per; i++) {
            const YgMedia *x = &YG_MEDIA[list[i]];
            ys_fmt(b, sizeof b, "%d", x->year, 0);
            text(CX, y, x->ja, C_ACC); text(CX + CW - 40, y, b, C_DIM); y += LH;
            ys_cpy(b, x->type, sizeof b); ys_cat(b, ": ", sizeof b); ys_cat(b, x->title, sizeof b); text(CX, y, b, C_INK); y += LH;
            y += (HTPC ? 15 : LH - 2) * wrap(CX, y, CW, x->task, C_DIM, HTPC ? 15 : LH - 2, 0, 2) + 2;
        }
    } else if (tab == 1) {
        n = YG_NRES;
        for (i = page * per; i < n && i < (page + 1) * per; i++) {
            if (button(CX, y, HTPC ? CW : 180, 22, YG_RES[i].name)) plat_open(YG_RES[i].url);
            if (!HTPC) wrap(CX + 188, y + 3, CW - 188, YG_RES[i].desc, C_DIM, 16, 0, 1);
            y += 26;
        }
    } else if (tab == 3) {
        static int res[9], nres, searched; int rows = HTPC ? 3 : 8, go_search;
        n = 0;
        input_box(CX, y, CW, 1);
#if HTPC
        osk(FBH - SAFE - 75);
        go_search = (IN.pressed & A_START) != 0;
        { int x = SAFE + 2; prompt_btn(&x, FBH - SAFE - 17, 0, "Key"); prompt_btn(&x, FBH - SAFE - 17, 1, "Del"); prompt_btn(&x, FBH - SAFE - 17, 6, "Find"); if (nres) prompt_btn(&x, FBH - SAFE - 17, 2, "Add 1st"); }
#else
        text_entry();
        go_search = (IN.pressed & A_OK) != 0;
        if (button(CX + CW - 90, FBY_BTN, 90, 28, "Search")) go_search = 1;
        clip_buttons(CX, FBY_BTN + 4);
#endif
        y += 27;
        if (go_search && input[0]) { nres = dict_search(input, res, rows); searched = 1; }
#if HTPC
        /* The on-screen keyboard has the focus, so the extra button saves the top match */
        if ((IN.pressed & A_X) && nres) { static char aw[120], ar[120], ag[120]; dict_row(res[0], aw, ar, ag, 120); flash_add(my_add(&P, aw, ar, ag, "", plat_today())); save_profile(); }
#endif
        for (i = 0; i < nres && i < rows; i++) {
            static char w[120], rd[120], g[120]; char row[200];
            dict_row(res[i], w, rd, g, 120);
            if (i == 0) copy_src = *w ? w : rd;
            if (HTPC) { ys_cpy(row, *w ? w : rd, sizeof row); ys_cat(row, " ", sizeof row); if (*w) { ys_cat(row, rd, sizeof row); ys_cat(row, " ", sizeof row); } ys_cat(row, g, sizeof row); wrap(CX, y, CW, row, C_INK, LH, 0, 1); y += LH; }
            else {
                int tw = text(CX, y, *w ? w : rd, C_ACC); if (*w) text(CX + tw + 10, y, rd, C_DIM);
                wrap(CX, y + 16, CW - 44, g, C_INK, LH, 0, 1);
                if (button(CX + CW - 40, y + 6, 38, 22, "Add")) { flash_add(my_add(&P, w, rd, g, "", plat_today())); save_profile(); }
                y += 34;
            }
        }
        if (!nres) wrap(CX, y, CW, searched ? "Nothing found. Try the plain form of the word." : "Search in Japanese, romaji, or English. Words from JMdict by EDRDG, CC BY-SA 4.0.", C_DIM, LH, 0, 3);
    } else if (tab == 4) {
        int lp = (FBY_BTN - y - 22) / LH, lines;
        n = 0;
        text(CX, y, YG_GUIDE[gsec].a, C_ACC);
        lines = wrap(CX, y + LH + 2, CW, YG_GUIDE[gsec].b, C_INK, LH, page * lp, lp);
        if (button(CX + CW - 90, FBY_BTN, 90, 28, L("Next", "つぎ", 2))) {
            if ((page + 1) * lp < lines) page++;
            else { gsec = (gsec + 1) % YG_NGUIDE; page = 0; }
        }
        if (button(CX, FBY_BTN, 90, 28, L("Back", "もどる", 2))) {
            if (page > 0) page--;
            else { gsec = (gsec + YG_NGUIDE - 1) % YG_NGUIDE; page = 0; }
        }
    } else {
        n = plat_scan_material();
        if (!n) wrap(CX, y, CW, "No Material folder found. Put one beside this program, or in the folder above it.", C_DIM, LH, 0, 3);
        for (i = page * per; i < n && i < (page + 1) * per; i++) {
            if (button(CX, y, CW, 22, plat_material(i))) plat_open(plat_material_full(i));
            y += 26;
        }
    }
    if (n > per) {
        ys_fmt(b, sizeof b, "Page %d of %d", page + 1, (n + per - 1) / per);
        text(CX + CW / 2 - 40, FBY_BTN + 6, b, C_DIM);
        if (button(CX, FBY_BTN, 70, 28, L("Prev", "前", 5)) && page > 0) page--;
        if (button(CX + CW - 70, FBY_BTN, 70, 28, L("Next", "つぎ", 2)) && (page + 1) * per < n) page++;
    }
#if HTPC
    if ((IN.pressed & A_BACK) && !(tab == 3 && had)) { go(S_HOME); return; }
    if (tab != 3) footer("Open", "Menu");
#endif
}

/* ---- settings ---- */
static int bind_action;
static void voice_help(void) {
    if (plat_has_tts()) show_result("Japanese voice found", "Listening lessons and Hear it buttons are on.", 1, S_SETTINGS);
    else show_result("No Japanese voice", "Listening needs one. In Windows Settings go to Time & language, then Speech, and add Japanese voices. Then restart YomiGo. Classes still work without it, and skip listening.", 0, S_SETTINGS);
}
static void scr_settings(void) {
    int y = CY, bh = HTPC ? 22 : 30, step = bh + (HTPC ? 2 : 6), w = HTPC ? CW : 300;
    char b[64];
    static const char *const pace[] = {"Pace: Gentle (1 class a day)", "Pace: Steady (2 a day)", "Pace: Sprint (3 a day)"};
    chrome(L("Settings", "せってい", 2), 0);
    if (button(CX, y, w, bh, pace[P.pace - 1])) { P.pace = P.pace % 3 + 1; save_profile(); }
    y += step;
    if (button(CX, y, w, bh, P.force_en ? "Labels: Always English" : "Labels: Gradually Japanese")) { P.force_en = !P.force_en; save_profile(); }
    y += step;
    if (button(HTPC ? CX + w / 2 + 2 : CX, HTPC ? y : y, HTPC ? w / 2 - 2 : w, bh, P.genki ? "Genki: On" : "Genki: Off")) { P.genki = !P.genki; save_profile(); }
    if (!HTPC) y += step;
    if (button(CX, y, HTPC ? w / 2 - 2 : w, bh, HTPC ? "Back up" : "Back up my save")) { save_profile(); plat_copy_backup(P.name); show_result("Backed up", "A dated copy of your save is in the backup folder, next to the program.", 1, S_SETTINGS); return; }
    y += step;
    if (button(CX, y, HTPC ? w / 2 - 2 : w, bh, plat_can_update() ? "Check for updates" : (HTPC ? "Manual updates" : "Updates: manual on this Windows"))) {
        if (plat_can_update()) plat_update_check(0);
        else { show_result("Manual updates", "This Windows can't make modern secure connections. Download new versions from github.com/knivesbatz/YomiGO/releases on another PC, then copy them over.", 1, S_SETTINGS); return; }
    }
    if (!HTPC) y += step;
    if (button(HTPC ? CX + w / 2 + 2 : CX, y, HTPC ? w / 2 - 2 : w, bh, "Switch profile")) { save_profile(); have_profile = 0; go(S_PROFILES); return; }
    y += step;
#if HTPC
    { static const char *const modes[] = {"Buttons: Auto", "Buttons: N64", "Buttons: Xbox", "Buttons: PlayStation"};
      if (button(CX, y, w, bh, modes[pad_mode()])) pad_set_mode((pad_mode() + 1) % 4); }
    y += step;
    if (button(CX, y, w / 2 - 2, bh, crt_mode() ? "TV: CRT / RF" : "TV: Flat")) crt_set(!crt_mode());
    if (button(CX + w / 2 + 2, y, w / 2 - 2, bh, plat_has_tts() ? "Voice: OK" : "Voice: None")) { voice_help(); return; }
    y += step;
    if (button(CX, y, w / 2 - 2, bh, "Rebind pad")) { bind_action = 0; pad_begin_bind(0); go(S_BIND); return; }
    ys_fmt(b, sizeof b, "Deadzone %d%%", pad_deadzone(), 0);
    if (button(CX + w / 2 + 2, y, w / 2 - 2, bh, b)) pad_set_deadzone(pad_deadzone() >= 50 ? 10 : pad_deadzone() + 10);
    if (IN.pressed & A_BACK) go(S_HOME);
    footer("Change", "Menu");
#else
    if (button(CX, y, w, bh, plat_has_tts() ? "Japanese voice: found" : "Japanese voice: none. How to add it")) { voice_help(); return; }
    y += step;
    ys_cpy(b, "YomiGo Desktop ", sizeof b); ys_cat(b, YG_VERSION, sizeof b); text(CX, y + 8, b, C_DIM);
    text(CX, y + 26, "(C) SENKOSOFT", C_DIM);
#endif
}
#if HTPC
static void scr_bind(void) {
    static const char *const names[] = {"OK", "Back", "Extra (X)", "Speak (Y)", "L", "R", "Start"};
    chrome("Rebind pad", 0);
    text_c(FBW / 2, 90, "Press the button for:", C_INK);
    text2(FBW / 2 - text_w(names[bind_action]), 112, names[bind_action], C_ACC);
    if (!pad_connected()) text_c(FBW / 2, 150, "No pad connected. Press Esc to go back.", C_BAD);
    if (!pad_binding()) {
        bind_action++;
        if (bind_action >= 7) { pad_save(); go(S_SETTINGS); return; }
        pad_begin_bind(bind_action);
    }
    if (IN.pressed & A_BACK) { pad_save(); go(S_SETTINGS); }
}
#endif

/* ---- profiles and first run ---- */
static void load_profile(const char *name) {
    static char b[YG_SAVE_CAP], fn[64];
    int n;
    ys_cpy(fn, name, sizeof fn); ys_cat(fn, ".sav", sizeof fn);
    n = plat_read(fn, b, sizeof b - 1);
    have_profile = (n > 0 && profile_load_text(&P, b, n));
    if (have_profile) { profile_touch_day(&P, plat_today()); go(P.placed || P.day ? S_HOME : S_PLACE_INTRO); }
}
static void scr_splash(void) {
    washi(0, 0, FBW, FBH, HTPC ? C_BG : C_BG, HTPC ? 0x2A1A44 : 0xEADFCC);
#if HTPC
    gradient(0, 0, FBW, FBH, 0x180A2C, 0x6A2050);
    torii(FBW / 2, 86, 6, anim * 3);
    text2(FBW / 2 - text_w("YomiGo") , 150, "YomiGo", C_ACC);
    text_c(FBW / 2, 186, "HTPC", C_PINK);
    if (anim & 16) text_c(FBW / 2, 204, "Press A or Start", C_INK);
#else
    gradient(0, 0, FBW, 150, C_PLUM, C_WINE);
    { int i; for (i = 0; i < 7; i++) sakura(40 + i * 86, 30 + (i * 37) % 80, 2); }
    fill(0, 150, FBW, 2, C_PINK);
    text2(FBW / 2 - text_w("読みGo"), 180, "読みGo", C_ACC);
    text_c(FBW / 2, 220, "YomiGo Japanese Learning System", C_INK);
    text_c(FBW / 2, 244, "Version " YG_VERSION, C_DIM);
    text_c(FBW / 2, 420, "(C) SENKOSOFT", C_DIM);
    if (anim & 16) text_c(FBW / 2, 340, "Click or press Enter", C_PLUM);
#endif
    if (IN.pressed || IN.mclick) { plat_sound(3); go(S_PROFILES); }
}
static void scr_profiles(void) {
    static char names[8][32];
    int n = plat_list_saves(names, 8), i, y;
    chrome("Who's studying?", 0);
    y = CY;
    for (i = 0; i < n; i++) {
        if (button(CX, y, CW, HTPC ? 24 : 32, names[i])) { load_profile(names[i]); return; }
        y += HTPC ? 26 : 38;
    }
    if (n < 8 && button(CX, y, CW, HTPC ? 24 : 32, "New learner")) { name_buf[0] = 0; input[0] = 0; go(S_NAME); return; }
#if HTPC
    footer("Choose", 0);
#endif
}
static void scr_name(void) {
    chrome("Your name", 0);
    text(CX, CY, "Enter a short name for this learner.", C_DIM);
    input_box(CX, CY + 22, CW, 1);
    /* Saves are named after the learner, so a Japanese name can't be used yet. Say so instead of ignoring OK. */
    { const char *p = input; int bad = 0; while (*p) if (*p++ & 0x80) bad = 1;
      if (bad) wrap(CX, CY + 50, CW, "Names use letters A-Z and numbers for now. Switch your keyboard to English, or type it in romaji.", C_BAD, LH, 0, 2); }
#if HTPC
    osk(CY + 96);
    { int x = SAFE + 2; prompt_btn(&x, FBH - SAFE - 17, 0, "Key"); prompt_btn(&x, FBH - SAFE - 17, 1, "Del"); prompt_btn(&x, FBH - SAFE - 17, 6, "Done"); }
    if ((IN.pressed & A_START) && input[0]) {
#else
    text_entry();
    if (ys_len(input) > 16) input[16] = 0;
    if (button(CX + CW - 120, FBY_BTN, 120, 28, "OK") && input[0]) {
#endif
        /* Saves are named after the learner, so keep only safe filename characters */
        int i, k = 0; char clean[YG_NAME];
        for (i = 0; input[i] && k < 16; i++) {
            char c = input[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_') clean[k++] = c;
        }
        clean[k] = 0;
        if (k) { profile_new(&P, clean); have_profile = 1; save_profile(); input[0] = 0; go(S_FIRST); }
    }
}
static void scr_first(void) {
    chrome("Do you study with Genki?", 0);
    panel(CX, CY, CW, HTPC ? 106 : 170);
    wrap(CX + 8, CY + 4, CW - 16, "YomiGo can add practice sorted by Genki lesson. It uses YomiGo's own cards alongside your copy of the book. You can change this later in Settings.", C_INK, LH, 0, 0);
    if (button(CX, FBY_BTN - (HTPC ? 30 : 36), CW, 28, "Yes, turn on Genki practice")) { P.genki = 1; save_profile(); go(S_PLACE_INTRO); return; }
    if (button(CX, FBY_BTN, CW, 28, "No thanks")) { P.genki = 0; save_profile(); go(S_PLACE_INTRO); return; }
#if HTPC
    footer("Choose", 0);
#endif
}
static void scr_genki(void) {
    int i, y = CY + LH + 2, h = HTPC ? 17 : 30, w = HTPC ? 44 : 100, n = plat_scan_material();
    chrome("Genki practice", 0);
    text(CX, CY, "By lesson, using YomiGo's own cards.", C_DIM);
    for (i = 0; i < YG_NGENKI; i++) {
        char b[16];
        ys_fmt(b, sizeof b, "L%d", YG_GENKI[i].lesson, 0);
        if (button(CX, y, w, h, b)) { quiz_genki(&Z, i); start_quiz(QM_PRACTICE); return; }
        if (!HTPC || ui_focused()) wrap(CX + w + 6, HTPC ? CY + LH + 2 : y + (h - 16) / 2, CW - w - 6, YG_GENKI[i].topics, C_DIM, LH, 0, HTPC ? 5 : 1);
        y += h + (HTPC ? 1 : 2);
    }
    if (!HTPC) wrap(CX, y + 4, CW, "Follows the order of the 3rd edition. Lessons with no YomiGo cards yet arrive with the N4 update.", C_DIM, 18, 0, 2);
    for (i = 0; i < n; i++) if (ys_contains_ci(plat_material(i), "genki")) {
        if (button(HTPC ? CX + w + 6 : CX, FBY_BTN, HTPC ? 170 : 170, 28, "Open my Genki file")) plat_open(plat_material_full(i));
        break;
    }
    if (IN.pressed & A_BACK) go(S_CARDS);
#if HTPC
    footer("Practice", "Back");
#endif
}
static void scr_place_intro(void) {
    chrome("Entrance Exam", 0);
    panel(CX, CY, CW, HTPC ? 106 : 170);
    wrap(CX + 8, CY + 4, CW - 16, "A short test finds your starting point. It begins with romaji, climbs through kana, words, grammar, and kanji, and stops when it gets too hard. You can press I don't know at any time.", C_INK, LH, 0, 0);
    if (button(CX, FBY_BTN - (HTPC ? 30 : 36), CW, 28, "Take the exam")) { place_section = 0; place_passed = 0; placement_section(&Z, 0); start_quiz(QM_PLACE); return; }
    if (button(CX, FBY_BTN, CW, 28, HTPC ? "Total beginner: day one" : "I'm a total beginner. Start from day one.")) { profile_place(&P, 0, plat_today()); save_profile(); go(S_HOME); return; }
#if HTPC
    footer("Choose", 0);
#endif
}
static void scr_place_result(void) {
    char b[160];
    chrome("Your starting point", 0);
    panel(CX, CY, CW, HTPC ? 100 : 170);
    text2(CX + 10, CY + 8, YG_STAGES[profile_stage(&P)].ja, C_ACC);
    text(CX + 10, CY + 44, YG_STAGES[profile_stage(&P)].name, C_INK);
    ys_fmt(b, sizeof b, "You'll begin at class %d. Earlier material goes to your flashcards for review, so fundamentals stay fresh. You can retake this exam by making a new learner.", P.day + 1, 0);
    wrap(CX + 10, CY + 64, CW - 20, b, C_DIM, LH, 0, 0);
    if (button(CX + CW / 2 - 70, FBY_BTN, 140, 28, "Begin")) go(S_HOME);
#if HTPC
    footer("Begin", 0);
#endif
}

/* Test driver: plays the current screen correctly through the real screen code */
void app_test_auto(void) {
    IN.pressed = 0; IN.num = 0; IN.text[0] = 0; IN.mx = IN.my = -1; IN.mclick = 0;
    switch (scr) {
    case S_READ:
        if (rd_q < 0 || rd_pick >= 0) IN.pressed = A_OK;
        else { int i; for (i = 0; i < 3; i++) if (rd_order[i] == 0) IN.num = i + 1; }
        break;
    case S_TYPING:
        ys_cpy(input, YG_CARDS[ty_cards[ty_i]].romaji, sizeof input);
        IN.pressed = HTPC ? A_START : A_OK;
        break;
    case S_QUIZ: {
        Question *q = &Z.q[qi];
        if (qanswered) { IN.pressed = HTPC ? A_OK : A_OK; break; }
        if (q->kind == Q_CHOICE) { IN.num = q->answer + 1; break; }
        ys_cpy(input, q->want, sizeof input);
        IN.pressed = HTPC ? A_START : A_OK;
        break; }
    case S_CHAT:
        if (chat_turn < YG_CHATS[chat_id].nturns) ys_cpy(input, YG_CHATS[chat_id].turns[chat_turn].want[0], sizeof input);
        IN.pressed = HTPC ? A_START : A_OK;
        break;
    case S_HOME:
        if (P.done_today >= P.pace && P.last_day == plat_today() && profile_exam_gate(&P) < 0) plat_test_next_day();
        else IN.pressed = A_OK;
        break;
    default: IN.pressed = A_OK; break;
    }
}
int app_test_done(void) { return have_profile && profile_course_done(&P) && scr == S_HOME; }

void app_init(void) {
    static char prev[16], b[16]; int n, cur;
    yg_seed((unsigned)plat_ticks() * 2654435761u + 1);
    cur = plat_scan_material();
    /* Daily material check: anything added since last launch gets announced on the home screen */
    n = plat_read("material.count", prev, sizeof prev - 1);
    if (n > 0) { prev[n] = 0; if (cur > ys_atoi(prev)) new_material = cur - ys_atoi(prev); }
    n = ys_itoa(cur, b); plat_write("material.count", b, n);
}
void app_frame(void) {
    ui_begin();
    anim++;
    copy_src = 0;
    field_on = 0;
    if (scr != S_TEXT) tx_poll(); /* watching the clipboard carries on behind other screens */
    switch (scr) {
    case S_SPLASH: scr_splash(); break;
    case S_PROFILES: scr_profiles(); break;
    case S_NAME: scr_name(); break;
    case S_PLACE_INTRO: scr_place_intro(); break;
    case S_PLACE_RESULT: scr_place_result(); break;
    case S_HOME: scr_home(); break;
    case S_CLASS: scr_class(); break;
    case S_TEACH: scr_teach(); break;
    case S_QUIZ: scr_quiz(); break;
    case S_RESULT: scr_result(); break;
    case S_CHAT: scr_chat(); break;
    case S_TASK: scr_task(); break;
    case S_CARDS: scr_cards(); break;
    case S_EXAMS: scr_exams(); break;
    case S_QUEST: scr_quest(); break;
    case S_LIBRARY: scr_library(); break;
    case S_SETTINGS: scr_settings(); break;
#if HTPC
    case S_BIND: scr_bind(); break;
#else
    case S_BIND: break;
#endif
    case S_FIRST: scr_first(); break;
    case S_TYPING: scr_typing(); break;
    case S_READ: scr_read(); break;
    case S_MYWORDS: scr_mywords(); break;
    case S_MYADD: scr_myadd(); break;
    case S_SHADOW: scr_shadow(); break;
    case S_TEXT: scr_text(); break;
    case S_GENKI: scr_genki(); break;
    }
#if !HTPC
    rail();
#endif
    /* Copy takes what you typed, or else the Japanese on screen */
    if (IN.copy) {
        const char *src = input[0] ? input : copy_src;
        if (src && *src) { plat_clip_set(src); flash("Copied!"); }
    }
    plat_ime_field(field_on, field_x, field_y);
    if (flash_t > 0) { int w = text_w(flash_msg) + 8; flash_t--; fill(FBW - w - 4, FBH / 2, w, 20, C_ACC); text(FBW - w, FBH / 2 + 2, flash_msg, HTPC ? 0x1B1033 : 0xFFFFFF); }
    ui_end();
}
