// YomiGo Mobile. All text goes in with textContent, so nothing from a save file can become markup.
const C = globalThis.YG_CONTENT, Y = globalThis.YG;
const Bridge = globalThis.YomiBridge || null;
const VERSION = Bridge ? Bridge.version() : "web";
const app = document.getElementById("app"), nav = document.getElementById("nav");

function h(tag, props, ...kids) {
  const e = document.createElement(tag);
  for (const k in props || {}) {
    if (k === "on") e.addEventListener("click", props.on);
    else if (k === "cls") e.className = props.cls;
    else e.setAttribute(k, props[k]);
  }
  for (const c of kids.flat()) if (c !== null && c !== undefined && c !== false) e.append(c.nodeType ? c : String(c));
  return e;
}
const btn = (label, on, cls) => h("button", { on, cls: cls || "" }, label);
const bar = (v, max) => { const b = h("div", { cls: "bar", role: "progressbar", "aria-valuenow": v, "aria-valuemax": max }, h("i")); b.firstChild.style.width = (max ? Math.min(100, v * 100 / max) : 0) + "%"; return b; };
let strokeTimer = null;
function show(...kids) { if (strokeTimer) { clearInterval(strokeTimer); strokeTimer = null; } app.replaceChildren(...kids.flat()); window.scrollTo(0, 0); }
// Stroke order: draws one stroke at a time, on a loop
function strokeBox(ch) {
  const S = globalThis.YG_STROKES && YG_STROKES[ch];
  if (!S) return null;
  const c = h("canvas", { width: 218, height: 218, role: "img", "aria-label": "Stroke order" }), g = c.getContext("2d");
  c.style.cssText = "background:#fff;border:2px solid var(--accent);border-radius:8px;display:block;margin:8px 0;width:109px;height:109px";
  let t = 0;
  const draw = () => {
    const cur = t % (S.length + 2); t++;
    g.clearRect(0, 0, 218, 218); g.lineWidth = 6; g.lineCap = "round"; g.lineJoin = "round";
    S.forEach((pts, i) => {
      if (i > cur) return;
      g.strokeStyle = i === cur ? "#d02050" : "#201018";
      g.beginPath(); pts.forEach(([x, y], k) => (k ? g.lineTo(x * 2, y * 2) : g.moveTo(x * 2, y * 2))); g.stroke();
      if (i === cur) { g.fillStyle = "#d02050"; g.beginPath(); g.arc(pts[0][0] * 2, pts[0][1] * 2, 7, 0, 7); g.fill(); }
    });
  };
  draw();
  setTimeout(() => { strokeTimer = setInterval(draw, 700); }, 0);
  return c;
}

// Storage: one text save per learner
let P = null;
const store = {
  list() { try { return JSON.parse(localStorage.getItem("yg.names") || "[]"); } catch { return []; } },
  load(n) { try { return Y.loadText(localStorage.getItem("yg.save." + n) || ""); } catch { return null; } },
  save() { if (!P) return; try { localStorage.setItem("yg.save." + P.name, Y.saveText(P)); const l = store.list(); if (!l.includes(P.name)) { l.push(P.name); localStorage.setItem("yg.names", JSON.stringify(l)); } } catch (e) { alert("Couldn't save: " + e.message); } },
};
const L = (en, ja, from) => (P && !P.en && Y.stageOf(P) >= from ? ja : en);
const speak = (t) => { if (Bridge) Bridge.speak(t); else if (globalThis.speechSynthesis) { const u = new SpeechSynthesisUtterance(t); u.lang = "ja-JP"; speechSynthesis.speak(u); } };
const canSpeak = () => globalThis.YG_TEST_VOICE || (Bridge ? Bridge.hasVoice() : !!globalThis.speechSynthesis && speechSynthesis.getVoices().some((v) => v.lang.startsWith("ja")));
const canListen = () => !!Bridge || !!(globalThis.SpeechRecognition || globalThis.webkitSpeechRecognition);
let onHeard = null;
globalThis.ygHeard = (text) => { if (onHeard) onHeard(text); };
function listen(cb) {
  onHeard = cb;
  if (Bridge) return Bridge.listen();
  const R = globalThis.SpeechRecognition || globalThis.webkitSpeechRecognition;
  if (!R) return;
  const r = new R(); r.lang = "ja-JP"; r.onresult = (e) => cb(e.results[0][0].transcript); r.onerror = () => cb(""); r.start();
}

// Navigation
const TABS = [["Today", "きょう", 2, home], ["Cards", "カード", 3, cards], ["Exams", "しけん", 2, exams], ["Quest", "クエスト", 3, quest], ["Library", "本だな", 5, library], ["More", "せってい", 2, settings]];
function setNav(cur) {
  nav.replaceChildren(...(P && cur >= 0 ? TABS.map((t, i) => h("button", { on: t[3], cls: i === cur ? "on" : "", "aria-current": i === cur ? "page" : "false" }, L(t[0], t[1], t[2]))) : []));
}

function profiles() {
  P = null; setNav(-1);
  const input = h("input", { placeholder: "Your name", maxlength: 16, "aria-label": "Your name" });
  show(h("h1", null, "YomiGo ✿ 読みGo"), h("p", { cls: "dim" }, "Developed by SENKOSOFT"),
    store.list().map((n) => btn(n, () => { P = store.load(n); if (P) home(); else alert("That save couldn't be read."); })),
    h("div", { cls: "card" }, h("h2", null, "New learner"), input, h("div", { cls: "dim" }, "Letters A-Z and numbers for now."),
      btn("Start", () => {
        const name = input.value.replace(/[^A-Za-z0-9_-]/g, "");
        if (!name) { input.focus(); return; }
        P = store.load(name) || Y.newProfile(name); store.save(); askGenki();
      }, "main")));
}
function askGenki() {
  setNav(-1);
  const pick = (v) => () => { P.genki = v; store.save(); placeIntro(); };
  show(h("h1", null, "Do you study with Genki?"), h("div", { cls: "card" }, "YomiGo can add practice sorted by Genki lesson. It uses YomiGo's own cards alongside your copy of the book. You can change this later in Settings."),
    btn("Yes, turn on Genki practice", pick(1), "main"), btn("No thanks", pick(0)));
}
const addedMsg = (r) => (r > 0 ? "Added to your flashcards." : r === 0 ? "You've already added that." : "Couldn't add that.");
function myWords() {
  setNav(1);
  const word = h("input", { placeholder: "Word", "aria-label": "Word", lang: "ja" }), reading = h("input", { placeholder: "Reading: kana or romaji, optional", "aria-label": "Reading", lang: "ja", autocapitalize: "off" }), mean = h("input", { placeholder: "Meaning", "aria-label": "Meaning" });
  const area = h("textarea", { rows: 4, "aria-label": "Word list", placeholder: "Paste words from YomiGo Reader, or an Anki export" }), msg = h("p", { cls: "acc", role: "status" });
  show(h("h1", null, "My words"), h("p", { cls: "dim" }, `${P.my.length} of 2000. They review with your flashcards. To save more, search the dictionary in the Library.`),
    h("div", { cls: "card" }, h("strong", null, "Add a word"), word, reading, mean,
      btn("Add", () => { const r = Y.myAdd(P, word.value, reading.value ? Y.normKana(reading.value) : "", mean.value, "", Y.today()); store.save(); if (r > 0) myWords(); else msg.textContent = addedMsg(r); }, "main"), msg),
    h("div", { cls: "card" }, h("strong", null, "Bring words in or out"), area,
      btn("Import the list above", () => { const n = Y.myImport(P, area.value, Y.today()); store.save(); msg.textContent = `Imported ${n} word(s).`; if (n) myWords(); }),
      btn("Show my list, for Anki", () => { area.value = Y.myExport(P); })),
    P.my.map((m, i) => h("div", { cls: "card" }, h("strong", { lang: "ja" }, m.word), m.reading !== m.word ? h("span", { cls: "dim", lang: "ja" }, "  " + m.reading) : "", h("div", null, m.gloss), m.sent ? h("div", { cls: "dim", lang: "ja" }, m.sent) : "",
      btn("Delete", () => { if (confirm(`Delete ${m.word}? You can't undo this.`)) { P.my.splice(i, 1); store.save(); myWords(); } }))));
}
// Text reader: pasted text, or a photo read on the phone. Tap a character to look up the word starting there.
let readerLines = [];
globalThis.ygOcr = (text) => { readerLines = [...text.split(/\r?\n/).map((l) => l.trim()).filter((l) => /[\u3040-\u30ff\u3400-\u9fff]/.test(l)), ...readerLines].slice(0, 30); textReader(text ? (readerLines.length ? "" : "No Japanese found in that picture.") : "Couldn't read that picture."); };
function textReader(msg) {
  setNav(4);
  const area = h("textarea", { rows: 3, "aria-label": "Japanese text", placeholder: "Paste Japanese here", lang: "ja" }), out = h("div"), note = h("p", { cls: "dim", role: "status" }, msg || "");
  const ensure = (then) => { if (globalThis.YG_DICT_TEXT) return then(); const s = document.createElement("script"); s.src = "dict.js"; s.onload = () => { Y.dictLoad(YG_DICT_TEXT); then(); }; document.head.append(s); };
  const pick = (line, i) => ensure(() => {
    const hits = Y.dictLookup(line.slice(i)).slice(0, 4);
    out.replaceChildren(...(hits.length ? hits.map(({ card }) => { const add = btn("Add to my words", () => { add.textContent = addedMsg(Y.myAdd(P, card.f, card.r, card.en, line, Y.today())); add.disabled = true; store.save(); });
      return h("div", { cls: "card" }, h("strong", { lang: "ja" }, card.f), card.r !== card.f ? h("span", { cls: "dim", lang: "ja" }, "  " + card.r) : "", h("div", null, card.en), add); }) : [h("p", { cls: "dim" }, "No match there.")]));
  });
  const lines = readerLines.map((line) => h("div", { cls: "card", lang: "ja", style: "font-size:22px;line-height:1.7" }, [...line].map((ch, i) => { const s = h("span", { role: "button", tabindex: "0" }, ch); s.addEventListener("click", () => pick(line, line.indexOf(ch) === i ? i : [...line].slice(0, i).join("").length)); return s; })));
  show(h("h1", null, L("Text Reader", "テキスト", 3)), h("p", { cls: "dim" }, "Paste Japanese, or read it from a photo of a page or screen. Then tap a character to look up the word that starts there."),
    area, btn("Add this text", () => { readerLines = [...area.value.split(/\r?\n/).map((l) => l.trim()).filter((l) => /[぀-ヿ㐀-鿿]/.test(l)), ...readerLines].slice(0, 30); textReader(); }, "main"),
    Bridge && Bridge.ocrPick ? btn("📷 Read from a photo", () => { note.textContent = "Reading..."; Bridge.ocrPick(); }) : "",
    note, out, lines, readerLines.length ? btn("Clear", () => { readerLines = []; textReader(); }) : "");
}
function genki() {
  setNav(1);
  show(h("h1", null, "Genki practice"), h("p", { cls: "dim" }, "By lesson, using YomiGo's own cards. Follows the order of the 3rd edition. Lessons with no YomiGo cards yet arrive with the N4 update."),
    C.genki.map((g, i) => h("div", { cls: "card" }, h("strong", null, "Lesson " + g.lesson), h("div", { cls: "dim" }, g.topics),
      // Extra practice is free to repeat, so it earns no XP
      btn("Practice", () => runQuiz(Y.quizGenki(i), "Genki practice", (r, n) => result("Practice done", `${r} of ${n} correct.`, r * 2 >= n, genki))))));
}
function placeIntro() {
  setNav(-1);
  show(h("h1", null, "Entrance Exam"), h("div", { cls: "card" }, "A short test finds your starting point. It begins with romaji, climbs through kana, words, grammar, and kanji, and stops when it gets too hard. You can press I don't know at any time."),
    btn("Take the exam", () => placement(0, 0), "main"),
    btn("I'm a total beginner. Start from day one.", () => { Y.place(P, 0, Y.today()); store.save(); home(); }));
}
function placement(section, passed) {
  runQuiz(Y.placementSection(section), "Entrance exam", (right) => {
    // Sections passed: romaji, hiragana, katakana, words, grammar, kanji
    const map = [0, 2, 3, 4, 4, 5, 6];
    if (right >= 4 && section < 5) return placement(section + 1, section + 1);
    Y.place(P, map[right >= 4 ? 6 : passed], Y.today()); store.save();
    const st = C.stages[Y.stageOf(P)];
    setNav(-1);
    show(h("h1", null, "Your starting point"), h("div", { cls: "card" }, h("div", { cls: "big acc" }, st.ja), st.name, h("p", { cls: "dim" }, `You'll begin at class ${P.day + 1}. Earlier material goes to your flashcards.`)), btn("Begin", home, "main"));
  }, { noExit: true });
}

function home() {
  setNav(0);
  const t = Y.today(), gate = Y.examGate(P), due = Y.dueCount(P, t);
  if (P.last !== t) P.today = 0;
  const parts = [h("div", { cls: "row" }, h("h1", null, L("Today", "きょう", 2)), h("span", { cls: "dim" }, `🔥 ${P.streak}  ·  XP ${P.xp}`))];
  if (gate >= 0) parts.push(h("div", { cls: "card" }, h("div", { cls: "bad" }, "Required to continue"), h("h2", null, C.exams[gate].name), "Pass with 80%. Retake it as often as you need.", btn(L("Take the exam", "しけん", 2), () => startExam(gate), "main")));
  else if (P.day >= C.days.length) parts.push(h("div", { cls: "card" }, h("h2", { cls: "good" }, "N4 complete!"), "N3 and beyond arrive in a future update. Keep reviewing, and read and listen to real Japanese every day."));
  else {
    const d = C.days[P.day], over = P.today >= P.pace && P.last === t;
    parts.push(h("div", { cls: "card" }, h("div", { cls: "row dim" }, h("span", null, C.stages[d.stage].name), h("span", null, `Class ${P.day + 1} of ${C.days.length}`)), h("div", { cls: "big" }, d.ja), h("div", { cls: "dim" }, d.title), bar(P.step, 10),
      over ? h("div", { cls: "good" }, "Today's class is done. Rest, review, or immerse.") : btn(P.step ? L("Continue class", "つづける", 2) : L("Start class", "はじめる", 2), runStep, "main")));
    parts.push(h("div", { cls: "card" }, h("div", { cls: "acc" }, L("Today's immersion", "イマージョン", 3)), d.imm));
  }
  parts.push(h("div", { cls: "card" }, h("div", { cls: "row" }, L("Flashcards due", "カード", 3), h("strong", null, due)), due ? btn(L("Review now", "ふくしゅう", 2), cards) : null));
  show(parts);
}

// A class is the same seven steps as on the PC
function runStep() {
  const d = C.days[P.day], rom = Y.romajiMode(P.day);
  store.save();
  switch (P.step) {
    case 0: { const q = Y.quizWarmup(P); if (!q.length) return nextStep(); return runQuiz(q, "Warm-up", nextStep); }
    case 1: return teach(0);
    case 2: { const w = d.new.map((id) => Y.byId[id]).filter((c) => c.t === "w").slice(0, 10); return w.length ? typing(w, 0, 0, false) : nextStep(); }
    case 3: { const q = Y.shuffle(d.new.slice(0, 20).map((id) => Y.makeQ(Y.byId[id], Math.floor(Math.random() * 3), rom))); if (!q.length) return nextStep(); return runQuiz(q, "Practice", (r) => { P.xp += r; nextStep(); }, { requeue: true }); }
    case 4: return d.chat ? chat(d.chat, 0, 0) : nextStep();
    case 5: return d.read ? reading(C.reading[d.read], -1, 0, false) : nextStep();
    case 6: {
      // Hearing needs a voice. Without one the class carries on, and Settings shows how to add it.
      const q = canSpeak() ? Y.quizListen(P.day) : [];
      if (!q.length) return nextStep();
      return runQuiz(q, L("Listening", "ききとり", 3), (r) => { P.xp += r; P.listens++; shadow(Y.shadowLines(P.day), 0); });
    }
    case 7: return runQuiz(Y.quizDay(P.day), "Daily quiz", (right, n) => {
      const pct = Math.trunc(right * 100 / n);
      if (pct >= Y.PASS) { P.xp += right * 2; if (pct === 100) P.perfects++; return nextStep(); }
      P.step = 1; store.save(); result("Not yet!", `You scored ${pct}%. You need 80% to continue. Look at your notes and try again.`, false, runStep);
    });
    case 8: return task(d.hw, L("Homework", "しゅくだい", 3), L("I did it!", "できた！", 2), () => {
      const q = d.new.length ? Y.shuffle([...d.new]).slice(0, 3).map((id) => Y.makeQ(Y.byId[id], 0, rom)) : [];
      const done = () => { P.hw++; P.xp += 30; nextStep(); };
      if (q.length) runQuiz(q, "Homework check", done); else done();
    });
    case 9: return task(d.imm, L("Immersion", "イマージョン", 3), L("I'll do it today", "やります！", 2), nextStep);
    default:
      Y.learnDay(P, P.day, Y.today()); Y.completeDay(P, Y.today()); store.save();
      result("Class complete!", `+100 XP. Streak: ${P.streak} day(s). Your new cards are in the flashcard deck.`, true, home);
  }
}
function nextStep() { P.step++; store.save(); runStep(); }
// Shadowing: hear a line, then say it. Speech checks what the recogniser heard; it isn't a pronunciation score.
function shadow(lines, i) {
  if (i >= lines.length) return nextStep();
  const l = lines[i], fb = h("div", { cls: "dim", role: "status" });
  setNav(-1);
  show(h("div", { cls: "row" }, h("h1", null, L("Shadowing", "シャドーイング", 3)), h("span", { cls: "dim" }, `${i + 1} / ${lines.length}`)),
    h("p", { cls: "dim" }, "Listen, then say it out loud, copying the rhythm and tone. Do it as many times as you like."),
    h("div", { cls: "card" }, h("div", { lang: "ja", style: "font-size:22px" }, l.jp), l.ro && Y.stageOf(P) < 4 ? h("div", { cls: "dim" }, l.ro) : "", h("div", null, l.en)),
    btn("🔊 " + L("Hear it", "聞く", 5), () => speak(l.jp)),
    canListen() ? btn("🎤 " + L("Try it", "話す", 5), () => listen((heard) => { fb.textContent = !heard ? "Didn't catch that. Try again." : (Y.answerOk(heard, l.jp, l.ro) ? "It heard what you were aiming for: " : "It heard: ") + heard + ". This only checks the words, not your accent."; })) : "", fb,
    btn("I said it", () => shadow(lines, i + 1), "main"));
  speak(l.jp);
}
// Reading: a short passage, then questions about it
function reading(r, qi, right, en) {
  setNav(-1);
  if (qi < 0) {
    show(h("h1", { lang: "ja" }, r.ja), h("div", { cls: "dim" }, r.title), h("div", { cls: "card pre", lang: "ja", style: "font-size:20px;line-height:1.8" }, en ? r.en : r.jp),
      btn("🔊 " + L("Hear it", "聞く", 5), () => speak(r.jp)), btn(en ? "Japanese" : "English", () => reading(r, -1, 0, !en)), btn(L("Questions", "しつもん", 3), () => reading(r, 0, 0, false), "main"), btn("Later", home));
    return;
  }
  if (qi >= r.q.length) { P.reads++; P.xp += 5 * right; return nextStep(); }
  const [q, good, ...bad] = r.q[qi], fb = h("div");
  const buttons = Y.shuffle([good, ...bad]).map((t) => { const b = btn(t, () => {
    buttons.forEach((x) => { x.disabled = true; if (x.textContent === good) x.className = "right"; });
    if (t !== good) b.className = "wrong";
    fb.replaceChildren(btn(L("Next", "つぎ", 2), () => reading(r, qi + 1, right + (t === good ? 1 : 0), false), "main"));
  }); return b; });
  show(h("div", { cls: "row" }, h("h1", { lang: "ja" }, r.ja), h("span", { cls: "dim" }, `${qi + 1} / ${r.q.length}`)), h("div", { cls: "card pre", lang: "ja" }, r.jp), h("div", { cls: "card" }, h("strong", null, q)), buttons, fb);
}
function task(body, title, label, go) { setNav(-1); show(h("h1", null, title), h("div", { cls: "card" }, body), btn(label, go, "main"), btn("Later", home)); }
function result(title, body, good, next) { setNav(-1); show(h("h1", { cls: good ? "good" : "bad" }, title), h("div", { cls: "card" }, body), btn(L("Continue", "つぎへ", 2), next, "main")); }

function teach(page) {
  const d = C.days[P.day], total = d.notes.length + d.new.length, rom = Y.romajiMode(P.day);
  setNav(-1);
  let body;
  if (page < d.notes.length) body = [h("div", { cls: "acc" }, L("Sensei's notes", "せんせいのノート", 3)), h("p", null, d.notes[page])];
  else {
    const c = Y.byId[d.new[page - d.notes.length]];
    body = [h("div", { cls: "big acc", lang: "ja" }, rom && c.t === "w" ? c.ro : c.f)];
    if ("hkj".includes(c.t)) body.push(strokeBox(c.f));
    if (c.t === "g") {
      body.push(h("p", null, c.en), h("p", { cls: "dim" }, c.n), h("p", { lang: "ja" }, c.s.replace("＿", c.r)), h("p", { cls: "dim" }, c.tr));
    } else {
      if (!"hk".includes(c.t)) body.push(h("div", { lang: "ja" }, rom ? c.r : c.f !== c.r ? c.r : ""), !rom && Y.stageOf(P) < 4 ? h("div", { cls: "dim" }, c.ro) : null);
      body.push(h("p", null, c.en));
      if (c.n) body.push(h("p", { cls: "dim" }, c.n));
    }
    body.push(btn("🔊 " + L("Hear it", "聞く", 5), () => speak(c.t === "g" ? c.s.replace("＿", c.r) : c.r)));
  }
  show(h("div", { cls: "row" }, h("h1", null, d.ja), h("span", { cls: "dim" }, `${page + 1}/${total}`)), h("div", { cls: "card" }, body),
    btn(page + 1 < total ? L("Next", "つぎ", 2) : L("Practice!", "れんしゅう", 3), () => (page + 1 < total ? teach(page + 1) : nextStep()), "main"),
    page > 0 ? btn(L("Back", "もどる", 2), () => teach(page - 1)) : btn("Later", home));
}

// Typing practice: every new word, three times each
function typing(words, i, rep, bad) {
  const c = words[i], rom = Y.romajiMode(P.day), REPS = 3;
  const inp = h("input", { autocapitalize: "off", autocomplete: "off", lang: "ja", placeholder: "Type here", "aria-label": "Type the word" }), pre = h("div", { cls: "acc", lang: "ja" });
  inp.addEventListener("input", () => { pre.textContent = Y.normKana(inp.value); });
  const submit = () => {
    if (!inp.value) return inp.focus();
    if (!Y.answerOk(inp.value, c.r, c.ro)) return typing(words, i, rep, true);
    if (rep + 1 < REPS) return typing(words, i, rep + 1, false);
    P.xp++;
    return i + 1 < words.length ? typing(words, i + 1, 0, false) : nextStep();
  };
  inp.addEventListener("keydown", (e) => { if (e.key === "Enter") submit(); });
  setNav(-1);
  show(h("div", { cls: "row" }, h("h1", null, L("Typing practice", "タイピング", 3)), h("span", { cls: "dim" }, `Word ${i + 1} of ${words.length}`)), bar(rep, REPS),
    h("div", { cls: "card" }, h("div", { cls: "big acc", lang: "ja" }, rom ? c.ro : c.f), h("div", { cls: "dim", lang: "ja" }, rom ? c.r : c.f !== c.r ? c.r : "", !rom && Y.stageOf(P) < 4 ? "  " + c.ro : ""), h("div", null, c.en), h("div", { cls: "dim" }, `Type it ${REPS} times. This is number ${rep + 1}.`)),
    bad ? h("div", { cls: "bad" }, "Not quite. Type: " + c.ro) : "", inp, pre, btn("Enter", submit, "main"), btn("Later", home));
  inp.focus();
}

function runQuiz(qs, title, done, opt = {}) {
  let i = 0, right = 0; const total0 = qs.length;
  setNav(-1);
  const step = () => {
    if (i >= qs.length) return done(right, total0);
    const q = qs[i], fb = h("div"), area = h("div"), big = h("div", { cls: "big", lang: "ja" }, q.say ? "🔊 Listen..." : q.prompt);
    let idk = false, dontKnow = null;
    const after = (ok) => {
      if (dontKnow) dontKnow.remove();
      if (ok) { if (!opt.seenWrong?.has(i)) right++; } else if (opt.requeue && qs.length < 60) qs.push(q);
      if (opt.onAnswer) opt.onAnswer(q, ok);
      if (q.say) big.textContent = q.prompt;
      fb.replaceChildren(h("strong", { cls: ok ? "good" : idk ? "dim" : "bad" }, ok ? L("Correct!", "正解！", 5) : idk ? "That's okay. Here's the answer." : L("Not quite.", "ちがいます", 3)), q.kind === "type" && !ok ? h("div", { lang: "ja" }, q.want) : "", !ok && q.card.n ? h("p", { cls: "dim" }, q.card.n) : "", btn(L("Next", "つぎ", 2), () => { i++; step(); }, "main"));
    };
    if (q.kind === "choice") {
      const buttons = q.choices.map((c) => { const b = btn(c, () => { buttons.forEach((x) => { x.disabled = true; if (x.textContent === q.answer) x.className = "right"; }); if (c !== q.answer) b.className = "wrong"; after(c === q.answer); }); b.lang = "ja"; return b; });
      area.append(h("div", { cls: "grid" }, buttons));
    } else {
      const inp = h("input", { autocapitalize: "off", autocomplete: "off", "aria-label": "Your answer", lang: "ja", placeholder: "Type here" }), pre = h("div", { cls: "acc", lang: "ja" });
      inp.addEventListener("input", () => { pre.textContent = Y.normKana(inp.value); });
      const check = btn(L("Check", "チェック", 3), () => { check.disabled = true; inp.disabled = true; after(Y.answerOk(inp.value, q.want, q.want2)); }, "main");
      inp.addEventListener("keydown", (e) => { if (e.key === "Enter" && !check.disabled) check.click(); });
      area.append(inp, pre, check);
      q.focus = inp;
    }
    // Not knowing is a normal answer. It counts as a miss, and shows the right one.
    dontKnow = btn(L("I don't know", "わかりません", 3), () => {
      idk = true;
      for (const b of area.querySelectorAll("button, input")) { b.disabled = true; if (b.textContent === q.answer) b.className = "right"; }
      after(false);
    });
    area.append(dontKnow);
    show(h("div", { cls: "row" }, h("h1", null, title), h("span", { cls: "dim" }, `${i + 1} / ${qs.length}`)), bar(i, qs.length),
      h("div", { cls: "card" }, h("div", { cls: "dim" }, q.tag), big, q.sub ? h("div", { cls: "dim" }, q.sub) : "", q.say ? btn("🔊 " + L("Hear again", "もう一度", 5), () => speak(q.say)) : q.card.t !== "g" && q.kind === "choice" && /[ぁ-ヿ一-龯]/.test(q.prompt) ? btn("🔊", () => speak(q.card.r)) : ""), area, fb,
      opt.noExit ? "" : btn("Leave", home));
    if (q.focus) q.focus.focus();
    if (q.say) speak(q.say);
  };
  step();
}

// Conversation, typed or spoken. Speech checks what the recogniser heard; it is not a pronunciation score.
function chat(id, turn, fails) {
  const c = C.chats[id], rom = Y.romajiMode(P.day), stage = Y.stageOf(P);
  if (turn >= c.turns.length) { P.chats++; P.xp += 30; return nextStep(); }
  const t = c.turns[turn], inp = h("input", { autocapitalize: "off", autocomplete: "off", lang: "ja", placeholder: "Type here", "aria-label": "Your reply" }), pre = h("div", { cls: "acc", lang: "ja" });
  inp.addEventListener("input", () => { pre.textContent = Y.normKana(inp.value); });
  const send = (text) => { if (Y.chatOk(text, t)) chat(id, turn + 1, 0); else chat(id, turn, fails + 1); };
  inp.addEventListener("keydown", (e) => { if (e.key === "Enter") send(inp.value); });
  setNav(-1);
  show(h("h1", { lang: "ja" }, c.ja), h("div", { cls: "dim" }, c.title),
    h("div", { cls: "card" }, h("div", { cls: "acc", lang: "ja" }, c.who),
      t.say ? [h("div", { lang: "ja", style: "font-size:20px" }, rom ? t.ro : t.say), rom || stage < 4 ? h("div", { cls: "dim", lang: "ja" }, rom ? t.say : t.ro) : "", stage < 5 || fails ? h("div", { cls: "dim" }, t.en) : "", btn("🔊", () => speak(t.say))] : h("div", { cls: "dim" }, "(You speak first.)")),
    fails ? h("div", { cls: "card" }, h("div", { cls: "bad" }, L("Not quite. Hint:", "ヒント:", 3)), t.hint, fails >= 3 ? h("div", { cls: "acc", lang: "ja" }, "Try: " + (rom ? t.wantRo : t.want[0])) : "") : "",
    inp, pre, btn(L("Say it", "言う", 5), () => send(inp.value), "main"),
    btn(L("I don't know", "わかりません", 3), () => chat(id, turn, 3)),
    canListen() ? btn("🎤 " + L("Speak", "話す", 5), () => listen((heard) => { if (!heard) return alert("Didn't catch that. Try again, or type it."); inp.value = heard; pre.textContent = "Heard: " + heard; send(heard); })) : "",
    btn("Later", home));
  inp.focus();
}

function cards() {
  setNav(1);
  const t = Y.today(), due = Y.dueCount(P, t), all = [...Object.values(P.cards), ...P.my];
  show(h("h1", null, L("Flashcards", "カード", 3)), h("div", { cls: "card" }, h("div", { cls: "big " + (due ? "acc" : "good") }, due), due === 1 ? "card due" : "cards due", h("p", { cls: "dim" }, `Deck: ${all.length}. Struggling with: ${all.filter((c) => c.lapses).length}. Right answers come back later, wrong ones tomorrow. Only due cards earn XP.`)),
    due ? btn(L("Review", "ふくしゅう", 2), () => runQuiz(Y.quizReview(P, t, 30), "Flashcards", (r, n) => { store.save(); result("Review done", `${r} of ${n} correct. Cards you missed will come back soon.`, r * 2 >= n, home); },
      { onAnswer: (q, ok) => { Y.srsGrade(q.my || P.cards[q.card.id], ok ? 2 : 0, Y.today()); P.reviews++; if (ok) P.xp += 2; store.save(); } }), "main") : "",
    btn(`My words (${P.my.length})`, myWords),
    P.genki ? btn("Genki practice", genki) : "");
}
function startExam(i) {
  const e = C.exams[i];
  runQuiz(Y.quizExam(i), e.name, (r, n) => {
    const pct = Math.trunc(r * 100 / n), pass = Y.examResult(P, i, pct); store.save();
    result(e.name, pass ? `You scored ${pct}%. The way forward is open.` : `You scored ${pct}%. You need 80%. Review your cards and challenge it again.`, pass, home);
  }, { noExit: true });
}
function exams() {
  setNav(2);
  const gate = Y.examGate(P);
  show(h("h1", null, L("Exams", "しけん", 2)), C.exams.map((e, i) => {
    const open = e.day < P.day, s = P.exams[e.id] || 0;
    return h("div", { cls: "card" }, h("div", { cls: "row" }, e.name, h("span", { cls: s >= Y.PASS ? "good" : i === gate ? "bad" : "dim" }, s >= Y.PASS ? `Best ${s}%` : !open ? "Locked" : i === gate ? "Required" : "Open")), open ? btn("Challenge", () => startExam(i)) : "");
  }), h("p", { cls: "dim" }, "These are YomiGo's own exams, not official JLPT tests. N3, N2, N1, and Beyond N1 arrive in updates. There's no official N0, so YomiGo will offer its own Beyond N1 stage."));
}
const RANKS = [["Newcomer", "入門者"], ["White Sash", "白帯"], ["Road Walker", "道を行く者"], ["Scribe", "書記"], ["Castle Guard", "城の守り"], ["Gatekeeper", "門番"], ["Kanji Knight", "漢字の騎士"], ["N5 Champion", "N5王者"], ["N4 Champion", "N4王者"]];
function quest() {
  setNav(3);
  const r = Y.rankOf(P), ja = Y.stageOf(P) >= 3 && !P.en ? 1 : 0;
  show(h("h1", null, L("Quest", "クエスト", 3)), h("div", { cls: "card" }, h("div", { cls: "big acc", lang: "ja" }, RANKS[r][ja]), bar(r, RANKS.length - 1), `XP ${P.xp}. Best streak ${P.best}.`, r + 1 < RANKS.length ? h("div", { cls: "dim" }, "Next: " + RANKS[r + 1][ja]) : ""),
    h("div", { cls: "card" }, `Classes ${P.day} · Reviews ${P.reviews} · Conversations ${P.chats} · Homework ${P.hw} · Perfect quizzes ${P.perfects}`));
}
function library() {
  setNav(4);
  const stage = Y.stageOf(P), open = (u) => { if (Bridge) Bridge.openUrl(u); else window.open(u, "_blank", "noopener"); };
  const era = (isNew) => C.media.filter((m) => m.stage <= stage + 1 && (m.era === "new") === isNew).map((m) => h("div", { cls: "card" }, h("div", { cls: "row" }, h("strong", { lang: "ja" }, m.ja), h("span", { cls: "dim" }, m.year)), h("div", { cls: "dim" }, `${m.type}: ${m.title}`), m.task));
  // The dictionary is big, so it loads the first time you search
  const q = h("input", { placeholder: "Japanese, romaji, or English", "aria-label": "Search the dictionary", lang: "ja", autocapitalize: "off" }), out = h("div");
  const ensure = (then) => { if (globalThis.YG_DICT_TEXT) return then(); const s = document.createElement("script"); s.src = "dict.js"; s.onload = () => { Y.dictLoad(YG_DICT_TEXT); then(); }; document.head.append(s); };
  const search = () => { if (q.value) ensure(() => { const rows = Y.dictSearch(q.value, 20); out.replaceChildren(...(rows.length ? rows.map((r) => { const add = btn("Add to my words", () => { add.textContent = addedMsg(Y.myAdd(P, r.word, r.reading, r.gloss, "", Y.today())); add.disabled = true; store.save(); }); return h("div", { cls: "card" }, h("strong", { lang: "ja" }, r.word || r.reading), r.word ? h("span", { cls: "dim", lang: "ja" }, "  " + r.reading) : "", h("div", null, r.gloss), add); }) : [h("p", { cls: "dim" }, "Nothing found. Try the plain form of the word.")])); }); };
  q.addEventListener("keydown", (e) => { if (e.key === "Enter") search(); });
  show(h("h1", null, L("Library", "本だな", 5)),
    btn("📝 " + L("Text Reader", "テキスト", 3), () => textReader(), "main"),
    h("h2", null, "Dictionary"), q, btn("Search", search, "main"), out, h("p", { cls: "dim" }, "About 22,000 common words from JMdict by EDRDG, CC BY-SA 4.0."),
    h("h2", null, "Study Guide"), C.guide.map((g) => { const d = h("details", { cls: "card" }, h("summary", null, h("strong", null, g.title))); g.body.split("\n").forEach((p) => d.append(h("p", null, p))); return d; }),
    h("h2", null, "Released after 2020"), era(true), h("h2", null, "Released before 1999"), era(false), h("p", { cls: "dim" }, "Difficulty is YomiGo's own judgement. Check your local services for availability."),
    h("h2", null, "Links"), C.res.map((r) => btn(`${r.name}: ${r.desc}`, () => open(r.url))));
}
function settings() {
  setNav(5);
  const area = h("textarea", { rows: 4, "aria-label": "Save text", placeholder: "Paste a YomiGo save here" });
  const paces = ["Gentle (1 class a day)", "Steady (2 a day)", "Sprint (3 a day)"];
  show(h("h1", null, L("Settings", "せってい", 2)),
    btn("Pace: " + paces[P.pace - 1], () => { P.pace = P.pace % 3 + 1; store.save(); settings(); }),
    btn(P.en ? "Labels: Always English" : "Labels: Gradually Japanese", () => { P.en = P.en ? 0 : 1; store.save(); settings(); }),
    btn(P.genki ? "Genki practice: On" : "Genki practice: Off", () => { P.genki = P.genki ? 0 : 1; store.save(); settings(); }),
    btn(canSpeak() ? "Japanese voice: found" : "Japanese voice: none. Tap to add it", () => { if (Bridge) Bridge.installVoice(); else alert("Listening needs a Japanese voice on this device."); }),
    btn("Check for updates", () => { if (Bridge) Bridge.checkUpdate(); else alert("Updates apply to the Android app."); }),
    btn("Switch learner", profiles),
    h("h2", null, "Move a save between devices"), area,
    btn("Show my save", () => { area.value = Y.saveText(P); }),
    btn("Load the save above", () => { const p = Y.loadText(area.value); if (!p) return alert("That isn't a YomiGo save."); if (confirm(`Replace ${p.name}'s progress on this device?`)) { P = p; store.save(); home(); } }),
    h("p", { cls: "dim" }, `YomiGo Mobile ${VERSION}. © SENKOSOFT. Speech feedback checks what the recogniser heard. It can't confirm native-like pronunciation.`));
}
profiles();
