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
test("lookup finds words and undoes polite endings", () => {
  assert.equal(Y.lookup("猫がいます")[0].card.id, "v.neko");
  assert.ok(Y.lookup("食べました").some((h) => h.card.id === "v.tabemasu"));
  assert.ok(Y.lookup("飲みません").some((h) => h.card.id === "v.nomimasu"));
  assert.equal(Y.lookup("xyz").length, 0);
});
