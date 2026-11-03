/* YomiGo shared core: content, spaced repetition, saves, quizzes. No OS calls and no C runtime. */
#ifndef YG_H
#define YG_H

typedef struct { const char *id; char type; const char *front, *reading, *romaji, *en, *note, *sentence, *w1, *w2, *w3, *trans; int day; } YgCard;
typedef struct { const char *name, *ja, *desc; int first; } YgStage;
typedef struct { int stage; const char *title, *ja; int nnotes; const char *const *notes; int nnew; const short *cards; int chat; const char *hw, *imm, *mat; int exam, read; } YgDay;
typedef struct { const char *id, *name; int stars, day; } YgExam;
typedef struct { const char *say, *romaji, *en; int nwant; const char *const *want; const char *hint, *want_ro; } YgTurn;
typedef struct { const char *title, *ja, *who; int nturns; const YgTurn *turns; } YgChat;
typedef struct { int stage, is_new; const char *type, *title, *ja; int year; const char *task; } YgMedia;
typedef struct { const char *cat, *name, *url, *desc; } YgRes;
typedef struct { const char *a, *b; } YgPair;
typedef struct { const char *title, *ja, *jp, *en; int nq; const char *const *q; } YgRead;
typedef struct { int lesson; const char *topics; int ncards; const short *cards; } YgGenki;

#include "content_gen.h"

/* Spaced repetition: integer SM-2. Same rules as shared/core.js, checked by tests/vectors.json. */
typedef struct { unsigned char seen; short reps, ivl, lapses, ease; int due; } CardState;
enum { GRADE_AGAIN, GRADE_HARD, GRADE_GOOD, GRADE_EASY };
void srs_new(CardState *c, int today);
void srs_grade(CardState *c, int grade, int today);

/* Words you add yourself. They review alongside the course's cards. */
#define YG_MY_MAX 2000
typedef struct { char word[40], reading[60], gloss[92], sent[120]; CardState st; } MyCard;

/* Profile */
#define YG_NAME 32
enum { STEP_WARMUP, STEP_TEACH, STEP_TYPING, STEP_DRILL, STEP_CHAT, STEP_READ, STEP_QUIZ, STEP_HOMEWORK, STEP_IMMERSE, STEP_DONE };
typedef struct {
    char name[YG_NAME];
    int xp, streak, best_streak, last_day, done_today, placed;
    int day, step;          /* the next class, and how far through it */
    int pace;               /* classes allowed per real day: 1, 2 or 3 */
    int force_en;           /* keep English labels even after immersion starts */
    int genki;              /* offer practice by Genki lesson */
    int reviews, perfects, chats, homeworks, reads;
    unsigned char exam_score[YG_NEXAMS];
    unsigned int ach;
    CardState cards[YG_NCARDS];
    int nmy;
    MyCard my[YG_MY_MAX];
} Profile;

void profile_new(Profile *p, const char *name);
int  profile_save_text(const Profile *p, char *out, int cap);
int  profile_load_text(Profile *p, const char *text, int len);
int  profile_stage(const Profile *p);
int  profile_due(const Profile *p, int today);
int  profile_exam_gate(const Profile *p);    /* exam that must be passed before the next class, or -1 */
int  profile_course_done(const Profile *p);
void profile_touch_day(Profile *p, int today);
void profile_learn_day(Profile *p, int day, int today);
void profile_complete_day(Profile *p, int today);
int  profile_exam_pass(Profile *p, int exam, int score, int today);
int  profile_place(Profile *p, int stage_reached, int today);
int  profile_rank(const Profile *p);
const char *rank_name(int rank, int ja);
int  rank_count(void);

enum { ACH_FIRST_CLASS, ACH_STREAK3, ACH_STREAK7, ACH_STREAK30, ACH_HIRAGANA, ACH_KATAKANA, ACH_FIRST_CHAT, ACH_REVIEW100, ACH_PERFECT, ACH_N5, ACH_READER, ACH_N4, ACH_MINER, ACH_COUNT };
typedef struct { const char *en, *ja, *desc; } YgAch;
extern const YgAch YG_ACH[ACH_COUNT];
void ach_check(Profile *p);
int  my_add(Profile *p, const char *word, const char *reading, const char *gloss, const char *sent, int today); /* 1 added, 0 already there, -1 full or incomplete */
void my_delete(Profile *p, int k);
int  my_import(Profile *p, const char *tsv, int today);      /* lines of word, reading, meaning, and an optional sentence */
int  my_export(const Profile *p, char *out, int cap);
#define YG_SAVE_CAP 900000

/* Quiz building */
#define YG_MAXQ 60
enum { Q_CHOICE, Q_TYPE };
typedef struct {
    short card, kind;       /* kind: Q_CHOICE or Q_TYPE */
    const char *prompt, *sub, *tag;
    const char *choices[4];
    short answer;           /* index for Q_CHOICE */
    const char *want, *want2;   /* accepted text for Q_TYPE */
} Question;
typedef struct { Question q[YG_MAXQ]; int n; } Quiz;

unsigned int yg_rand(void);
void yg_seed(unsigned int s);
void q_make(Question *q, int card, int kind_pref, int romaji_mode);
void q_make_my(Question *q, const Profile *p, int k, int kind_pref);  /* its card number is YG_NCARDS + k */
void quiz_new(Quiz *z);
void quiz_add_cards(Quiz *z, const short *cards, int n, int romaji_mode, int per_card);
void quiz_shuffle(Quiz *z);
void quiz_warmup(Quiz *z, const Profile *p);
void quiz_day(Quiz *z, const Profile *p, int day);
void quiz_exam(Quiz *z, int exam);
void quiz_review(Quiz *z, const Profile *p, int today, int max);
void quiz_genki(Quiz *z, int index);
int  placement_section(Quiz *z, int section);   /* builds 5 questions; 0 when no sections are left */
int  day_romaji_mode(int day);

/* Text */
int  ys_len(const char *s);
int  ys_cmp(const char *a, const char *b);
int  ys_ncmp(const char *a, const char *b, int n);
void ys_cpy(char *d, const char *s, int cap);
void ys_cat(char *d, const char *s, int cap);
int  ys_itoa(int v, char *d);
int  ys_atoi(const char *s);
int  ys_contains_ci(const char *hay, const char *needle);
void ys_fmt(char *d, int cap, const char *f, int a, int b);
int  utf8_next(const char **s);
int  utf8_put(int cp, char *d);
void norm_kana(const char *in, char *out, int cap);     /* kana, katakana, kanji, or romaji becomes hiragana */
int  answer_ok(const char *input, const char *want, const char *romaji);
int  chat_ok(const char *input, const YgTurn *t);
int  dict_search(const char *q, int *out, int max);
void dict_row(int off, char *word, char *reading, char *gloss, int cap);

#endif
