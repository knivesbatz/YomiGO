/* Core self-test. Built with the normal C runtime. Prints every failure and exits non-zero. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/yg.h"
#include "vectors_gen.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static Profile a, b;
static char buf[YG_SAVE_CAP];

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
    CHECK(answer_ok("konnichiwa", "こんにちは", ""), "a final は typed as said, with no romaji to compare");
    CHECK(!answer_ok("wa", "は", ""), "a lone は is still ha");
    { static const char *const kr[][2] = {{"きって", "kitte"}, {"まっちゃ", "matcha"}, {"ほん", "hon"}, {"きんようび", "kin'youbi"}, {"コーヒー", "koohii"},
                                         {"こんにちは", "konnichiwa"}, {"はは", "haha"}, {"ちゃわん", "chawan"}, {"ファイル", "fairu"}};
      int k; char ro[64];
      for (k = 0; k < 9; k++) { kana_romaji(kr[k][0], ro, sizeof ro); CHECK(!ys_cmp(ro, kr[k][1]), "romaji %d", k); } }

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
            /* a word spelled like this card must not supply a wrong choice */
            if (q.kind == Q_CHOICE && YG_CARDS[i].type != 'g') { int c, o; for (c = 0; c < 4; c++) for (o = 0; o < YG_NCARDS; o++)
                if (o != i && YG_CARDS[o].type == YG_CARDS[i].type && !strcmp(YG_CARDS[o].front, YG_CARDS[i].front) && !strcmp(q.choices[c], YG_CARDS[o].en) && strcmp(YG_CARDS[o].en, YG_CARDS[i].en))
                    CHECK(0, "card %s offers %s", YG_CARDS[i].id, q.choices[c]); }
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
    /* Finding words in a line, with endings undone: the same cases as the JS core */
    for (i = 0; i < (int)(sizeof LOOKUP / sizeof LOOKUP[0]); i++) { int r[8], len, m, got = lookup_at(LOOKUP[i].text, 0, r, 8, &len), hit = 0; char w[128], rd[128], g[128];
        for (m = 0; m < got; m++) { dict_row(r[m], w, rd, g, 128); if (!strcmp(w, LOOKUP[i].word) || (!*w && !strcmp(rd, LOOKUP[i].word))) hit = 1; }
        CHECK(hit, "lookup %s should find %s (%d results)", LOOKUP[i].text, LOOKUP[i].word, got); CHECK(len > 0, "lookup %s length", LOOKUP[i].text); }
    { int r[8], len; const char *t = "今日は猫を見た"; CHECK(lookup_at(t, 9, r, 8, &len) > 0 && len == 1, "a word mid-line"); CHECK(lookup_at("zz", 0, r, 8, &len) == 0, "nothing for latin"); }
    /* Listening: something to hear on every class that teaches something, with a real answer */
    for (i = 0; i < YG_NDAYS; i++) { int k, c; ShadowLine sl[4]; int ns = shadow_lines(i, sl, 4);
        quiz_listen(&z, i, 34);
        for (k = 0; k < z.n; k++) if (YG_CARDS[z.q[k].card].type == 'g') for (c = 0; c < 4; c++) CHECK((int)strlen(z.q[k].choices[c]) <= 34 && strcmp(z.q[k].choices[c], "---"), "TV listening choice %s", z.q[k].choices[c]);
        quiz_listen(&z, i, 0);
        CHECK(z.n > 0 || i == 0, "day %d has no listening", i);
        for (k = 0; k < z.n; k++) { const Question *q = &z.q[k]; CHECK(*q->say, "day %d q%d says nothing", i, k); CHECK(*q->prompt, "day %d q%d no reveal", i, k);
            if (q->kind == Q_CHOICE) { for (c = 0; c < 4; c++) CHECK(q->choices[c] && strcmp(q->choices[c], "---"), "day %d q%d blank choice", i, k); } else CHECK(*q->want, "day %d q%d no want", i, k); }
        for (k = 0; k < ns; k++) { CHECK(*sl[k].jp && !strstr(sl[k].jp, "＿"), "day %d shadow line %d: %s", i, k, sl[k].jp); CHECK(*sl[k].en, "day %d shadow en", i); } }
    /* Your own words: same save format as the JS core, adding, duplicates, import, review, damage */
    CHECK(profile_load_text(&b, MYLINE, (int)strlen(MYLINE)), "my line loads");
    CHECK(b.nmy == 2 && !strcmp(b.my[0].word, "猫") && !strcmp(b.my[0].reading, "ねこ") && !strcmp(b.my[0].gloss, "cat") && !strcmp(b.my[0].sent, "猫がいます"), "my fields %d", b.nmy);
    CHECK(b.my[0].st.reps == 1 && b.my[0].st.ivl == 2 && b.my[0].st.lapses == 3 && b.my[0].st.ease == 2000 && b.my[0].st.due == 5, "my state");
    CHECK(!strcmp(b.my[1].word, "だめ") && !b.my[1].sent[0], "my empty sentence");
    profile_new(&a, "M");
    CHECK(my_add(&a, "犬", "いぬ", "dog", "犬がいます", 100) == 1, "add");
    CHECK(my_add(&a, "犬", "いぬ", "dog again", "", 100) == 0, "duplicate refused");
    CHECK(my_add(&a, "", "", "nothing", "", 100) == -1, "no word refused");
    CHECK(my_add(&a, "かな", "", "kana only", "", 100) == 1 && !strcmp(a.my[1].reading, "かな"), "reading defaults to the word");
    CHECK(my_add(&a, "長", "ながいながいながいながいながいながいながいながいながいながいながいながいながいながいながいながい", "long", "", 100) == 1, "long reading");
    { const unsigned char *r = (const unsigned char *)a.my[2].reading; int l = (int)strlen((const char *)r); CHECK(l % 3 == 0, "cut mid-character at %d", l); }
    CHECK(my_import(&a, "#separator:tab\n#columns:Word\n猫\tねこ\tcat\t猫だ\tsite.com\n犬\tいぬ\tdog\n\n鳥\tとり\tbird\n", 100) == 2, "import skips headers, duplicates, and blanks");
    n = my_export(&a, buf, sizeof buf); CHECK(n > 0 && strstr(buf, "鳥\tとり\tbird\t\n"), "export");
    n = profile_save_text(&a, buf, sizeof buf); CHECK(n > 0 && profile_load_text(&b, buf, n) && b.nmy == a.nmy, "my round trip %d %d", b.nmy, a.nmy);
    CHECK(!memcmp(b.my, a.my, sizeof(MyCard) * 1) || !strcmp(b.my[0].sent, a.my[0].sent), "my data");
    CHECK(profile_due(&a, 100) == a.nmy, "new words are due"); CHECK(profile_due(&a, 99) == 0, "not before they're added");
    quiz_review(&z, &a, 100, 30); CHECK(z.n == a.nmy, "review has them, %d", z.n);
    for (i = 0; i < z.n; i++) { CHECK(z.q[i].card >= YG_NCARDS, "my card number"); if (z.q[i].kind == Q_CHOICE) { int c, d; for (c = 0; c < 4; c++) { CHECK(z.q[i].choices[c] != 0, "my blank choice"); for (d = 0; d < c; d++) CHECK(strcmp(z.q[i].choices[c], z.q[i].choices[d]), "my dup choice"); } } else CHECK(*z.q[i].want, "my want"); }
    { int was = a.nmy; my_delete(&a, 0); CHECK(a.nmy == was - 1 && !strcmp(a.my[0].word, "かな"), "delete"); my_delete(&a, 999); CHECK(a.nmy == was - 1, "bad delete ignored"); }
    { static const char t[] = "YOMIGO-SAVE 1\nname=x\nmy 1 2 3\nmy\nmy 1 2 3 4 5\t\t\t\t\n"; CHECK(profile_load_text(&b, t, (int)strlen(t)) && b.nmy == 0, "damaged my lines dropped, %d", b.nmy); }
    /* Reading: right answers are distinct from wrong ones, and every passage has a class */
    { int r, q, d; for (r = 0; r < YG_NREAD; r++) { const YgRead *x = &YG_READ[r]; int used = 0;
        for (q = 0; q < x->nq; q++) CHECK(strcmp(x->q[q * 4 + 1], x->q[q * 4 + 2]) && strcmp(x->q[q * 4 + 1], x->q[q * 4 + 3]), "reading %s q%d", x->title, q);
        for (d = 0; d < YG_NDAYS; d++) if (YG_DAYS[d].read == r) used = 1;
        CHECK(used, "reading %s has no class", x->title); } }
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
      CHECK(a.ach & (1u << ACH_N5), "N5 achievement"); CHECK(a.ach & (1u << ACH_N4), "N4 achievement");
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
