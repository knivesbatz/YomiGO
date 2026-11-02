#include "yg.h"

/* ---- strings ---- */
int ys_len(const char *s) { int n = 0; while (s[n]) n++; return n; }
int ys_cmp(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return (unsigned char)*a - (unsigned char)*b; }
int ys_ncmp(const char *a, const char *b, int n) { while (n-- > 0) { if (*a != *b) return (unsigned char)*a - (unsigned char)*b; if (!*a) return 0; a++; b++; } return 0; }
void ys_cpy(char *d, const char *s, int cap) { int i = 0; while (s[i] && i < cap - 1) { d[i] = s[i]; i++; } d[i] = 0; }
void ys_cat(char *d, const char *s, int cap) { int n = ys_len(d); ys_cpy(d + n, s, cap - n); }
int ys_itoa(int v, char *d) {
    char t[12]; int n = 0, i = 0, neg = v < 0; unsigned u = neg ? 0u - (unsigned)v : (unsigned)v;
    do { t[n++] = (char)('0' + u % 10); u /= 10; } while (u);
    if (neg) d[i++] = '-';
    while (n) d[i++] = t[--n];
    d[i] = 0; return i;
}
int ys_atoi(const char *s) { int v = 0, neg = 0; if (*s == '-') { neg = 1; s++; } while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0'); return neg ? -v : v; }
static char lc(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
int ys_contains_ci(const char *h, const char *n) {
    int i, j;
    if (!*n) return 1;
    for (i = 0; h[i]; i++) { for (j = 0; n[j] && lc(h[i + j]) == lc(n[j]); j++) {} if (!n[j]) return 1; }
    return 0;
}
/* %d is replaced by a then b, and %% by a percent sign */
void ys_fmt(char *d, int cap, const char *f, int a, int b) {
    int i = 0, used = 0;
    while (*f && i < cap - 12) {
        if (f[0] == '%' && f[1] == 'd') { i += ys_itoa(used++ ? b : a, d + i); f += 2; }
        else if (f[0] == '%' && f[1] == '%') { d[i++] = '%'; f += 2; }
        else d[i++] = *f++;
    }
    d[i] = 0;
}
int utf8_next(const char **s) {
    const unsigned char *p = (const unsigned char *)*s; int c = *p;
    if (!c) return 0;
    if (c < 0x80) { *s += 1; return c; }
    if (c < 0xE0) { *s += 2; return ((c & 31) << 6) | (p[1] & 63); }
    if (c < 0xF0) { *s += 3; return ((c & 15) << 12) | ((p[1] & 63) << 6) | (p[2] & 63); }
    *s += 4; return 0xFFFD;
}
int utf8_put(int c, char *d) {
    if (c < 0x80) { d[0] = (char)c; return 1; }
    if (c < 0x800) { d[0] = (char)(0xC0 | c >> 6); d[1] = (char)(0x80 | (c & 63)); return 2; }
    d[0] = (char)(0xE0 | c >> 12); d[1] = (char)(0x80 | ((c >> 6) & 63)); d[2] = (char)(0x80 | (c & 63)); return 3;
}

/* ---- kana normalising: every accepted form of an answer collapses to hiragana ---- */
static int vowel_kana(int k) {
    static const char *const rows[5] = { "あかさたなはまやらわがざだばぱぁゃ", "いきしちにひみりぎじぢびぴぃ", "うくすつぬふむゆるぐずづぶぷぅゅ", "えけせてねへめれげぜでべぺぇ", "おこそとのほもよろをごぞどぼぽぉょ" };
    static const int v[5] = { 0x3042, 0x3044, 0x3046, 0x3048, 0x304A };
    int i;
    for (i = 0; i < 5; i++) { const char *s = rows[i]; int c; while ((c = utf8_next(&s)) != 0) if (c == k) return v[i]; }
    return 0;
}
static int is_vowel_y(char c) { return c == 'a' || c == 'i' || c == 'u' || c == 'e' || c == 'o' || c == 'y'; }

/* Mirrors romaji_to_kana in tools/build_content.py */
static void kanji_to_kana(const char *in, char *out, int cap) {
    int o = 0;
    while (*in && o < cap - 8) {
        int i, hit = 0;
        for (i = 0; i < YG_NSUBS; i++) {
            int n = ys_len(YG_SUBS[i].a);
            if (ys_ncmp(in, YG_SUBS[i].a, n) == 0) { ys_cpy(out + o, YG_SUBS[i].b, cap - o); o += ys_len(out + o); in += n; hit = 1; break; }
        }
        if (!hit) out[o++] = *in++;
    }
    out[o] = 0;
}
void norm_kana(const char *input, char *out, int cap) {
    static char sub[512], latin[512];
    const char *s; int o = 0, last = 0, c;
    kanji_to_kana(input, sub, sizeof sub);
    /* lower-case and convert romaji runs */
    { int i = 0; const char *p = sub;
      while (*p && i < (int)sizeof latin - 8) {
        char ch = lc(*p);
        if (ch == ' ' || ch == ',' || ch == '.' || ch == '!' || ch == '?') { p++; continue; }
        if ((unsigned char)ch >= 0x80) { latin[i++] = *p++; continue; }
        { int j, hit = 0;
          char low[6]; int k;
          for (k = 0; k < 5 && p[k]; k++) low[k] = lc(p[k]);
          low[k] = 0;
          for (j = 0; j < YG_NROMA; j++) {
              int n = ys_len(YG_ROMA[j].a);
              if (n <= k && ys_ncmp(low, YG_ROMA[j].a, n) == 0) { ys_cpy(latin + i, YG_ROMA[j].b, (int)sizeof latin - i); i += ys_len(latin + i); p += n; hit = 1; break; }
          }
          if (hit) continue;
          if (ch == 'n') {
              char nx = lc(p[1]);
              if (nx == '\'') p += 2;
              else if (nx == 'n' && !is_vowel_y(lc(p[2]))) p += 2;
              else p += 1;
              i += utf8_put(0x3093, latin + i);
          } else if (ch == '\'') { p++; }
          else if (ch == '-') { p++; i += utf8_put(0x30FC, latin + i); }
          else if ((ch >= 'a' && ch <= 'z') && (lc(p[1]) == ch || (ch == 't' && lc(p[1]) == 'c'))) { p++; i += utf8_put(0x3063, latin + i); }
          else latin[i++] = *p++;
        }
      }
      latin[i] = 0;
    }
    s = latin;
    while ((c = utf8_next(&s)) != 0 && o < cap - 4) {
        if (c >= 0x30A1 && c <= 0x30F6) c -= 0x60;           /* katakana to hiragana */
        if (c == 0x30FC) { c = vowel_kana(last); if (!c) continue; } /* long vowel bar */
        if (c == 0x3002 || c == 0x3001 || c == 0xFF01 || c == 0xFF1F || c == 0x3000 || c == 0x30FB) continue; /* punctuation and spaces */
        o += utf8_put(c, out + o);
        last = c;
    }
    out[o] = 0;
}
int answer_ok(const char *input, const char *want, const char *romaji) {
    static char a[256], b[256];
    if (!*input) return 0;
    norm_kana(input, a, sizeof a);
    if (!a[0]) return 0;
    norm_kana(want, b, sizeof b);
    if (ys_cmp(a, b) == 0) return 1;
    if (romaji && *romaji) { norm_kana(romaji, b, sizeof b); if (ys_cmp(a, b) == 0) return 1; }
    return 0;
}
/* Particles are spelled as they sound when people type quickly, so は/を/へ are compared by sound in chat. */
static void particle_sound(char *s) {
    char *p = s;
    while (*p) {
        if (!ys_ncmp(p, "は", 3)) { p[0] = (char)0xE3; p[1] = (char)0x82; p[2] = (char)0x8F; }  /* わ */
        else if (!ys_ncmp(p, "を", 3)) { p[0] = (char)0xE3; p[1] = (char)0x81; p[2] = (char)0x8A; } /* お */
        else if (!ys_ncmp(p, "へ", 3)) { p[0] = (char)0xE3; p[1] = (char)0x81; p[2] = (char)0x88; } /* え */
        p++;
    }
}
int chat_ok(const char *input, const YgTurn *t) {
    static char a[256], b[256]; int i;
    norm_kana(input, a, sizeof a);
    if (!a[0]) return 0;
    particle_sound(a);
    for (i = 0; i < t->nwant; i++) {
        norm_kana(t->want[i], b, sizeof b);
        particle_sound(b);
        if (ys_cmp(a, b) == 0) return 1;
    }
    return 0;
}

/* ---- spaced repetition ---- */
void srs_new(CardState *c, int today) { c->seen = 1; c->reps = 0; c->ivl = 0; c->lapses = 0; c->ease = 2500; c->due = today; }
void srs_grade(CardState *c, int g, int today) {
    int ivl = c->ivl, ease = c->ease;
    if (g == GRADE_AGAIN) { c->reps = 0; ivl = 0; ease -= 200; c->lapses++; }
    else {
        if (g == GRADE_HARD) { ivl = ivl * 12 / 10; if (ivl < 1) ivl = 1; ease -= 150; }
        else if (g == GRADE_GOOD) { ivl = c->reps == 0 ? 1 : c->reps == 1 ? 3 : ivl * ease / 1000; }
        else { ivl = c->reps == 0 ? 3 : c->reps == 1 ? 6 : (ivl * ease / 1000) * 13 / 10; ease += 150; }
        c->reps++;
    }
    if (ease < 1300) ease = 1300;
    if (ivl > 3650) ivl = 3650;
    c->ivl = (short)ivl; c->ease = (short)ease; c->due = today + ivl;
}

/* ---- profile ---- */
void profile_new(Profile *p, const char *name) {
    char *b = (char *)p; int i;
    for (i = 0; i < (int)sizeof *p; i++) b[i] = 0;
    ys_cpy(p->name, name, YG_NAME);
    p->pace = 1;
}
static int put(char *o, int cap, int n, const char *k, int v) {
    char t[16];
    if (n + 48 >= cap) return n;
    ys_cpy(o + n, k, cap - n); n += ys_len(o + n);
    o[n++] = '='; ys_itoa(v, t); ys_cpy(o + n, t, cap - n); n += ys_len(o + n); o[n++] = '\n'; o[n] = 0;
    return n;
}
int profile_save_text(const Profile *p, char *o, int cap) {
    int n = 0, i; char t[16];
    ys_cpy(o, "YOMIGO-SAVE 1\nname=", cap); n = ys_len(o);
    ys_cpy(o + n, p->name, cap - n); n += ys_len(o + n); o[n++] = '\n'; o[n] = 0;
    n = put(o, cap, n, "xp", p->xp); n = put(o, cap, n, "streak", p->streak); n = put(o, cap, n, "best", p->best_streak);
    n = put(o, cap, n, "last", p->last_day); n = put(o, cap, n, "today", p->done_today); n = put(o, cap, n, "placed", p->placed);
    n = put(o, cap, n, "day", p->day); n = put(o, cap, n, "step", p->step); n = put(o, cap, n, "pace", p->pace);
    n = put(o, cap, n, "en", p->force_en); n = put(o, cap, n, "genki", p->genki); n = put(o, cap, n, "reviews", p->reviews); n = put(o, cap, n, "perfects", p->perfects);
    n = put(o, cap, n, "chats", p->chats); n = put(o, cap, n, "hw", p->homeworks); n = put(o, cap, n, "ach", (int)p->ach);
    for (i = 0; i < YG_NEXAMS; i++) if (p->exam_score[i]) {
        ys_cpy(o + n, "exam.", cap - n); n += 5; n = put(o, cap, n, YG_EXAMS[i].id, p->exam_score[i]);
    }
    for (i = 0; i < YG_NCARDS; i++) if (p->cards[i].seen) {
        const CardState *c = &p->cards[i];
        if (n + 96 >= cap) return -1;
        ys_cpy(o + n, "card ", cap - n); n += 5;
        ys_cpy(o + n, YG_CARDS[i].id, cap - n); n += ys_len(o + n);
        { int vals[5]; int j; vals[0] = c->reps; vals[1] = c->ivl; vals[2] = c->lapses; vals[3] = c->ease; vals[4] = c->due;
          for (j = 0; j < 5; j++) { o[n++] = ' '; ys_itoa(vals[j], t); ys_cpy(o + n, t, cap - n); n += ys_len(o + n); } }
        o[n++] = '\n'; o[n] = 0;
    }
    return n;
}
static int card_index(const char *id, int n) {
    int i;
    for (i = 0; i < YG_NCARDS; i++) if (ys_ncmp(YG_CARDS[i].id, id, n) == 0 && YG_CARDS[i].id[n] == 0) return i;
    return -1;
}
/* Rejects anything that isn't a YomiGo save, and clamps values so a damaged file can't index out of range */
int profile_load_text(Profile *p, const char *text, int len) {
    int i = 0, nl;
    static const char head[] = "YOMIGO-SAVE 1\n";
    if (len < 14 || ys_ncmp(text, head, 14) != 0) return 0;
    profile_new(p, "");
    i = 14;
    while (i < len) {
        char line[160]; int n = 0, eq;
        for (nl = i; nl < len && text[nl] != '\n'; nl++) if (n < 159) line[n++] = text[nl];
        line[n] = 0; i = nl + 1;
        if (n > 0 && line[n - 1] == '\r') line[--n] = 0;
        if (ys_ncmp(line, "card ", 5) == 0) {
            char *q = line + 5; int idn = 0, vals[5], j, k;
            while (q[idn] && q[idn] != ' ') idn++;
            k = card_index(q, idn);
            q += idn;
            for (j = 0; j < 5; j++) { if (*q == ' ') q++; vals[j] = ys_atoi(q); if (*q == '-') q++; while (*q >= '0' && *q <= '9') q++; }
            if (k >= 0) {
                CardState *c = &p->cards[k];
                c->seen = 1; c->reps = (short)vals[0]; c->ivl = (short)(vals[1] < 0 ? 0 : vals[1] > 3650 ? 3650 : vals[1]);
                c->lapses = (short)vals[2]; c->ease = (short)(vals[3] < 1300 ? 1300 : vals[3] > 5000 ? 5000 : vals[3]); c->due = vals[4];
            }
            continue;
        }
        for (eq = 0; line[eq] && line[eq] != '='; eq++) {}
        if (!line[eq]) continue;
        line[eq] = 0;
        { const char *k = line, *v = line + eq + 1; int x = ys_atoi(v);
          if (!ys_cmp(k, "name")) ys_cpy(p->name, v, YG_NAME);
          else if (!ys_cmp(k, "xp")) p->xp = x;
          else if (!ys_cmp(k, "streak")) p->streak = x;
          else if (!ys_cmp(k, "best")) p->best_streak = x;
          else if (!ys_cmp(k, "last")) p->last_day = x;
          else if (!ys_cmp(k, "today")) p->done_today = x;
          else if (!ys_cmp(k, "placed")) p->placed = x;
          else if (!ys_cmp(k, "day")) p->day = x < 0 ? 0 : x > YG_NDAYS ? YG_NDAYS : x;
          else if (!ys_cmp(k, "step")) p->step = x < 0 ? 0 : x > STEP_DONE ? 0 : x;
          else if (!ys_cmp(k, "pace")) p->pace = x < 1 ? 1 : x > 3 ? 3 : x;
          else if (!ys_cmp(k, "en")) p->force_en = x;
          else if (!ys_cmp(k, "genki")) p->genki = x != 0;
          else if (!ys_cmp(k, "reviews")) p->reviews = x;
          else if (!ys_cmp(k, "perfects")) p->perfects = x;
          else if (!ys_cmp(k, "chats")) p->chats = x;
          else if (!ys_cmp(k, "hw")) p->homeworks = x;
          else if (!ys_cmp(k, "ach")) p->ach = (unsigned)x;
          else if (!ys_ncmp(k, "exam.", 5)) { int e; for (e = 0; e < YG_NEXAMS; e++) if (!ys_cmp(k + 5, YG_EXAMS[e].id)) p->exam_score[e] = (unsigned char)(x < 0 ? 0 : x > 100 ? 100 : x); }
        }
    }
    return p->name[0] != 0;
}
int profile_stage(const Profile *p) { int d = p->day >= YG_NDAYS ? YG_NDAYS - 1 : p->day; return YG_DAYS[d].stage; }
int profile_due(const Profile *p, int today) {
    int i, n = 0;
    for (i = 0; i < YG_NCARDS; i++) if (p->cards[i].seen && p->cards[i].due <= today) n++;
    return n;
}
#define PASS_MARK 80
int profile_exam_gate(const Profile *p) {
    int i;
    for (i = 0; i < YG_NEXAMS; i++) if (YG_EXAMS[i].day < p->day && p->exam_score[i] < PASS_MARK) return i;
    return -1;
}
int profile_course_done(const Profile *p) { return p->day >= YG_NDAYS && profile_exam_gate(p) < 0; }
void profile_touch_day(Profile *p, int today) {
    if (p->last_day != today) { p->done_today = 0; }
}
void profile_learn_day(Profile *p, int day, int today) {
    const YgDay *d = &YG_DAYS[day]; int i;
    for (i = 0; i < d->nnew; i++) if (!p->cards[d->cards[i]].seen) srs_new(&p->cards[d->cards[i]], today);
}
void profile_complete_day(Profile *p, int today) {
    if (p->last_day != today) {
        p->streak = (p->last_day == today - 1) ? p->streak + 1 : 1;
        p->done_today = 0; p->last_day = today;
    }
    if (p->streak > p->best_streak) p->best_streak = p->streak;
    p->done_today++;
    p->xp += 100;
    p->day++; p->step = 0;
    ach_check(p);
}
int profile_exam_pass(Profile *p, int exam, int score, int today) {
    (void)today;
    if (score > p->exam_score[exam]) p->exam_score[exam] = (unsigned char)score;
    if (score >= PASS_MARK) { p->xp += 200 * YG_EXAMS[exam].stars; ach_check(p); return 1; }
    p->xp += 20; /* showing up counts, a little */
    return 0;
}
/* Everything below the placed stage counts as already taught, and those exams as credited */
int profile_place(Profile *p, int stage, int today) {
    int i, j;
    if (stage > YG_NSTAGES - 1) stage = YG_NSTAGES - 1;
    p->day = YG_STAGES[stage].first; p->step = 0; p->placed = 1;
    for (i = 0; i < p->day; i++) {
        for (j = 0; j < YG_DAYS[i].nnew; j++) { CardState *c = &p->cards[YG_DAYS[i].cards[j]]; srs_new(c, today); c->reps = 2; c->ivl = 3; c->due = today + 1 + (j % 5); }
    }
    for (i = 0; i < YG_NEXAMS; i++) if (YG_EXAMS[i].day < p->day) p->exam_score[i] = PASS_MARK;
    return p->day;
}
static const char *const RANKS[][2] = {
    {"Newcomer", "入門者"}, {"White Sash", "白帯"}, {"Road Walker", "道を行く者"}, {"Scribe", "書記"},
    {"Castle Guard", "城の守り"}, {"Gatekeeper", "門番"}, {"Kanji Knight", "漢字の騎士"}, {"N5 Champion", "N5王者"}
};
int rank_count(void) { return 8; }
const char *rank_name(int r, int ja) { if (r < 0) r = 0; if (r > 7) r = 7; return RANKS[r][ja ? 1 : 0]; }
int profile_rank(const Profile *p) {
    int i, r = 0;
    for (i = 0; i < YG_NEXAMS; i++) if (p->exam_score[i] >= PASS_MARK) r = i + 1;
    return r;
}
const YgAch YG_ACH[ACH_COUNT] = {
    {"First Step", "はじめの一歩", "Finish your first class."},
    {"Three Days", "三日坊主じゃない", "Study 3 days in a row."},
    {"One Week", "一週間", "Study 7 days in a row."},
    {"One Moon", "一か月", "Study 30 days in a row."},
    {"Hiragana Master", "ひらがな名人", "Pass the Hiragana Hall Trial."},
    {"Katakana Master", "カタカナ名人", "Pass the Katakana Castle Trial."},
    {"First Words", "はじめての会話", "Finish your first conversation."},
    {"Hundred Cards", "百枚", "Review 100 flashcards."},
    {"Flawless", "満点", "Score 100% on a quiz."},
    {"N5 Champion", "N5王者", "Pass the N5 Mastery Exam."},
};
void ach_check(Profile *p) {
#define GOT(a, cond) if (cond) p->ach |= 1u << (a)
    GOT(ACH_FIRST_CLASS, p->day > 0 && !p->placed);
    GOT(ACH_FIRST_CLASS, p->homeworks > 0);
    GOT(ACH_STREAK3, p->best_streak >= 3); GOT(ACH_STREAK7, p->best_streak >= 7); GOT(ACH_STREAK30, p->best_streak >= 30);
    GOT(ACH_HIRAGANA, p->exam_score[2] >= PASS_MARK); GOT(ACH_KATAKANA, p->exam_score[3] >= PASS_MARK);
    GOT(ACH_FIRST_CHAT, p->chats > 0); GOT(ACH_REVIEW100, p->reviews >= 100); GOT(ACH_PERFECT, p->perfects > 0);
    GOT(ACH_N5, p->exam_score[YG_NEXAMS - 1] >= PASS_MARK);
#undef GOT
}

/* ---- quizzes ---- */
static unsigned int rng = 0x5EED1234u;
void yg_seed(unsigned int s) { rng = s ? s : 1; }
unsigned int yg_rand(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

int day_romaji_mode(int day) { return day < YG_NDAYS && YG_DAYS[day].stage <= 1; }

/* What a question uses from a card: front as text, reading, or meaning */
static const char *answer_text(const YgCard *c, int use_reading, int romaji) {
    if (c->type == 'h' || c->type == 'k') return c->romaji;
    if (use_reading) return romaji ? c->romaji : c->reading;
    return c->en;
}
/* Distractors come from cards taught up to now, of the same type, with different answer text */
static void fill_choices(Question *q, int card, const char *right, int use_reading, int use_front, int romaji) {
    const YgCard *c = &YG_CARDS[card];
    int slot = yg_rand() % 4, got = 1, tries;
    q->choices[slot] = right; q->answer = (short)slot;
    for (tries = 0; tries < 600 && got < 4; tries++) {
        const YgCard *o = &YG_CARDS[yg_rand() % YG_NCARDS];
        const char *t; int i, dup = 0;
        if (o->type != c->type || o->day < 0) continue;
        if (tries < 300 && (o->day > c->day + 2 || o->day < c->day - 8)) continue;
        t = use_front ? (romaji ? o->romaji : o->front) : answer_text(o, use_reading, romaji);
        for (i = 0; i < 4; i++) if ((got > 0) && q->choices[i] && ys_cmp(q->choices[i], t) == 0) dup = 1;
        if (dup) continue;
        { int s = 0; while (q->choices[s]) s++; q->choices[s] = t; got++; }
    }
    { int s; for (s = 0; s < 4; s++) if (!q->choices[s]) q->choices[s] = "---"; }
}
void q_make(Question *q, int card, int pref, int romaji) {
    const YgCard *c = &YG_CARDS[card]; int s;
    for (s = 0; s < 4; s++) q->choices[s] = 0;
    q->card = (short)card; q->kind = Q_CHOICE; q->sub = ""; q->want = q->want2 = ""; q->tag = "";
    if (c->type == 'g') {
        int pos = yg_rand() % 4, i = 0, k;
        const char *wr[3]; wr[0] = c->w1; wr[1] = c->w2; wr[2] = c->w3;
        q->prompt = c->sentence; q->sub = c->trans; q->tag = "Fill in the blank";
        for (k = 0; k < 4; k++) q->choices[k] = (k == pos) ? c->reading : wr[i++];
        q->answer = (short)pos;
        return;
    }
    if (c->type == 'h' || c->type == 'k') {
        if (pref == 0) { q->prompt = c->front; q->tag = "How is this read?"; fill_choices(q, card, c->romaji, 0, 0, 0); }
        else if (pref == 1) { q->prompt = c->romaji; q->tag = c->type == 'h' ? "Which hiragana?" : "Which katakana?"; fill_choices(q, card, c->front, 0, 1, 0); }
        else { q->prompt = c->front; q->kind = Q_TYPE; q->tag = "Type the sound"; q->want = c->romaji; q->want2 = c->reading; }
        return;
    }
    if (c->type == 'j') {
        if (pref == 0) { q->prompt = c->front; q->tag = "What does it mean?"; fill_choices(q, card, c->en, 0, 0, 0); }
        else if (pref == 1) { q->prompt = c->en; q->tag = "Which kanji?"; fill_choices(q, card, c->front, 0, 1, 0); }
        else { q->prompt = c->front; q->tag = "Choose the reading"; fill_choices(q, card, c->reading, 1, 0, 0); }
        return;
    }
    if (pref == 0) { q->prompt = romaji ? c->romaji : c->front; q->tag = "What does it mean?"; if (!romaji && c->front != c->reading && ys_cmp(c->front, c->reading)) q->sub = c->reading; fill_choices(q, card, c->en, 0, 0, 0); }
    else if (pref == 1) { q->prompt = c->en; q->tag = "Which word?"; fill_choices(q, card, romaji ? c->romaji : c->front, 0, 1, romaji); }
    else { q->prompt = c->en; q->kind = Q_TYPE; q->tag = romaji ? "Type it in romaji" : "Type the reading"; q->want = c->reading; q->want2 = c->romaji; }
}
void quiz_new(Quiz *z) { z->n = 0; }
void quiz_add_cards(Quiz *z, const short *cards, int n, int romaji, int per) {
    int i, k;
    for (i = 0; i < n; i++) for (k = 0; k < per && z->n < YG_MAXQ; k++) q_make(&z->q[z->n++], cards[i], (int)(yg_rand() % 3), romaji);
}
void quiz_shuffle(Quiz *z) {
    int i;
    for (i = z->n - 1; i > 0; i--) {
        int j = yg_rand() % (i + 1); Question t = z->q[i]; z->q[i] = z->q[j]; z->q[j] = t;
    }
}
/* Warm-up always brings fundamentals back: yesterday, plus kana and early words from long ago */
void quiz_warmup(Quiz *z, const Profile *p) {
    int rom = day_romaji_mode(p->day), i, tries;
    quiz_new(z);
    if (p->day > 0) { const YgDay *d = &YG_DAYS[p->day - 1]; quiz_add_cards(z, d->cards, d->nnew < 5 ? d->nnew : 5, rom, 1); }
    for (i = 0, tries = 0; i < 3 && tries < 500; tries++) {
        int c = yg_rand() % YG_NCARDS;
        if (p->cards[c].seen && YG_CARDS[c].day >= 0 && YG_CARDS[c].day < p->day) {
            if (rom && (YG_CARDS[c].type == 'h' || YG_CARDS[c].type == 'k')) continue;
            q_make(&z->q[z->n++], c, (int)(yg_rand() % 3), rom); i++;
        }
    }
    quiz_shuffle(z);
}
void quiz_day(Quiz *z, const Profile *p, int day) {
    const YgDay *d = &YG_DAYS[day]; int rom = day_romaji_mode(day);
    (void)p;
    quiz_new(z);
    quiz_add_cards(z, d->cards, d->nnew > 16 ? 16 : d->nnew, rom, 1);
    if (d->nnew > 16) { /* big kana days: sample the rest instead of a 60-question slog */
        int i; for (i = 0; i < 4; i++) { int c = d->cards[16 + yg_rand() % (d->nnew - 16)]; q_make(&z->q[z->n++], c, (int)(yg_rand() % 3), rom); }
    }
    { int back; for (back = day - 1; back >= 0 && z->n < 6; back--) { const YgDay *y = &YG_DAYS[back]; quiz_add_cards(z, y->cards, y->nnew < 4 ? y->nnew : 4, rom, 1); } }
    quiz_shuffle(z);
}
/* An exam samples everything in its stage, and a little from earlier stages */
void quiz_exam(Quiz *z, int exam) {
    const YgExam *e = &YG_EXAMS[exam];
    int want = e->stars >= 5 ? 40 : e->stars >= 3 ? 25 : 15, stage = YG_DAYS[e->day].stage, rom = stage <= 1, tries;
    int first = (e->stars >= 5) ? 0 : YG_STAGES[stage].first;
    static unsigned char used[YG_NCARDS]; int i;
    for (i = 0; i < YG_NCARDS; i++) used[i] = 0;
    quiz_new(z);
    for (tries = 0; tries < 6000 && z->n < want; tries++) {
        int c = yg_rand() % YG_NCARDS, day = YG_CARDS[c].day;
        if (day < 0 || day > e->day || used[c]) continue;
        if (day < first && (yg_rand() % 8)) continue;
        if (rom && (YG_CARDS[c].type == 'h' || YG_CARDS[c].type == 'k')) continue;
        used[c] = 1;
        q_make(&z->q[z->n++], c, (int)(yg_rand() % 3), rom);
    }
    quiz_shuffle(z);
}
void quiz_review(Quiz *z, const Profile *p, int today, int max) {
    int i;
    quiz_new(z);
    for (i = 0; i < YG_NCARDS && z->n < max; i++) {
        const CardState *c = &p->cards[i];
        if (c->seen && c->due <= today) {
            int rom = YG_CARDS[i].day >= 0 && day_romaji_mode(YG_CARDS[i].day) && day_romaji_mode(p->day);
            q_make(&z->q[z->n++], i, (int)(yg_rand() % 3), rom);
        }
    }
    quiz_shuffle(z);
}
/* Genki practice: YomiGo's own cards, grouped by the lesson of the book that covers them */
void quiz_genki(Quiz *z, int index) {
    const YgGenki *g = &YG_GENKI[index];
    quiz_new(z);
    quiz_add_cards(z, g->cards, g->ncards, 0, 1);
    quiz_shuffle(z);
    if (z->n > 15) z->n = 15;
}
/* Placement walks up the stages. Each section samples what that stage teaches. */
int placement_section(Quiz *z, int section) {
    static const char types[] = "hkwgj";
    int tries;
    if (section >= 5) return 0;
    quiz_new(z);
    for (tries = 0; tries < 5000 && z->n < 5; tries++) {
        int c = yg_rand() % YG_NCARDS, j, dup = 0;
        if (YG_CARDS[c].type != types[section] || YG_CARDS[c].day < 0) continue;
        for (j = 0; j < z->n; j++) if (z->q[j].card == c) dup = 1;
        if (dup) continue;
        q_make(&z->q[z->n++], c, section < 2 ? 0 : 0, 0);
    }
    return 1;
}
