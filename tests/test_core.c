/* Core self-test. Built with the normal C runtime. Prints every failure and exits non-zero. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/yg.h"
#include "vectors_gen.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static Profile a, b;
static char buf[200000];

int main(void) {
    int i, n;
    static Quiz z;
    for (i = 0; i < (int)(sizeof NORM / sizeof NORM[0]); i++) {
        char o[256]; norm_kana(NORM[i].a, o, sizeof o);
        CHECK(!strcmp(o, NORM[i].b), "norm %s -> %s, want %s", NORM[i].a, o, NORM[i].b);
    }
    for (i = 0; i < (int)(sizeof SRS / sizeof SRS[0]); i++) {
        CardState c; srs_new(&c, 100); c.reps = (short)SRS[i].r; c.ivl = (short)SRS[i].iv; c.ease = (short)SRS[i].e;
        srs_grade(&c, SRS[i].g, 100);
        CHECK(c.ivl == SRS[i].wi && c.ease == SRS[i].we && c.reps == SRS[i].wr, "srs %d: ivl %d ease %d reps %d", i, c.ivl, c.ease, c.reps);
        CHECK(c.due == 100 + c.ivl, "srs due %d", i);
    }
    /* Answer checks people will actually hit */
    CHECK(answer_ok("konnichiwa", "こんにちは", "konnichiwa"), "wa spelling accepted");
    CHECK(answer_ok("こんにちは", "こんにちは", "konnichiwa"), "kana accepted");
    CHECK(!answer_ok("", "あ", "a"), "empty rejected");
    CHECK(!answer_ok("ka", "あ", "a"), "wrong rejected");
    CHECK(answer_ok("kin'youbi", "きんようび", "kin'youbi"), "apostrophe");

    /* Every chat reply must be reachable, typed in romaji and in kana with kanji */
    for (i = 0; i < YG_NCHATS; i++) { int t, w; for (t = 0; t < YG_CHATS[i].nturns; t++) for (w = 0; w < YG_CHATS[i].turns[t].nwant; w++)
        CHECK(chat_ok(YG_CHATS[i].turns[t].want[w], &YG_CHATS[i].turns[t]), "chat %d turn %d want %d", i, t, w); }
    { const YgTurn *t = &YG_CHATS[8].turns[0];
      CHECK(chat_ok("はい、食べます", t), "kanji reply"); CHECK(chat_ok("hai tabemasu", t), "romaji reply"); CHECK(!chat_ok("いいえ", t), "bare no is not enough here"); }

    /* Save round trip, including a card from each type */
    profile_new(&a, "Maya"); a.xp = 1234; a.day = 12; a.step = 3; a.pace = 2; a.genki = 1; a.streak = 4; a.exam_score[0] = 90; a.ach = 5;
    for (i = 0; i < YG_NCARDS; i += 7) { srs_new(&a.cards[i], 50); srs_grade(&a.cards[i], GRADE_GOOD, 50); }
    n = profile_save_text(&a, buf, sizeof buf);
    CHECK(n > 0, "save size");
    CHECK(profile_load_text(&b, buf, n), "load");
    CHECK(!strcmp(b.name, "Maya") && b.xp == 1234 && b.day == 12 && b.step == 3 && b.pace == 2 && b.genki == 1 && b.streak == 4 && b.exam_score[0] == 90 && b.ach == 5, "profile fields");
    CHECK(!memcmp(a.cards, b.cards, sizeof a.cards), "card states");
    /* Garbage and damaged files must fail or clamp, never crash */
    CHECK(!profile_load_text(&b, "not a save", 10), "garbage rejected");
    { const char *t = "YOMIGO-SAVE 1\nname=x\nday=99999\nstep=77\npace=9\ncard nonsense 1 2 3 4 5\ncard h.a 1 999999 3 1 5\n";
      CHECK(profile_load_text(&b, t, (int)strlen(t)), "damaged loads");
      CHECK(b.day == YG_NDAYS && b.step == 0 && b.pace == 3, "clamped %d %d %d", b.day, b.step, b.pace); }

    /* Questions: correct answer present once, no blanks, on every card */
    for (i = 0; i < YG_NCARDS; i++) {
        int pref; if (YG_CARDS[i].day < 0) continue;
        for (pref = 0; pref < 3; pref++) {
            Question q; q_make(&q, i, pref, 0);
            CHECK(q.prompt && *q.prompt, "card %s has empty prompt", YG_CARDS[i].id);
            if (q.kind == Q_CHOICE) { int c, d;
                for (c = 0; c < 4; c++) { CHECK(q.choices[c] && strcmp(q.choices[c], "---"), "card %s pref %d has blank choice", YG_CARDS[i].id, pref);
                    for (d = 0; d < c; d++) CHECK(strcmp(q.choices[c], q.choices[d]), "card %s dup choice %s", YG_CARDS[i].id, q.choices[c]); }
            } else CHECK(*q.want, "card %s empty want", YG_CARDS[i].id);
        }
    }
    for (i = 0; i < YG_NGENKI; i++) { quiz_genki(&z, i); CHECK(z.n >= 3 && z.n <= 15, "genki %d has %d", i, z.n); }
    /* Dictionary: reading, romaji, spelling, English, and nonsense */
    { int r[9], k, got; char w[128], rd[128], g[128];
      static const char *const qs[] = {"ねこ", "neko", "猫", "cat", "NEKO"};
      for (k = 0; k < 5; k++) { int hit = 0, m;
        got = dict_search(qs[k], r, 9); CHECK(got > 0, "dict %s empty", qs[k]);
        for (m = 0; m < got; m++) { dict_row(r[m], w, rd, g, 128); if (!strcmp(w, "猫")) hit = 1; }
        CHECK(hit, "dict %s doesn't find the cat", qs[k]); }
      got = dict_search("コーヒー", r, 9); CHECK(got > 0, "dict katakana"); dict_row(r[0], w, rd, g, 128); CHECK(!strcmp(rd, "コーヒー"), "katakana first hit %s", rd);
      CHECK(dict_search("zzqqxx", r, 9) == 0, "nonsense finds nothing"); CHECK(dict_search("", r, 9) == 0, "empty finds nothing"); }
    /* Course flow: walk the whole thing as a learner would */
    profile_new(&a, "Walker");
    { int today = 1000, guard = 0;
      CHECK(profile_exam_gate(&a) < 0, "no gate at start");
      while (!profile_course_done(&a) && guard++ < 500) {
          int g = profile_exam_gate(&a);
          if (g >= 0) {
              CHECK(!profile_exam_pass(&a, g, 50, today), "failing exam must not pass");
              CHECK(profile_exam_gate(&a) == g, "fail keeps the gate shut");
              CHECK(profile_exam_pass(&a, g, 85, today), "exam %d pass", g);
              continue;
          }
          quiz_warmup(&z, &a); quiz_day(&z, &a, a.day);
          CHECK(z.n >= 4, "day %d quiz has %d questions", a.day, z.n);
          profile_learn_day(&a, a.day, today);
          profile_complete_day(&a, today); today++;
      }
      CHECK(profile_course_done(&a), "course can be finished");
      CHECK(a.streak == YG_NDAYS, "streak %d", a.streak);
      CHECK(profile_rank(&a) == YG_NEXAMS, "rank %d", profile_rank(&a));
      CHECK(a.ach & (1u << ACH_N5), "N5 achievement");
      for (i = 0; i < YG_NEXAMS; i++) { quiz_exam(&z, i); CHECK(z.n >= 15, "exam %d has %d", i, z.n); }
    }
    /* Exam gate blocks the next class */
    profile_new(&a, "Gate");
    a.day = YG_EXAMS[0].day + 1;
    CHECK(profile_exam_gate(&a) == 0, "gate shut before exam 0 is passed");
    /* Missed a day: the streak resets */
    profile_new(&a, "S"); a.last_day = 10; a.streak = 5; a.best_streak = 5; a.day = 1;
    profile_complete_day(&a, 12);
    CHECK(a.streak == 1 && a.best_streak == 5, "streak reset %d", a.streak);
    /* Placement credits earlier exams, and cards are scheduled */
    profile_new(&a, "P");
    profile_place(&a, 3, 100);
    CHECK(a.day == YG_STAGES[3].first && profile_exam_gate(&a) < 0, "placement");
    CHECK(a.cards[YG_DAYS[0].cards[0]].seen, "placed cards known");
    for (i = 0; i < 6; i++) { CHECK(placement_section(&z, i) && z.n == 5, "placement %d has %d", i, z.n); }
    CHECK(!placement_section(&z, 6), "placement ends");
    /* Romaji always comes first: the first section shows no Japanese script */
    placement_section(&z, 0);
    for (i = 0; i < z.n; i++) { const unsigned char *p = (const unsigned char *)z.q[i].prompt; for (; *p; p++) CHECK(*p < 0x80, "romaji section has non-romaji prompt %s", z.q[i].prompt); }
    printf(fails ? "%d FAILED\n" : "core tests passed (%d failures)\n", fails);
    return fails != 0;
}
