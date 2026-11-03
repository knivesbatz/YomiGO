// JS core tests: the same vectors as the C core, plus a whole-course walk and the extension's lookup.
import assert from "node:assert/strict";
import fs from "node:fs";
import { test } from "node:test";

const root = new URL("..", import.meta.url);
const C = JSON.parse(fs.readFileSync(new URL("build/gen/content.json", root), "utf8"));
globalThis.YG_CONTENT = C;
await import(new URL("shared/core.js", root));
const Y = globalThis.YG;
const V = JSON.parse(fs.readFileSync(new URL("tests/vectors.json", root), "utf8"));

test("kana normalising matches the C core", () => {
  for (const [a, b] of V.norm) assert.equal(Y.normKana(a), b, a);
});
test("spaced repetition matches the C core", () => {
  for (const s of V.srs) {
    const c = { reps: s.start[0], ivl: s.start[1], ease: s.start[2], lapses: 0, due: 0 };
    Y.srsGrade(c, s.grade, 100);
    assert.deepEqual([c.ivl, c.ease, c.reps, c.due], [s.ivl, s.ease, s.reps, 100 + s.ivl]);
  }
});
test("save round trip, and damaged files", () => {
  const p = Y.newProfile("Maya");
  p.xp = 77; p.day = 5; p.exams.S0 = 90; p.cards["h.a"] = Y.srsNew(10);
  assert.deepEqual(Y.loadText(Y.saveText(p)), p);
  assert.equal(Y.loadText("nonsense"), null);
  const d = Y.loadText("YOMIGO-SAVE 1\nname=x\nday=99999\npace=9\nstep=77\ncard nope 1 2 3 4 5\ncard h.a 1 999999 3 1 5\n");
  assert.equal(d.day, C.days.length); assert.equal(d.pace, 3); assert.equal(d.step, 0);
  assert.equal(d.cards["h.a"].ivl, 3650); assert.equal(d.cards.nope, undefined);
});
test("chat replies are all reachable", () => {
  for (const c of Object.values(C.chats)) for (const t of c.turns) for (const w of t.want) assert.ok(Y.chatOk(w, t), w);
  const t = C.chats.eat1.turns[0];
  assert.ok(Y.chatOk("はい、食べます", t)); assert.ok(Y.chatOk("hai tabemasu", t)); assert.ok(!Y.chatOk("いいえ", t));
});
test("every card makes a sound question", () => {
  for (const c of C.cards) if (c.day >= 0) for (const pref of [0, 1, 2]) {
    const q = Y.makeQ(c, pref, false);
    assert.ok(q.prompt, c.id);
    if (q.kind === "choice") { assert.equal(q.choices.length, 4, c.id); assert.equal(new Set(q.choices).size, 4, c.id); assert.ok(q.choices.includes(q.answer), c.id); }
    else assert.ok(q.want, c.id);
  }
});
test("the whole course can be walked, and exams gate it", () => {
  const p = Y.newProfile("Walker"); let t = 1000;
  for (let guard = 0; guard < 500 && (p.day < C.days.length || Y.examGate(p) >= 0); guard++) {
    const g = Y.examGate(p);
    if (g >= 0) {
      assert.ok(Y.quizExam(g).length >= 15);
      assert.equal(Y.examResult(p, g, 50), false);
      assert.equal(Y.examGate(p), g);
      assert.equal(Y.examResult(p, g, 85), true);
      continue;
    }
    assert.ok(Y.quizDay(p.day).length >= 4, "day " + p.day);
    Y.learnDay(p, p.day, t); Y.completeDay(p, t); t++;
  }
  assert.equal(p.day, C.days.length); assert.equal(p.streak, C.days.length); assert.equal(Y.rankOf(p), C.exams.length);
});
test("every Genki lesson has practice", () => {
  C.genki.forEach((g, i) => { const n = Y.quizGenki(i).length; assert.ok(n >= 3 && n <= 15, "lesson " + g.lesson); });
  const p = Y.newProfile("G"); p.genki = 1;
  assert.equal(Y.loadText(Y.saveText(p)).genki, 1);
});
test("the dictionary finds words, in any form", () => {
  const n = Y.dictLoad(fs.readFileSync(new URL("extension/dict.tsv", root), "utf8"));
  assert.ok(n > 20000, "entries " + n);
  for (const q of ["ねこ", "neko", "猫", "cat", "NEKO"]) assert.ok(Y.dictSearch(q).some((r) => r.word === "猫"), q);
  assert.equal(Y.dictSearch("コーヒー")[0].reading, "コーヒー");
  assert.equal(Y.dictSearch("zzqqxx").length, 0);
  for (const [text, word] of [["食べました", "食べる"], ["猫がいます", "猫"], ["行って", "行く"], ["行きました", "行く"], ["来ます", "来る"], ["高くない", "高い"], ["飲まない", "飲む"], ["します", "する"], ["読みたい", "読む"], ["見た", "見る"]])
    assert.ok(Y.dictLookup(text).some((h) => h.card.f === word), text + " -> " + word);
});
test("the entrance exam starts with romaji", () => {
  for (const q of Y.placementSection(0)) assert.ok(/^[\x00-\x7f]+$/.test(q.prompt), q.prompt);
  for (let i = 0; i < 6; i++) assert.equal(Y.placementSection(i).length, 5, "section " + i);
});
test("a word spelled the same never supplies a wrong answer", () => {
  const toki = C.cards.find((c) => c.id === "v.toki"), ji = C.cards.find((c) => c.id === "v.ji");
  for (let i = 0; i < 200; i++) assert.ok(!Y.makeQ(toki, 0, false).choices.includes(ji.en));
});
test("your own words: same save format as C, adding, import, review, damage", () => {
  const b = Y.loadText(V.myline);
  assert.equal(b.my.length, 2);
  assert.deepEqual(b.my[0], { word: "猫", reading: "ねこ", gloss: "cat", sent: "猫がいます", reps: 1, ivl: 2, lapses: 3, ease: 2000, due: 5 });
  assert.equal(b.my[1].sent, "");
  const a = Y.newProfile("M");
  assert.equal(Y.myAdd(a, "犬", "いぬ", "dog", "犬がいます", 100), 1);
  assert.equal(Y.myAdd(a, "犬", "いぬ", "dog again", "", 100), 0);
  assert.equal(Y.myAdd(a, "", "", "nothing", "", 100), -1);
  assert.equal(Y.myAdd(a, "かな", "", "kana only", "", 100), 1);
  assert.equal(a.my[1].reading, "かな");
  assert.equal(Y.myImport(a, "#separator:tab\n#columns:Word\n猫\tねこ\tcat\t猫だ\tsite.com\n犬\tいぬ\tdog\n\n鳥\tとり\tbird\n", 100), 2);
  assert.ok(Y.myExport(a).includes("鳥\tとり\tbird\t"));
  assert.deepEqual(Y.loadText(Y.saveText(a)).my, a.my);
  assert.equal(Y.dueCount(a, 100), a.my.length);
  assert.equal(Y.dueCount(a, 99), 0);
  const qs = Y.quizReview(a, 100, 30);
  assert.equal(qs.length, a.my.length);
  for (const q of qs) { assert.ok(q.my); if (q.kind === "choice") { assert.equal(new Set(q.choices).size, 4); assert.ok(q.choices.includes(q.answer)); } else assert.ok(q.want); }
  assert.equal(Y.loadText("YOMIGO-SAVE 1\nname=x\nmy 1 2 3\nmy\nmy 1 2 3 4 5\t\t\t\t\n").my.length, 0);
});
test("listening has something real to hear on every class", () => {
  C.days.forEach((d, i) => {
    const qs = Y.quizListen(i);
    assert.ok(qs.length > 0 || i === 0, "day " + i);
    for (const q of qs) { assert.ok(q.say && q.prompt, "day " + i); if (q.kind === "choice") { assert.equal(new Set(q.choices).size, 4, "day " + i + " " + q.say); assert.ok(q.choices.includes(q.answer)); } else assert.ok(q.want); }
    for (const l of Y.shadowLines(i)) { assert.ok(l.jp && l.en); assert.ok(!l.jp.includes("＿")); }
  });
});
test("finding words in a line matches the C core", () => {
  Y.dictLoad(fs.readFileSync(new URL("extension/dict.tsv", root), "utf8"));
  for (const [text, word] of V.lookup) assert.ok(Y.dictLookup(text).some((h) => h.card.f === word), text + " -> " + word);
});
test("lookup finds words and undoes polite endings", () => {
  assert.equal(Y.lookup("猫がいます")[0].card.id, "v.neko");
  assert.ok(Y.lookup("食べました").some((h) => h.card.id === "v.tabemasu"));
  assert.ok(Y.lookup("飲みません").some((h) => h.card.id === "v.nomimasu"));
  assert.equal(Y.lookup("xyz").length, 0);
});
