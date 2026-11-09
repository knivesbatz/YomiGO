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
test("two devices' saves merge as on the PC", () => {
  const a = Y.newProfile("Maya"), b = Y.newProfile("Maya");
  a.xp = 50; a.last = 10; a.pace = 1; a.day = 3; a.cards["h.a"] = { reps: 1, ivl: 1, lapses: 0, ease: 2500, due: 11 };
  b.xp = 20; b.last = 12; b.pace = 3; b.day = 5; b.step = 2; b.cards["h.a"] = { reps: 3, ivl: 6, lapses: 1, ease: 2300, due: 18 }; b.cards["h.i"] = Y.srsNew(12);
  b.exams.S0 = 85; a.my.push({ word: "猫", reading: "ねこ", gloss: "cat", sent: "", reps: 0, ivl: 0, lapses: 0, ease: 2500, due: 1 });
  b.my.push({ word: "猫", reading: "ねこ", gloss: "cat", sent: "", reps: 2, ivl: 3, lapses: 0, ease: 2500, due: 9 }, { word: "犬", reading: "いぬ", gloss: "dog", sent: "", reps: 0, ivl: 0, lapses: 0, ease: 2500, due: 1 });
  const m = Y.merge(a, b);
  assert.equal(m.xp, 50); assert.equal(m.pace, 3); assert.equal(m.day, 5); assert.equal(m.step, 2); assert.equal(m.last, 12);
  assert.equal(m.cards["h.a"].reps, 3); assert.ok(m.cards["h.i"]); assert.equal(m.exams.S0, 85);
  assert.equal(m.my.length, 2); assert.equal(m.my[0].reps, 2);
});
test("a keyboard that eats an n isn't a wrong answer, and a broken streak costs XP once", () => {
  assert.ok(Y.imeSlip("ごめんあさい", "ごめんなさい", "gomen nasai"));
  assert.ok(!Y.imeSlip("ごめさい", "ごめんなさい", "gomen nasai"));
  assert.ok(!Y.imeSlip("ごめんなさい", "ごめんなさい"), "a right answer isn't a slip");
  const p = Y.newProfile("Maya"); p.xp = 500; p.streak = 6; p.last = 100;
  assert.equal(Y.streakCheck(p, 101), null, "yesterday keeps it");
  assert.deepEqual(Y.streakCheck(p, 103), { missed: 2, lost: 6, xp: 100 });
  assert.equal(p.xp, 400); assert.equal(p.streak, 0);
  assert.equal(Y.streakCheck(p, 104), null, "charged once");
  const q = Y.newProfile("Maya"); q.streak = 3; q.last = 10; q.xp = 20;
  assert.equal(Y.streakCheck(q, 30).xp, 20, "never below zero");
  const a = Y.newProfile("M"), b = Y.newProfile("M"); a.streak = 0; a.last = 50; b.streak = 9; b.last = 40;
  assert.equal(Y.merge(a, b).streak, 0, "the older device can't restore a lost streak");
  assert.equal(Y.loadText(Y.saveText(p)).broke, 103);
});
test("finding new words in pasted text", () => {
  const dict = fs.readFileSync(new URL("mobile/app/src/main/assets/www/dict.js", root), "utf8");
  Y.dictLoad(JSON.parse(dict.slice(dict.indexOf("=") + 1, dict.lastIndexOf(";"))));
  const w = Y.mineWords(["猫が好きです。", "猫は可愛い。犬もいます。"], (c) => c.f === "犬");
  assert.equal(w[0].f, "猫"); assert.equal(w[0].n, 2); assert.equal(w[0].sent, "猫が好きです。");
  assert.ok(w[0].gloss && /cat/.test(w[0].gloss));
  assert.ok(!w.some((x) => x.f === "犬"), "known words are left out");
  assert.ok(!w.some((x) => x.f === "です" || x.f === "が"), "endings and particles are left out");
  const m = Y.mineWords(["今日は天気がいいので、木がたくさんあります。"], () => false).map((x) => x.r);
  assert.ok(m.includes("きょう") && !m.includes("こんにちは"), "今日は in a sentence is today: " + m);
  assert.ok(m.includes("たくさん") && !m.includes("くさ") && !m.includes("い"), "no mis-splits: " + m);
  const v = Y.mineWords(["散歩しました。ここにあって、わかりました。"], () => false).map((x) => x.r);
  assert.ok(!Y.mineWords(["散歩のあとで"], () => false).some((x) => x.f === "ノア"), "のあと is not ノア");
  assert.ok(!v.includes("ます") && !v.includes("あう") && v.includes("わかる"), "inflections: " + v);
});
test("a verb taught as ます is known in its dictionary form", () => {
  assert.ok(Y.wordForms("帰ります").includes("帰る"));
  assert.ok(Y.wordForms("かえります").includes("かえる"));
  assert.ok(Y.wordForms("運転します").includes("運転") && Y.wordForms("運転します").includes("運転する"));
  assert.deepEqual(Y.wordForms("猫"), ["猫"]);
  assert.ok(!Y.wordForms("話します").includes("話"), "話す is not 話 + する");
});
test("the entrance exam listens only when there's a voice, and never in the kana sections", () => {
  assert.equal(Y.placementSection(3, true).filter((q) => q.say).length, 2);
  assert.equal(Y.placementSection(3, false).filter((q) => q.say).length, 0);
  for (const i of [0, 1, 2]) assert.equal(Y.placementSection(i, true).filter((q) => q.say).length, 0);
  for (const q of Y.placementSection(6, true).filter((q) => q.say)) assert.equal(q.say, q.card.r);
});
test("the entrance exam tests each half of N4 and N3, and can start you partway through", () => {
  assert.equal(Y.PLACEMENT_SECTIONS, 10);
  const n4 = C.stages[C.stages.length - 2].first, n3 = C.stages[C.stages.length - 1].first;
  const n4mid = Y.placementDay(7), n3mid = Y.placementDay(9);
  assert.ok(n4 < n4mid && n4mid < n3 && n3 < n3mid && n3mid < Y.placementDay(10) && Y.placementDay(10) < C.days.length);
  assert.equal(Y.placementDay(8), n3, "passing all of N4 starts you at N3, not back at N4");
  for (const [i, lo, hi] of [[6, n4, n4mid], [7, n4mid, n3], [8, n3, n3mid], [9, n3mid, C.days.length]]) {
    const q = Y.placementSection(i);
    assert.equal(q.length, 5);
    for (const x of q) assert.ok(x.card.day >= lo && x.card.day < hi, x.card.id);
  }
  for (let n = 1; n <= 10; n++) assert.ok(Y.placementDay(n) >= Y.placementDay(n - 1));
  const p = Y.newProfile("T"); Y.place(p, n4mid, 100);
  assert.equal(p.day, n4mid); assert.equal(Y.stageOf(p), C.stages.length - 2);
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
test("cards an update adds to classes already taken join the reviews, ten a day", () => {
  const p = Y.newProfile("Old"); p.day = 40;
  const ids = [...new Set(C.days.slice(0, 40).flatMap((d) => d.new))];
  ids.slice(0, 5).forEach((id) => { p.cards[id] = Y.srsNew(1); }); // what an older save had
  const n = Y.catchUp(p, 100);
  assert.equal(n, ids.length - 5);
  assert.ok(ids.every((id) => p.cards[id]));
  const dues = Object.values(p.cards).filter((c) => c.due >= 100).map((c) => c.due);
  assert.equal(Math.max(...dues), 100 + Math.floor((n - 1) / 10));
  assert.equal(Y.catchUp(p, 100), 0); // only once
});

test("a word's final は can be typed as it's said, but a lone は can't", () => {
  assert.ok(Y.answerOk("konnichiwa", "こんにちは", ""));
  assert.ok(Y.answerOk("こんばんわ", "こんばんは", ""));
  assert.ok(!Y.answerOk("wa", "は", ""));
});

test("readings shown in romaji spell as the kana cards do, and as said", () => {
  for (const [k, r] of [["きって", "kitte"], ["まっちゃ", "matcha"], ["ほん", "hon"], ["きんようび", "kin'youbi"], ["コーヒー", "koohii"],
                        ["こんにちは", "konnichiwa"], ["はは", "haha"], ["ちゃわん", "chawan"], ["ファイル", "fairu"]]) assert.equal(Y.kanaRomaji(k), r, k);
});

test("a save from before 0.20 keeps its class, and a class and part survive a save", () => {
  const old = Y.loadText("YOMIGO-SAVE 1\nname=Hana\nday=50\n");
  assert.equal(C.days[old.day].src, 50); assert.equal(C.days[old.day].part, 1);
  old.day++; assert.equal(Y.loadText(Y.saveText(old)).day, old.day);
  assert.equal(Y.loadText("YOMIGO-SAVE 1\nname=Hana\nday=85\n").day, C.days.length);
});
