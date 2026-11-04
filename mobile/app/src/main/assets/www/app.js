// YomiGo screens. Shared by the Android app, the Windows app and its TV mode. Lessons come from core.js, looks from ui.js, platform from host.js.
"use strict";
const C = globalThis.YG_CONTENT, Y = globalThis.YG;
const { h, icon, btn, iconBtn, progress, ring, list, toggle, segmented, empty, field, toast, sheet, sfx } = UI;
const app = document.getElementById("app"), nav = document.getElementById("nav"), toolbar = document.getElementById("toolbar"), root = document.documentElement;

let P = null;
let back = null;      // what Back does here: Esc, the Android back gesture, a mouse's back button, B on a pad
let leaving = [];     // clean-up for the screen being left: timers, listeners
let leaveHook = null; // set while an exam question is open
const store = { save() { if (P) Host.store.write(P.name, Y.saveText(P)); }, load: (n) => Y.loadText(Host.store.text(n)) };
const L = (en, ja, from) => (P && !P.en && Y.stageOf(P) >= from ? ja : en);
const hear = (t, label) => btn([icon("speaker"), label || L("Hear it", "聞く", 5)], () => Host.speak(t), "plain");

function show(...kids) {
  for (const f of leaving) try { f(); } catch {}
  leaving = [];
  const all = kids.flat(Infinity).filter((k) => k !== null && k !== undefined && k !== false && k !== "");
  // In the Big Sur look the page title sits in the toolbar
  if (root.classList.contains("mac")) { const i = all.findIndex((k) => k.classList && (k.classList.contains("head") || k.classList.contains("bar-top"))); toolbar.replaceChildren(...(i >= 0 ? all.splice(i, 1) : [])); }
  app.replaceChildren(...all);
  app.classList.remove("view-in"); void app.offsetWidth; app.classList.add("view-in");
  app.scrollTop = 0; window.scrollTo(0, 0); toolbar.classList.remove("scrolled");
  document.dispatchEvent(new Event("yg:show"));
}
// Registered while a screen is being built, so it belongs to that screen, not the one show() is about to leave
app.addEventListener("scroll", () => toolbar.classList.toggle("scrolled", app.scrollTop > 2));
addEventListener("scroll", () => toolbar.classList.toggle("scrolled", scrollY > 2));
const onLeave = (f) => queueMicrotask(() => leaving.push(f));
function goBack() {
  const d = document.querySelector("dialog[open]");
  if (d) { d.dispatchEvent(new Event("cancel", { cancelable: true })); return true; }
  if (back) { sfx("back"); back(); return true; }
  return false;
}

// ---- headers ----
const head = (title, sub, right) => h("div", { cls: "head" }, h("div", null, h("h1", null, title), sub ? h("div", { cls: "sub" }, sub) : null), right || null);
// The bar on top of a lesson or quiz: close, title, how far along
function topBar(title, count, prog, close) {
  return h("div", { cls: "bar-top" }, close ? iconBtn("close", "Leave", close, "quiet") : null,
    h("div", { cls: "title", lang: /[぀-ヿ一-鿿]/.test(title) ? "ja" : null }, title), prog || null, count ? h("span", { cls: "count" }, count) : null);
}

// ---- navigation ----
const TABS = [["Today", "きょう", 2, () => home(), "home"], ["Cards", "カード", 3, () => cards(), "cards"], ["Exams", "しけん", 2, () => exams(), "exam"],
  ["Quest", "クエスト", 3, () => quest(), "quest"], ["Library", "本だな", 5, () => library(), "library"],
  ...(Host.desktop ? [["Reader", "テキスト", 3, () => textReader(), "reader"], ["Talk", "かいわ", 3, () => talk(), "talk"]] : []),
  ["Settings", "せってい", 2, () => settings(), "settings"]];
const GROUP = { home: "Learn", library: "Read and talk", settings: "YomiGo" };
function setNav(cur) {
  const flow = !cur;
  app.classList.toggle("flow", flow);
  root.classList.toggle("no-side", flow || !P);
  nav.classList.toggle("hide", flow || !P);
  back = cur === "home" || !P ? null : home;
  if (!P || flow) return;
  nav.replaceChildren(h("div", { cls: "brand" }, "Yomi", h("b", null, "Go"), h("span", { lang: "ja", cls: "dim small" }, " 読みGo")),
    ...TABS.flatMap((t, i) => [GROUP[t[4]] ? h("div", { cls: "nav-h" }, GROUP[t[4]]) : null, h("button", { cls: "tab", type: "button", "data-quiet": "1", "aria-current": t[4] === cur ? "page" : null, title: `${t[0]}${Host.desktop && i < 9 ? `  (Ctrl+${i + 1})` : ""}`,
      on: () => { if (t[4] !== cur) sfx("nav"); t[3](); } }, icon(t[4]), h("span", null, L(t[0], t[1], t[2])))].filter(Boolean)),
    h("div", { cls: "who" }, h("span", { cls: "avatar", "aria-hidden": "true" }, P.name[0].toUpperCase()), P.name));
}
document.addEventListener("keydown", (e) => {
  if (Host.tv) return; // the TV has its own keys
  if (e.key === "Escape" && !document.querySelector("dialog[open]")) {
    const t = e.target; if ((t.tagName === "INPUT" || t.tagName === "TEXTAREA") && t.value) return;
    if (goBack()) e.preventDefault();
  }
  const free = P && !app.classList.contains("flow");
  if ((e.ctrlKey || e.metaKey) && /^[1-9]$/.test(e.key) && free) { const t = TABS[+e.key - 1]; if (t) { e.preventDefault(); sfx("nav"); t[3](); } }
  if ((e.ctrlKey || e.metaKey) && e.key === "," && free) { e.preventDefault(); settings(); }
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "f" && free) { e.preventDefault(); libTab = 0; library(); }
});
// The phone's back gesture and a mouse's back button step back inside YomiGo
history.replaceState({ yg: 0 }, ""); history.pushState({ yg: 1 }, "");
addEventListener("popstate", () => { if (goBack()) history.pushState({ yg: 1 }, ""); });
// In an exam, leaving YomiGo while a question is open counts it as wrong, so answers can't be looked up. Not on the TV.
document.addEventListener("visibilitychange", () => { if (document.hidden && leaveHook) leaveHook(); });
addEventListener("blur", () => { if (Host.desktop && !Host.tv && leaveHook) leaveHook(); });

// ---- learners ----
function profiles() {
  P = null; setNav("");
  const name = field({ placeholder: "Your name", maxlength: 16, "aria-label": "Your name", enterkeyhint: "go" });
  const start = () => {
    const n = name.value.replace(/[^A-Za-z0-9_-]/g, "");
    if (!n) { toast("Names use letters A-Z and numbers for now.", "bad"); return name.focus(); }
    P = store.load(n) || Y.newProfile(n); store.save(); askSpeaker(askGenki);
  };
  name.addEventListener("keydown", (e) => { if (e.key === "Enter") start(); });
  const names = Host.store.list();
  show(h("div", { cls: "prompt welcome" }, h("img", { src: "icon.png", alt: "", width: 96, height: 96 }), h("h1", null, "Welcome to YomiGo"), h("div", { cls: "dim", lang: "ja" }, "読みGo ・ にほんごを、まいにち")),
    names.length ? list("Continue as", names.map((n) => ({ label: n, icon: "person", on: () => {
      P = store.load(n); if (!P) return toast("That save couldn't be read.", "bad");
      sfx("select"); if (Y.catchUp(P, Y.today())) store.save(); P.placed || P.day ? home() : placeIntro();
    } }))) : null,
    h("div", { cls: "card" }, h("h3", null, names.length ? "New learner" : "Welcome! What's your name?"), name, h("p", { cls: "dim small" }, "Letters A-Z and numbers for now."), btn("Start", start, "primary")));
}
// Who's learning picks the casual words for "I" added to My words. わたし is in the course for everyone.
const GENDER = ["Not asked", "Woman", "Man", "Rather not say", "Says ぼく"];
function askSpeaker(next) {
  setNav(""); back = null;
  const pick = (g) => () => {
    const who = g === 1 ? "w" : g === 2 ? "m" : g === 4 ? "b" : "";
    P.gender = g;
    C.pronouns.filter((p) => who && p.who.includes(who)).forEach((p) => Y.myAdd(P, p.word, p.reading, p.en, p.sent, Y.today()));
    store.save(); sfx("select"); next();
  };
  show(head("Who's learning?"), h("div", { cls: "card" }, "Japanese has several words for \"I\". わたし suits everyone, and the course teaches it. Casually, many men say ぼく or おれ, and many women あたし. Some people of any gender say ぼく. Your answer adds the ones that fit you to My words. You can change it in Settings."),
    list(null, [{ label: "A woman", on: pick(1) }, { label: "A man", on: pick(2) }, { label: "Rather not say", on: pick(3) }, { label: "I use ぼく", on: pick(4) }]));
}
function askGenki() {
  setNav("");
  const pick = (v) => () => { P.genki = v; store.save(); placeIntro(); };
  show(head("Do you study with Genki?"), h("div", { cls: "card" }, "YomiGo can add practice sorted by Genki lesson. It uses YomiGo's own cards alongside your copy of the book. You can change this later in Settings."),
    btn("Yes, turn on Genki practice", pick(1), "primary"), btn("No thanks", pick(0)));
}
function placeIntro() {
  setNav("");
  show(head("Entrance Exam", "Find your starting point"), h("div", { cls: "card" }, "A short test finds your starting point. It begins with romaji, climbs through kana, words, grammar, and kanji, and stops when it gets too hard. You can press I don't know at any time."),
    btn("Take the exam", () => placement(0, 0), "primary"),
    btn("I'm a total beginner. Start from day one.", () => { Y.place(P, 0, Y.today()); store.save(); home(); }));
}
function placement(section, passed) {
  runQuiz(Y.placementSection(section), "Entrance exam", (right) => {
    // Sections passed: romaji, hiragana, katakana, words, grammar, kanji
    const map = [0, 2, 3, 4, 4, 5, 6];
    if (right >= 4 && section < 5) return placement(section + 1, section + 1);
    Y.place(P, map[right >= 4 ? 6 : passed], Y.today()); store.save();
    const st = C.stages[Y.stageOf(P)];
    setNav(""); sfx("done");
    show(head("Your starting point"), h("div", { cls: "card hero" }, h("div", { cls: "big acc", lang: "ja" }, st.ja), h("h3", null, st.name), h("p", { cls: "dim" }, `You'll begin at class ${P.day + 1}. Earlier material goes to your flashcards.`)), btn("Begin", home, "primary"));
  }, { noExit: true });
}

// ---- today ----
const RANKS = [["Newcomer", "入門者"], ["White Sash", "白帯"], ["Road Walker", "道を行く者"], ["Scribe", "書記"], ["Castle Guard", "城の守り"], ["Gatekeeper", "門番"], ["Kanji Knight", "漢字の騎士"], ["N5 Champion", "N5王者"], ["N4 Champion", "N4王者"], ["N3 Champion", "N3王者"]];
const rankName = () => RANKS[Y.rankOf(P)][Y.stageOf(P) >= 3 && !P.en ? 1 : 0];
const greeting = () => { const hr = new Date().getHours(); return hr < 5 ? "Up late" : hr < 12 ? "Good morning" : hr < 18 ? "Good afternoon" : "Good evening"; };
function home() {
  if (!P.gender) return askSpeaker(home); // saves from before 0.7.1 are asked once
  if (Host.tv && typeof TV !== "undefined") return TV.hub();
  setNav("home");
  const t = Y.today(), gate = Y.examGate(P), due = Y.dueCount(P, t);
  if (P.last !== t) P.today = 0;
  const parts = [head(L("Today", "きょう", 2), `${greeting()}, ${P.name}`),
    h("div", { cls: "chips" }, h("span", { cls: "chip fire", title: "Day streak" }, icon("flame"), `${P.streak} day${P.streak === 1 ? "" : "s"}`),
      h("span", { cls: "chip star", title: "Experience" }, icon("star"), `${P.xp} XP`), h("span", { cls: "chip rank", lang: "ja" }, rankName()))];
  if (gate >= 0) parts.push(h("div", { cls: "card hero" }, h("span", { cls: "badge bad" }, "Required to continue"), h("h2", null, C.exams[gate].name), h("p", { cls: "dim" }, "Pass with 80%. Retake it as often as you need."), btn(L("Take the exam", "しけん", 2), () => startExam(gate), "primary")));
  else if (P.day >= C.days.length) parts.push(h("div", { cls: "card hero" }, h("h2", { cls: "good" }, "Course complete!"), h("p", null, "More words, then N2, arrive in future updates. Keep reviewing, and read and listen to real Japanese every day.")));
  else {
    const d = C.days[P.day], over = P.today >= P.pace && P.last === t;
    parts.push(h("div", { cls: "card hero" }, h("div", { cls: "row" }, h("div", null, h("div", { cls: "eyebrow" }, `${C.stages[d.stage].name} · Class ${P.day + 1} of ${C.days.length}`),
        h("div", { cls: "big", lang: "ja", style: "margin-top:8px" }, d.ja), h("div", { cls: "dim" }, d.title)), ring(P.step, 10, `${P.step * 10}%`)),
      over ? h("p", { cls: "good" }, "Today's class is done. Rest, review, or immerse.") : btn(P.step ? L("Continue class", "つづける", 2) : L("Start class", "はじめる", 2), () => { sfx("select"); runStep(); }, "primary")));
    parts.push(h("div", { cls: "grid2" },
      h("div", { cls: "card" }, h("div", { cls: "eyebrow" }, L("Flashcards due", "カード", 3)), h("div", { cls: "stat " + (due ? "acc" : "good") }, due), due ? btn(L("Review now", "ふくしゅう", 2), cards, "sm") : h("div", { cls: "dim small" }, "All caught up")),
      h("div", { cls: "card" }, h("div", { cls: "eyebrow" }, L("Today's immersion", "イマージョン", 3)), h("p", { cls: "small" }, d.imm))));
  }
  show(parts);
}

// A class is ten steps, as on every YomiGo
function runStep() {
  const d = C.days[P.day], rom = Y.romajiMode(P.day);
  store.save();
  switch (P.step) {
    case 0: { const q = Y.quizWarmup(P); if (!q.length) return nextStep(); return runQuiz(q, "Warm-up", nextStep); }
    case 1: return teach(0);
    case 2: { const w = d.new.map((id) => Y.byId[id]).filter((c) => c.t === "w").slice(0, 10); return w.length ? typing(w, 0, 0, false) : nextStep(); }
    case 3: { const q = Y.shuffle(d.new.slice(0, 20).map((id) => Y.makeQ(Y.byId[id], Math.floor(Math.random() * 3), rom))); if (!q.length) return nextStep(); return runQuiz(q, "Practice", (r) => { P.xp += r; nextStep(); }, { requeue: true, xpEach: 1 }); }
    case 4: return d.chat ? chat(d.chat, 0, 0) : nextStep();
    case 5: return d.read ? reading(C.reading[d.read], -1, 0, false) : nextStep();
    case 6: {
      // Hearing needs a voice. Without one the class carries on, and Settings shows how to add it.
      const q = Host.canSpeak() ? Y.quizListen(P.day) : [];
      if (!q.length) return nextStep();
      return runQuiz(q, L("Listening", "ききとり", 3), (r) => { P.xp += r; P.listens++; shadow(Y.shadowLines(P.day), 0); }, { xpEach: 1 });
    }
    case 7: return runQuiz(Y.quizDay(P.day), "Daily quiz", (right, n) => {
      const pct = Math.trunc(right * 100 / n);
      if (pct >= Y.PASS) { P.xp += right * 2; if (pct === 100) { P.perfects++; UI.endRun(); sfx("ace"); UI.petalRain(); toast("Perfect score!", "good"); } return nextStep(); }
      P.step = 1; store.save(); result("Not yet!", `You scored ${pct}%. You need 80% to continue. Look at your notes and try again.`, false, runStep);
    });
    case 8: return task(d.hw, L("Homework", "しゅくだい", 3), L("I did it!", "できた！", 2), () => {
      const q = d.new.length ? Y.shuffle([...d.new]).slice(0, 3).map((id) => Y.makeQ(Y.byId[id], 0, rom)) : [];
      const done = () => { P.hw++; P.xp += 30; nextStep(); };
      if (q.length) runQuiz(q, "Homework check", done); else done();
    });
    case 9: return task(d.imm, L("Immersion", "イマージョン", 3), L("I'll do it today", "やります！", 2), nextStep, d.mat);
    default:
      Y.learnDay(P, P.day, Y.today()); Y.completeDay(P, Y.today()); store.save();
      result("Class complete!", `Streak: ${P.streak} day(s). Your new cards are in the flashcard deck.`, 2, home, 100);
  }
}
function nextStep() { P.step++; store.save(); runStep(); }
const stepBar = () => progress(P.step, 10);

// Shadowing: hear a line, then say it. Speech checks what the recogniser heard; it isn't a pronunciation score.
function shadow(lines, i) {
  if (i >= lines.length) return nextStep();
  const l = lines[i], fb = h("p", { cls: "dim", role: "status" });
  setNav("");
  show(topBar(L("Shadowing", "シャドーイング", 3), `${i + 1} / ${lines.length}`, stepBar(), home),
    h("p", { cls: "dim" }, "Listen, then say it out loud, copying the rhythm and tone. Do it as many times as you like."),
    h("div", { cls: "card" }, h("div", { lang: "ja", cls: "jp-read" }, l.jp), l.ro && Y.stageOf(P) < 4 ? h("div", { cls: "dim" }, l.ro) : "", h("div", null, l.en), hear(l.jp)),
    Host.canListen() ? btn([icon("mic"), L("Try it", "話す", 5)], () => Host.listen((heard) => { fb.textContent = !heard ? "Didn't catch that. Try again." : (Y.answerOk(heard, l.jp, l.ro) ? "It heard what you were aiming for: " : "It heard: ") + heard + ". This only checks the words, not your accent."; })) : "", fb,
    btn("I said it", () => shadow(lines, i + 1), "primary"));
  Host.speak(l.jp);
}
// Reading: a short passage, then questions about it
function reading(r, qi, right, en) {
  setNav("");
  if (qi < 0) {
    show(topBar(r.ja, null, stepBar(), home), h("div", { cls: "dim" }, r.title), h("div", { cls: "card pre jp-read", lang: en ? null : "ja" }, en ? r.en : r.jp),
      h("div", { cls: "btn-row" }, hear(r.jp), btn(en ? "Japanese" : "English", () => reading(r, -1, 0, !en), "plain")), btn(L("Questions", "しつもん", 3), () => reading(r, 0, 0, false), "primary"));
    return;
  }
  if (qi >= r.q.length) { P.reads++; P.xp += 5 * right; return nextStep(); }
  const [q, good, ...bad] = r.q[qi], fb = h("div");
  const buttons = Y.shuffle([good, ...bad]).map((t) => { const b = btn(t, () => {
    buttons.forEach((x) => { x.disabled = true; if (x.textContent === good) x.classList.add("right"); });
    if (t !== good) { b.classList.add("wrong"); UI.cheerMiss(); } else UI.cheerRight(b);
    const n = btn(L("Next", "つぎ", 2), () => reading(r, qi + 1, right + (t === good ? 1 : 0), false), "primary"); fb.replaceChildren(n); n.focus();
  }, "", { "data-quiet": "1" }); return b; });
  show(topBar(r.ja, `${qi + 1} / ${r.q.length}`, stepBar(), home), h("div", { cls: "card pre", lang: "ja" }, r.jp), h("div", { cls: "card tight" }, h("strong", null, q)), buttons, fb);
}
// Homework and immersion. Immersion on the PC also offers a matching file from the Material folder.
function task(body, title, label, go, mat) {
  setNav("");
  const extra = h("div");
  show(topBar(title, null, stepBar(), home), h("div", { cls: "card hero" }, h("p", null, body)), extra, btn(label, go, "primary"), btn("Later", home, "quiet"));
  if (mat && Host.desktop) Host.call("material").then((files) => {
    const f = files.find(([rel]) => rel.toLowerCase().includes(mat.toLowerCase()));
    if (f) extra.replaceChildren(...list(L("From your Material folder", "教材", 5), [{ label: f[0], icon: "folder", on: () => Host.open(f[1]) }]));
  });
}
function result(title, body, good, next, xp) {
  UI.endRun(); sfx(good === 2 ? "ace" : good ? "done" : "bad"); if (good === 2) UI.petalRain();
  setNav(""); back = next;
  const n = h("span", { cls: "count-up" }, "0");
  show(h("div", { cls: "prompt" }, h("div", { cls: "mark " + (good ? "good" : "bad"), lang: "ja" }, good ? "○" : "×"), h("h1", { style: "margin-top:16px" }, title)),
    xp ? h("div", { cls: "card", style: "text-align:center" }, h("div", { cls: "eyebrow" }, "Experience"), h("div", { cls: "stat star", style: "color:var(--gold)" }, "+", n, " XP")) : null,
    h("div", { cls: "card" }, body), btn(L("Continue", "つぎへ", 2), next, "primary"));
  if (xp) UI.countUp(n, xp);
}

function strokeBox(ch) {
  const S = globalThis.YG_STROKES && YG_STROKES[ch];
  if (!S) return null;
  const c = h("canvas", { width: 256, height: 256, role: "img", "aria-label": "Stroke order", cls: "strokes" }), g = c.getContext("2d");
  let t = 0;
  const draw = () => {
    const cur = t % (S.length + 2), k = 256 / 109; t++;
    g.clearRect(0, 0, 256, 256); g.lineWidth = 7; g.lineCap = "round"; g.lineJoin = "round";
    S.forEach((pts, i) => {
      if (i > cur) return;
      g.strokeStyle = i === cur ? "#d6336c" : "#1d1d1f";
      g.beginPath(); pts.forEach(([x, y], j) => (j ? g.lineTo(x * k, y * k) : g.moveTo(x * k, y * k))); g.stroke();
      if (i === cur) { g.fillStyle = "#d6336c"; g.beginPath(); g.arc(pts[0][0] * k, pts[0][1] * k, 8, 0, 7); g.fill(); }
    });
  };
  draw();
  const timer = setInterval(draw, 700); onLeave(() => clearInterval(timer));
  return c;
}
function teach(page) {
  const d = C.days[P.day], total = d.notes.length + d.new.length, rom = Y.romajiMode(P.day);
  setNav(""); back = page > 0 ? () => teach(page - 1) : home;
  let body;
  if (page < d.notes.length) body = [h("div", { cls: "eyebrow acc" }, L("Sensei's notes", "せんせいのノート", 3)), h("p", { style: "font-size:18px" }, d.notes[page])];
  else {
    const c = Y.byId[d.new[page - d.notes.length]];
    body = [h("div", { cls: "big acc", lang: "ja" }, rom && c.t === "w" ? c.ro : c.f)];
    if ("hkj".includes(c.t)) body.push(strokeBox(c.f));
    if (c.t === "g") body.push(h("p", null, c.en), h("p", { cls: "dim" }, c.n), h("div", { cls: "note" }, h("div", { lang: "ja" }, c.s.replace("＿", c.r)), h("div", { cls: "dim small" }, c.tr)));
    else {
      if (!"hk".includes(c.t)) body.push(h("div", { lang: "ja", style: "font-size:20px" }, rom ? c.r : c.f !== c.r ? c.r : ""), !rom && Y.stageOf(P) < 4 ? h("div", { cls: "dim" }, c.ro) : null);
      body.push(h("p", { style: "font-size:18px" }, c.en));
      if (c.n) body.push(h("p", { cls: "dim" }, c.n));
      // a word comes with a sentence people actually say, so it's learnt as a phrase
      if (c.t === "w" && c.s && !rom) body.push(h("div", { cls: "note" }, h("div", { lang: "ja" }, c.s), h("div", { cls: "dim small" }, c.tr)));
    }
    body.push(hear(c.t === "g" ? c.s.replace("＿", c.r) : c.r));
  }
  show(topBar(d.ja, `${page + 1}/${total}`, progress(page + 1, total), home), h("div", { cls: "card" }, body),
    btn(page + 1 < total ? L("Next", "つぎ", 2) : L("Practice!", "れんしゅう", 3), () => (page + 1 < total ? teach(page + 1) : nextStep()), "primary"),
    page > 0 ? btn(L("Back", "もどる", 2), () => teach(page - 1), "quiet") : btn("Later", home, "quiet"));
}

// Typing practice: every new word, three times each
function typing(words, i, rep, bad) {
  const c = words[i], rom = Y.romajiMode(P.day), REPS = 3;
  const inp = field({ lang: "ja", placeholder: "Type here", "aria-label": "Type the word", cls: "big-in", enterkeyhint: "done" }), pre = h("div", { cls: "preview", lang: "ja", "aria-live": "polite" });
  inp.addEventListener("input", () => { pre.textContent = Y.normKana(inp.value); });
  const submit = () => {
    if (!inp.value) return inp.focus();
    const ok = Y.answerOk(inp.value, c.r, c.ro);
    ok ? UI.cheerRight(inp) : UI.cheerMiss();
    if (!ok) return typing(words, i, rep, true);
    if (rep + 1 < REPS) return typing(words, i, rep + 1, false);
    P.xp++;
    return i + 1 < words.length ? typing(words, i + 1, 0, false) : nextStep();
  };
  inp.addEventListener("keydown", (e) => { if (e.key === "Enter") submit(); });
  setNav("");
  show(topBar(L("Typing practice", "タイピング", 3), `Word ${i + 1} of ${words.length}`, progress(i * REPS + rep, words.length * REPS), home),
    h("div", { cls: "card prompt" }, h("div", { cls: "big acc", lang: "ja" }, rom ? c.ro : c.f), h("div", { cls: "dim", lang: "ja" }, rom ? c.r : c.f !== c.r ? c.r : "", !rom && Y.stageOf(P) < 4 ? "  " + c.ro : ""), h("div", null, c.en),
      h("div", { cls: "chips", style: "justify-content:center;margin:12px 0 0" }, [0, 1, 2].map((k) => h("span", { cls: "badge " + (k < rep ? "good" : k === rep ? "acc" : "") }, k < rep ? "✓" : k + 1)))),
    bad ? h("p", { cls: "bad", role: "alert", style: "text-align:center" }, "Not quite. Type " + c.ro + " or " + c.r) : "", inp, pre, btn("Enter", submit, "primary"), btn("Later", home, "quiet"));
  inp.focus();
}

function runQuiz(qs, title, done, opt = {}) {
  let i = 0, right = 0; const total0 = qs.length;
  setNav("");
  const leave = opt.noExit ? null : home;
  const step = () => {
    back = opt.noExit ? () => {} : home;
    if (i >= qs.length) return done(right, total0);
    const q = qs[i], fb = h("div", { cls: "feedback", "aria-live": "polite" }), area = h("div"), big = h("div", { cls: "big", lang: "ja" }, q.say ? icon("speaker") : q.prompt);
    if (q.say) big.firstChild.style.cssText = "width:56px;height:56px;color:var(--accent)";
    let idk = false, left = false, dontKnow = null, answered = false;
    const after = (ok) => {
      answered = true; leaveHook = null;
      if (dontKnow) dontKnow.remove();
      ok ? UI.cheerRight(fb, opt.xpEach) : idk ? (UI.endRun(), sfx("off")) : UI.cheerMiss();
      if (ok) { if (!opt.seenWrong?.has(i)) right++; } else if (opt.requeue && qs.length < 60) qs.push(q);
      if (opt.onAnswer) opt.onAnswer(q, ok);
      if (q.say) big.textContent = q.prompt;
      const next = btn(L("Next", "つぎ", 2), () => { i++; step(); }, "primary", { "data-quiet": "1" });
      fb.replaceChildren(idk ? "" : h("div", { cls: "mark " + (ok ? "good" : "bad"), "aria-hidden": "true", lang: "ja" }, ok ? "○" : "×"),
        h("strong", { cls: ok ? "good" : idk ? "dim" : "bad" }, ok ? L("Correct!", "正解！", 5) : idk ? "That's okay. Here's the answer." : left ? "You left YomiGo during the exam, so this one counts as wrong." : L("Not quite.", "ちがいます", 3)),
        q.kind === "type" && !ok ? h("div", { cls: "answer", lang: "ja" }, q.want) : "", !ok && q.card.n ? h("p", { cls: "dim" }, q.card.n) : "", next);
      next.focus({ preventScroll: true }); fb.scrollIntoView({ behavior: UI.calm.matches ? "auto" : "smooth", block: "nearest" });
    };
    if (q.kind === "choice") {
      const buttons = q.choices.map((c, k) => btn(c, () => { buttons.forEach((x) => { x.disabled = true; if (x.textContent === q.answer) x.classList.add("right"); }); if (c !== q.answer) buttons.find((x) => x.textContent === c).classList.add("wrong"); after(c === q.answer); }, "", { lang: "ja", "data-quiet": "1", "data-key": k + 1 }));
      area.append(h("div", { cls: "choices" }, buttons));
      // 1 to 4 pick an answer from the keyboard
      const keys = (e) => { if (!answered && buttons[0].isConnected && /^[1-9]$/.test(e.key) && buttons[+e.key - 1] && !(e.target.tagName === "INPUT")) buttons[+e.key - 1].click(); };
      document.addEventListener("keydown", keys); onLeave(() => document.removeEventListener("keydown", keys));
    } else {
      const inp = field({ "aria-label": "Your answer", lang: "ja", placeholder: "Type here", cls: "big-in", enterkeyhint: "done" }), pre = h("div", { cls: "preview", lang: "ja" });
      inp.addEventListener("input", () => { pre.textContent = Y.normKana(inp.value); });
      const check = btn(L("Check", "チェック", 3), () => { if (!inp.value.trim()) return inp.focus(); check.disabled = true; inp.disabled = true; after(Y.answerOk(inp.value, q.want, q.want2)); }, "primary", { "data-quiet": "1" });
      inp.addEventListener("keydown", (e) => { if (e.key === "Enter" && !check.disabled) check.click(); });
      area.append(inp, pre, check);
      q.focus = inp;
    }
    if (opt.noExit && !Host.tv) leaveHook = () => { left = true; for (const b of area.querySelectorAll("button, input")) b.disabled = true; after(false); };
    // Not knowing is a normal answer. It counts as a miss, and shows the right one.
    dontKnow = btn(L("I don't know", "わかりません", 3), () => {
      idk = true;
      for (const b of area.querySelectorAll("button, input")) { b.disabled = true; if (b.textContent === q.answer) b.classList.add("right"); }
      after(false);
    }, "quiet");
    area.append(dontKnow);
    show(topBar(title, `${i + 1} / ${qs.length}`, progress(i, qs.length), leave),
      h("div", { cls: "card prompt" }, h("div", { cls: "eyebrow" }, q.tag), big, q.sub ? h("div", { cls: "dim" }, q.sub) : "",
        q.say ? hear(q.say, L("Hear again", "もう一度", 5)) : q.card.t !== "g" && q.kind === "choice" && /[ぁ-ヿ一-龯]/.test(q.prompt) ? hear(q.card.r) : ""), area, fb);
    if (q.focus) q.focus.focus();
    if (q.say) Host.speak(q.say);
  };
  step();
}

// Conversation, typed or spoken, as messages. Speech checks what the recogniser heard; it is not a pronunciation score.
function chat(id, turn, fails, log = []) {
  const c = C.chats[id], rom = Y.romajiMode(P.day), stage = Y.stageOf(P);
  if (turn >= c.turns.length) { P.chats++; P.xp += 30; return nextStep(); }
  const t = c.turns[turn], inp = field({ lang: "ja", placeholder: "Your reply", "aria-label": "Your reply", enterkeyhint: "send" }), pre = h("div", { cls: "preview", lang: "ja" });
  inp.addEventListener("input", () => { pre.textContent = Y.normKana(inp.value); });
  const send = (text) => {
    if (!text.trim()) return inp.focus();
    if (Y.chatOk(text, t)) { UI.cheerRight(inp); chat(id, turn + 1, 0, [...log, t.say ? { them: rom ? t.ro : t.say } : null, { you: text }].filter(Boolean)); }
    else { UI.cheerMiss(); chat(id, turn, fails + 1, log); }
  };
  inp.addEventListener("keydown", (e) => { if (e.key === "Enter") send(inp.value); });
  setNav("");
  show(topBar(c.ja, c.title, stepBar(), home),
    h("div", { cls: "chat" }, log.map((m) => m.you ? h("div", { cls: "msg you", lang: "ja" }, h("div", { cls: "who" }, "You"), m.you) : h("div", { cls: "msg them", lang: "ja" }, h("div", { cls: "who" }, c.who), m.them)),
      t.say ? h("div", { cls: "msg them" }, h("div", { cls: "who", lang: "ja" }, c.who), h("div", { lang: "ja", style: "font-size:19px" }, rom ? t.ro : t.say), rom || stage < 4 ? h("div", { cls: "dim small", lang: "ja" }, rom ? t.say : t.ro) : "",
        stage < 5 || fails ? h("div", { cls: "en" }, t.en) : "", iconBtn("speaker", "Hear it", () => Host.speak(t.say), "quiet")) : h("p", { cls: "dim", style: "text-align:center" }, "You speak first.")),
    fails ? h("div", { cls: "note" }, h("strong", { cls: "bad" }, L("Not quite. Hint: ", "ヒント: ", 3)), t.hint, fails >= 3 ? h("div", { cls: "acc", lang: "ja" }, "Try: " + (rom ? t.wantRo : t.want[0])) : "") : "",
    h("div", { cls: "composer" }, inp, iconBtn("send", L("Say it", "言う", 5), () => send(inp.value), "primary")), pre,
    h("div", { cls: "btn-row" }, btn(L("I don't know", "わかりません", 3), () => chat(id, turn, 3, log), "quiet"),
      Host.canListen() ? btn([icon("mic"), L("Speak", "話す", 5)], () => Host.listen((heard) => { if (!heard) return toast("Didn't catch that. Try again, or type it."); inp.value = heard; pre.textContent = "Heard: " + heard; send(heard); }), "plain") : ""));
  if (t.say) Host.speak(t.say);
  inp.focus();
}

// ---- flashcards ----
function cards() {
  setNav("cards");
  const t = Y.today(), due = Y.dueCount(P, t), all = [...Object.values(P.cards), ...P.my];
  show(head(L("Flashcards", "カード", 3), `${all.length} cards in your deck`),
    h("div", { cls: "card hero", style: "text-align:center" }, h("div", { cls: "stat " + (due ? "acc" : "good"), style: "font-size:64px" }, due), h("div", null, due === 1 ? "card due" : "cards due"),
      h("p", { cls: "dim small" }, `Struggling with ${all.filter((c) => c.lapses).length}. Right answers come back later, wrong ones tomorrow. Only due cards earn XP.`),
      due ? btn(L("Review", "ふくしゅう", 2), () => runQuiz(Y.quizReview(P, t, 30), "Flashcards", (r, n) => { store.save(); result("Review done", `${r} of ${n} correct. Cards you missed will come back soon.`, r === n ? 2 : r * 2 >= n, home, r * 2); },
        { xpEach: 2, onAnswer: (q, ok) => { Y.srsGrade(q.my || P.cards[q.card.id], ok ? 2 : 0, Y.today()); P.reviews++; if (ok) P.xp += 2; store.save(); } }), "primary") : h("p", { cls: "good" }, "All caught up. Nice!")),
    list(null, [{ label: "My words", value: P.my.length, icon: "plus", on: myWords }, P.genki ? { label: "Genki practice", icon: "book", tint: "#0a84ff", on: genki } : null]));
}
const addedMsg = (r) => (r > 0 ? "Added to your flashcards." : r === 0 ? "You've already added that." : "Couldn't add that.");
function myWords() {
  setNav("cards"); back = cards;
  const word = field({ placeholder: "Word", "aria-label": "Word", lang: "ja" }), reading = field({ placeholder: "Reading: kana or romaji, optional", "aria-label": "Reading", lang: "ja" }), mean = field({ placeholder: "Meaning", "aria-label": "Meaning" });
  const area = h("textarea", { rows: 4, "aria-label": "Word list", placeholder: "Paste words from YomiGo Reader, or an Anki export" });
  const add = () => { const r = Y.myAdd(P, word.value, reading.value ? Y.normKana(reading.value) : "", mean.value, "", Y.today()); store.save(); if (r > 0) { sfx("done"); myWords(); toast("Added " + word.value, "good"); } else toast(addedMsg(r), r === 0 ? "" : "bad"); };
  mean.addEventListener("keydown", (e) => { if (e.key === "Enter") add(); });
  show(head("My words", `${P.my.length} of 2000. They review with your flashcards.`),
    h("div", { cls: "card" }, h("h3", null, "Add a word"), word, reading, mean, btn("Add", add, "primary")),
    h("div", { cls: "card" }, h("h3", null, "Bring words in or out"), area,
      h("div", { cls: "btn-row" }, btn("Import", () => { const n = Y.myImport(P, area.value, Y.today()); store.save(); toast(`Imported ${n} word(s).`, n ? "good" : ""); if (n) myWords(); }), btn("Show my list, for Anki", () => { area.value = Y.myExport(P); area.select(); }))),
    P.my.length ? list("Your words", P.my.map((m, i) => { const del = async () => { if (await sheet(`Delete ${m.word}?`, "You can't undo this.", [{ label: "Delete", cls: "danger" }]) === 0) { P.my.splice(i, 1); store.save(); myWords(); } };
      return { label: m.word, lang: "ja", sub: (m.reading !== m.word ? m.reading + " · " : "") + m.gloss, control: iconBtn("close", "Delete " + m.word, del, "quiet"),
        menu: [["Hear it", () => Host.speak(m.reading)], ["Copy", () => copy(m.word)], "-", ["Delete…", del]] }; })) :
      empty("plus", "No words yet", "Words you save from the dictionary or the Text Reader show up here."));
}
function genki() {
  setNav("cards"); back = cards;
  show(head("Genki practice", "By lesson, using YomiGo's own cards. Follows the order of the 3rd edition."),
    // Extra practice is free to repeat, so it earns no XP
    list(null, C.genki.map((g, i) => ({ label: "Lesson " + g.lesson, sub: g.topics, on: () => runQuiz(Y.quizGenki(i), "Genki practice", (r, n) => result("Practice done", `${r} of ${n} correct.`, r === n ? 2 : r * 2 >= n, genki)) }))));
}

// ---- exams and quest ----
function startExam(i) {
  const e = C.exams[i], missed = [];
  runQuiz(Y.quizExam(i), e.name, (r, n) => {
    const pct = Math.trunc(r * 100 / n), pass = Y.examResult(P, i, pct);
    if (!pass) for (const c of missed) if (P.cards[c.id]) P.cards[c.id].due = Y.today(); // missed cards come up in Flashcards straight away
    store.save();
    if (pass) return result(e.name, `You scored ${pct}%. The way forward is open.`, 2, home);
    // After a failed exam: go over what was missed, or take an unscored practice run of the same exam
    const practice = (qs, title) => runQuiz(qs, title, (r2, n2) => result(title, `You got ${r2} of ${n2} right.`, r2 === n2 ? 2 : r2 * 5 >= n2 * 4, home));
    setNav(""); UI.endRun(); sfx("bad");
    show(h("div", { cls: "prompt" }, h("div", { cls: "mark bad", lang: "ja" }, "×"), h("h1", null, e.name)), h("div", { cls: "card" }, `You scored ${pct}%. You need 80%. Practise what you missed, or take a practice test, then challenge it again. Missed cards are in Flashcards too.`),
      missed.length ? btn(L("Practise missed", "まちがいの練習", 3), () => practice(missed.slice(0, 15).map((c, k) => Y.makeQ(c, k % 3, Y.romajiMode(P.day))), "Practice")) : "",
      btn(L("Practice test", "れんしゅうテスト", 3), () => practice(Y.quizExam(i), "Practice test")),
      btn(L("Continue", "つぎへ", 2), home, "primary"));
  }, { noExit: true, onAnswer: (q, ok) => { if (!ok && q.card.id && !missed.includes(q.card)) missed.push(q.card); } });
}
function exams() {
  setNav("exam");
  const gate = Y.examGate(P);
  show(head(L("Exams", "しけん", 2), "Pass each gate with 80% to keep going."),
    list(null, C.exams.map((e, i) => {
      const open = e.day < P.day, s = P.exams[e.id] || 0;
      return { label: e.name, icon: "exam", tint: s >= Y.PASS ? "var(--good)" : open ? "var(--accent)" : "var(--ink-3)",
        value: h("span", { cls: "badge " + (s >= Y.PASS ? "good" : i === gate ? "bad" : "") }, s >= Y.PASS ? `Best ${s}%` : !open ? "Locked" : i === gate ? "Required" : "Open"), on: open ? () => startExam(i) : null };
    }), Host.tv ? "These are YomiGo's own exams, not official JLPT tests." : "These are YomiGo's own exams, not official JLPT tests. Stay in YomiGo while you take one: switching to another app counts that question as wrong."));
}
function quest() {
  setNav("quest");
  const r = Y.rankOf(P), ja = Y.stageOf(P) >= 3 && !P.en ? 1 : 0;
  const stat = (n, label) => h("div", { cls: "card tight", style: "text-align:center;margin:0" }, h("div", { cls: "stat" }, n), h("div", { cls: "dim small" }, label));
  show(head(L("Quest", "クエスト", 3), "Your road so far"),
    h("div", { cls: "card hero" }, h("div", { cls: "row" }, h("div", null, h("div", { cls: "eyebrow" }, "Rank"), h("div", { cls: "big acc", lang: ja ? "ja" : null }, RANKS[r][ja]),
      r + 1 < RANKS.length ? h("div", { cls: "dim" }, "Next: " + RANKS[r + 1][ja]) : h("div", { cls: "good" }, "The highest rank!")), ring(r, RANKS.length - 1, `${r}/${RANKS.length - 1}`))),
    h("div", { cls: "grid2", style: "grid-template-columns:repeat(auto-fit,minmax(130px,1fr))" }, stat(P.xp, "XP"), stat(P.best, "Best streak"), stat(P.day, "Classes"), stat(P.reviews, "Reviews"), stat(P.chats, "Conversations"), stat(P.hw, "Homework"), stat(P.perfects, "Perfect quizzes")));
}

// ---- library: dictionary, study guide, media, and on the PC the Material folder ----
const ensureDict = (then) => { if (globalThis.YG_DICT_TEXT) return then(); const s = document.createElement("script"); s.src = "dict.js"; s.onload = () => { Y.dictLoad(YG_DICT_TEXT); then(); }; document.head.append(s); };
const dictCard = (word, rd, gloss, sent) => {
  const add = btn([icon("plus"), "Add"], () => { const r = Y.myAdd(P, word, rd, gloss, sent || "", Y.today()); toast(addedMsg(r), r > 0 ? "good" : ""); if (r > 0) sfx("done"); add.disabled = true; store.save(); }, "sm", { "data-quiet": "1" });
  const card = h("div", { cls: "card tight" }, h("div", { cls: "row" }, h("div", null, h("strong", { lang: "ja", style: "font-size:20px" }, word || rd), h("span", { cls: "dim", lang: "ja" }, (word && rd !== word ? "  " + rd : "") + "  " + Y.kanaRomaji(rd))), add), h("div", null, gloss));
  card.addEventListener("contextmenu", (e) => UI.menu(e, [["Add to My words", () => add.click()], ["Copy", () => copy(word || rd)], "-", ["Hear it", () => Host.speak(rd)]]));
  return card;
};
const copy = (t) => navigator.clipboard.writeText(t).then(() => toast("Copied " + t), () => toast("Couldn't copy that.", "bad"));
let libTab = 0;
function library() {
  setNav("library");
  const tabs = ["Dictionary", "Guide", "Media", ...(Host.desktop ? ["Material"] : [])], pane = h("div");
  const panes = [() => {
    const q = field({ placeholder: "Japanese, romaji, or English", "aria-label": "Search the dictionary", lang: "ja", type: "search", enterkeyhint: "search" }), out = h("div");
    const search = () => { if (q.value.trim()) ensureDict(() => { const rows = Y.dictSearch(q.value, 20); out.replaceChildren(...(rows.length ? rows.map((r) => dictCard(r.word, r.reading, r.gloss)) : [empty("library", "Nothing found", "Try the plain form of the word.")])); }); };
    q.addEventListener("keydown", (e) => { if (e.key === "Enter") search(); });
    pane.replaceChildren(h("div", { cls: "field-row", style: "margin-top:12px" }, q, btn("Search", search, "primary")), out, h("p", { cls: "dim small" }, "About 22,000 common words from JMdict by EDRDG, CC BY-SA 4.0."));
    if (!Host.tv) q.focus();
  }, () => pane.replaceChildren(h("div", { style: "margin-top:16px" }), C.guide.map((g) => { const d = h("details", { cls: "card" }, h("summary", { tabindex: "0" }, g.title)); g.body.split("\n").forEach((p) => d.append(h("p", null, p))); return d; })),
  () => { const stage = Y.stageOf(P);
    pane.replaceChildren(h("div", { style: "margin-top:16px" }), C.media.filter((m) => m.stage <= stage + 1).map((m) => h("div", { cls: "card tight" }, h("strong", { lang: "ja" }, m.ja), h("div", { cls: "dim small" }, `${m.type}: ${m.title}`), h("p", { cls: "small" }, m.task))),
      h("p", { cls: "dim small" }, "Difficulty is YomiGo's own judgement. Check your local services for availability.")); },
  () => Host.call("material").then((files) => {
    const filt = field({ type: "search", placeholder: "Filter your files", "aria-label": "Filter" }), out = h("div");
    const draw = () => { const f = filt.value.toLowerCase(), rows = files.filter(([rel]) => rel.toLowerCase().includes(f)).slice(0, 200);
      out.replaceChildren(...(rows.length ? list(null, rows.map(([rel, full]) => ({ label: rel, icon: "folder", on: () => Host.open(full) }))) : [empty("folder", "No files", files.length ? "Nothing matches that." : "Put your books and audio in a folder named Material, beside YomiGo.")])); };
    filt.addEventListener("input", draw); draw();
    pane.replaceChildren(h("p", { cls: "dim" }, `${files.length} file(s). They open in your usual programs, and never leave this PC.`), filt, out);
  })];
  show(head(L("Library", "本だな", 5)), Host.desktop ? null : list(null, [{ label: L("Text Reader", "テキスト", 3), sub: "Paste text or read a photo, then tap to look words up", icon: "reader", on: () => textReader() }]),
    segmented(tabs, libTab, "Library section", (i) => { libTab = i; panes[i](); }), pane);
  panes[libTab]();
}

// Text reader: pasted text, a photo or screenshot, or what's said in a video. Pick a character to look up the word starting there.
let readerLines = [], watch = false, watchTimer = null, onTop = false, busy = "";
const addLines = (text) => { const l = text.split(/\r?\n/).map((s) => s.trim()).filter((s) => /[぀-ヿ㐀-鿿]/.test(s)); readerLines = [...l, ...readerLines].slice(0, 30); return l.length; };
globalThis.ygOcr = (text) => { const n = text ? addLines(text) : 0; textReader(); if (!text) toast("Couldn't read that picture.", "bad"); else if (!n) toast("No Japanese found in that picture."); };
function textReader() {
  setNav(Host.desktop ? "reader" : "library"); if (!Host.desktop) back = library;
  const area = h("textarea", { rows: 3, "aria-label": "Japanese text", placeholder: "Paste Japanese here", lang: "ja" }), out = h("div", { "aria-live": "polite" });
  const pick = (line, i, span) => ensureDict(() => {
    document.querySelectorAll(".reader-line span.on").forEach((s) => s.classList.remove("on")); span.classList.add("on");
    const hits = Y.dictLookup(line.slice(i)).slice(0, 4);
    out.replaceChildren(...(hits.length ? hits.map(({ card }) => dictCard(card.f, card.r, card.en, line)) : [h("p", { cls: "dim" }, "No match there.")]));
  });
  const lines = readerLines.map((line) => h("div", { cls: "card tight reader-line", lang: "ja" }, [...line].map((ch, k) => {
    const at = [...line].slice(0, k).join("").length, s = h("span", { role: "button", tabindex: "0", "aria-label": ch });
    s.textContent = ch; s.addEventListener("click", () => pick(line, at, s)); s.addEventListener("keydown", (e) => { if (e.key === "Enter") pick(line, at, s); });
    return s;
  })));
  lines.forEach((el, n) => el.addEventListener("contextmenu", (e) => UI.menu(e, [["Copy line", () => copy(readerLines[n])], ["Hear it", () => Host.speak(readerLines[n])], "-", ["Remove line", () => { readerLines.splice(n, 1); textReader(); }]])));
  const tools = Host.desktop ? h("div", { cls: "chips" },
    btn("Paste", async () => { const n = addLines(await Host.call("clip_text")); n ? textReader() : toast("There's no Japanese text on the clipboard."); }, "sm"),
    h("label", { cls: "chip" }, "Watch clipboard ", toggle(watch, "Watch the clipboard", (v) => { watch = v; startWatch(); if (v) toast("Japanese copied in any program now shows up here."); })),
    btn(busy === "ocr" ? "Reading…" : "Screenshot", readScreenshot, "sm", { title: "Take a screenshot with Win+Shift+S first", disabled: !!busy }),
    btn(busy === "listen" ? "Working…" : "Transcribe", transcribe, "sm", { title: "Write down what's said in an audio or video file", disabled: !!busy }),
    h("label", { cls: "chip" }, "Keep on top ", toggle(onTop, "Keep YomiGo on top", (v) => { onTop = v; Host.call("topmost", { on: v }); }))) : null;
  show(head(L("Text Reader", "テキスト", 3), Host.desktop ? "Copy Japanese from a game or page, or take a screenshot with Win+Shift+S." : "Paste Japanese, or read it from a photo. Then tap a character."),
    tools, area, h("div", { cls: "btn-row" }, btn("Add this text", () => { if (addLines(area.value)) textReader(); else toast("No Japanese in that text."); }, "primary"),
      Host.ocrPick ? btn("Read a photo", () => { toast("Reading…"); Host.ocrPick(); }) : null),
    out, lines.length ? lines : empty("reader", "Nothing to read yet", Host.desktop ? "With Watch on, text copied by a text hooker such as Textractor shows up by itself. Transcribe writes down what's said in a video." : "Add some Japanese above."),
    readerLines.length ? btn("Clear", () => { readerLines = []; textReader(); }, "quiet") : null);
}
function startWatch() {
  clearInterval(watchTimer); watchTimer = null;
  if (!watch) return;
  let seq = -1;
  watchTimer = setInterval(async () => { const s = await Host.call("clip_seq"); if (seq >= 0 && s !== seq && addLines(await Host.call("clip_text")) && document.querySelector(".reader-line, .empty")) textReader(); seq = s; }, 500);
}
async function needHelper(exe, what) {
  if (await Host.call("has", { name: exe })) return true;
  Host.call("fetch_helpers"); toast(`Downloading the ${what}. Try again in a minute.`); return false;
}
async function readScreenshot() {
  if (!(await needHelper("YomiGoOCR.exe", "screenshot reader"))) return;
  busy = "ocr"; textReader();
  const msg = { 100: "First take a screenshot with Win+Shift+S, then press this.", 2: "Windows has no Japanese text recognition. Add Japanese under Settings > Time & language > Language & region, then check its optical character recognition is installed.", 3: "Couldn't read that picture.", 4: "That picture is too large to read. Try a smaller part of the screen." };
  try { const t = await Host.call("ocr"); busy = ""; addLines(t) ? sfx("done") : toast("No Japanese text found in that picture."); }
  catch (code) { busy = ""; toast(msg[code] || "Couldn't start the text reader. It needs YomiGoOCR.exe beside YomiGo, and Windows 10 or later.", "bad"); }
  if (document.querySelector(".reader-line, .empty")) textReader();
}
async function transcribe() {
  if (!(await needHelper("YomiGoListen.exe", "transcriber"))) return;
  const get = !(await Host.call("has", { name: "models\\ggml-large-v3-turbo-q5_0.bin" }));
  if (get && await sheet("Download the speech model?", "Transcribing needs a 574 MB speech model, downloaded once from Hugging Face and checked before use.", [{ label: "Download", cls: "primary" }]) !== 0) return;
  busy = "listen"; textReader();
  const t = UI.toast; t(get ? "Downloading the speech model…" : "Pick a file. Long videos take a while: keep studying, and the lines show up here when it's done.");
  const tick = setInterval(async () => { const b = document.querySelector('button[title^="Write down"]'); if (b) b.textContent = `Working ${await Host.call("pct", { name: "listen" })}%`; }, 800);
  try {
    const [code, text] = await Host.call("listen", { get });
    const msg = { 2: "The speech model didn't load. Close some programs and try again.", 3: "Couldn't get the sound out of that file. MP3, M4A, WAV and MP4 work.", 4: "Transcribing needs a processor with AVX2, from about 2013 or later.", [-1073741795]: "Transcribing needs a processor with AVX2, from about 2013 or later.", 6: "No speech found in that file.", 7: "" };
    if (get) t(code === 0 ? "The speech model is ready. Press Transcribe and pick a file." : "The download failed or didn't check out, so nothing was kept. Try again later.", code === 0 ? "good" : "bad");
    else if (code === 0 || code === 8) { const n = addLines(text); sfx("done"); t(code === 0 ? `Done: ${n} lines. Japanese subtitles are saved beside the file, as .ja.srt.` : `Done: ${n} lines. A subtitle file was already beside it, so it was left alone.`, "good"); }
    else if (msg[code] !== "") t(msg[code] || "Transcribing needs 64-bit Windows 10 or later.", "bad");
  } catch { t("Couldn't start the transcriber. It needs YomiGoListen.exe beside YomiGo, and 64-bit Windows 10 or later.", "bad"); }
  clearInterval(tick); busy = "";
  if (document.querySelector(".reader-line, .empty")) textReader();
}

// ---- talk: open conversation with Sakura, a partner that runs on this PC ----
const tk = { msgs: [], wait: false, get: false, en: true };
const tkPart = (msg, tag) => { for (const raw of msg.split(/\r?\n/)) { const l = raw.replace(/^[\s*]+/, ""); if (l.startsWith(tag)) return l.slice(tag.length).trim(); } return ""; };
// What the partner is told: who it's talking to, the lesson, and the reply format the screen reads
function tkSystem() {
  const stage = Y.stageOf(P), d = C.days[Math.min(P.day, C.days.length - 1)];
  let s = "You are Sakura, a friendly young woman from Japan, chatting by text with someone learning Japanese. ";
  s += stage < 7 ? "They know about JLPT N5. Use simple polite Japanese with です and ます, and only common kanji." : stage < 8 ? "They are working on JLPT N4. Use polite Japanese, with some plain forms." : "They are working on JLPT N3. Use natural, everyday Japanese.";
  if (P.gender === 1 || P.gender === 2) s += P.gender === 1 ? " They are a woman." : " They are a man.";
  if (P.gender === 4) s += " They call themselves ぼく.";
  s += ` Today's lesson is: ${d.title}.`;
  const pats = d.new.map((id) => Y.byId[id]).filter((c) => c.t === "g").slice(0, 4).map((c) => c.f);
  if (pats.length) s += " Use these patterns when they fit: " + pats.join(", ");
  // What this learner has actually been taught, newest first, so Sakura's Japanese grows with theirs
  for (const [want, max, lead] of [["w", 40, " Words they've learned lately: "], ["g", 8, ". Grammar they've learned lately: "]]) {
    const got = [];
    for (let k = Math.min(P.day, C.days.length) - 1; k >= 0 && got.length < max; k--) for (const id of C.days[k].new) { if (got.length >= max) break; const c = Y.byId[id]; if (c.t === want) got.push(c.f); }
    if (got.length) s += lead + got.join(", ");
  }
  return s + ". Mostly use words they know, and no grammar much harder than theirs. Keep each reply to one or two short sentences, and end with a simple question. Write only your reply, in Japanese.";
}
async function tkAsk() {
  const esc = (t) => t.replace(/\r/g, "").replace(/\n/g, "\\n");
  let req = "S\t" + esc(tkSystem()) + "\n" + (tk.msgs.length ? "X\tfix en\n" : "X\ten\nU\t(Start the conversation: greet me and ask one simple question about today's lesson.)\n");
  for (const m of tk.msgs) req += m.role + "\t" + esc(m.role === "A" ? tkPart(m.text, "JA:") || m.text : m.text) + "\n";
  tk.wait = true; talk();
  try { tk.msgs.push({ role: "A", text: await Host.call("talk", { req }) }); sfx("ok"); }
  catch (code) { toast(code === -1 ? "Couldn't start the conversation partner. It needs YomiGoTalk.exe beside YomiGo, and 64-bit Windows 10 or later." : code === 2 ? "The conversation model didn't load. Close some programs and try again." : code === 4 || code === -1073741795 ? "Talking needs a processor with AVX2, from about 2013 or later." : "The conversation partner stopped. Press New topic to try again.", "bad"); }
  tk.wait = false;
  if (document.querySelector(".talk")) talk();
}
async function talk() {
  setNav("talk");
  const head2 = head(L("Talk", "かいわ", 3), "Chat freely with Sakura. Everything runs on this PC.",
    h("div", { cls: "chips", style: "margin:0" }, h("label", { cls: "chip" }, "English ", toggle(tk.en, "Show English", (v) => { tk.en = v; talk(); })),
      btn("New topic", () => { if (!tk.wait && !tk.get) { tk.msgs = []; talk(); } }, "sm", { disabled: tk.wait || tk.get })));
  // A small model writes poor beginner Japanese, and talking needs sentences, so it opens at the Grammar Gate
  if (Y.stageOf(P) < 4) return show(head(L("Talk", "かいわ", 3)), empty("talk", "Talk opens at the Grammar Gate", "Once you can build sentences, you can chat freely with Sakura, a partner that runs on this PC. Until then, each class's conversation is the place to practise."));
  if (!(await Host.call("has", { name: "YomiGoTalk.exe" }))) { Host.call("fetch_helpers"); return show(head2, empty("talk", "Getting Sakura ready", "Downloading the conversation partner. Come back in a minute.")); }
  const model = await Host.call("has", { name: "models\\gemma-4-E4B_q4_0-it.gguf" });
  if (!model && !tk.msgs.length) {
    const pct = h("span", null, "");
    show(head2, tk.get ? h("div", { cls: "card" }, h("h3", null, "Downloading the model…"), progress(0, 100), pct) :
      empty("talk", "Meet Sakura", "Talking needs a 5.2 GB model, downloaded once from Hugging Face and checked before use. Everything then runs on this PC, and nothing you write is sent anywhere.",
        btn("Download and start", async () => { tk.get = true; talk(); const code = await Host.call("talk_get").catch(() => -1); tk.get = false; if (code) toast("The download failed or didn't check out, so nothing was kept. Try again later.", "bad"); if (document.querySelector(".talk, .empty")) talk(); }, "primary")));
    app.firstChild.classList.add("talk");
    if (tk.get) { const t = setInterval(async () => { const v = await Host.call("pct", { name: "talk" }); pct.textContent = v + "%"; const b = app.querySelector(".progress i"); if (b) b.style.width = v + "%"; }, 700); onLeave(() => clearInterval(t)); }
    return;
  }
  if (!tk.msgs.length && !tk.wait) return tkAsk();
  const inp = field({ lang: "ja", placeholder: tk.wait ? "Sakura is typing…" : "Write in Japanese", "aria-label": "Your message", enterkeyhint: "send", disabled: tk.wait });
  const send = () => { if (tk.wait || !inp.value.trim()) return; tk.msgs.push({ role: "U", text: inp.value.trim() }); sfx("select"); tkAsk(); };
  inp.addEventListener("keydown", (e) => { if (e.key === "Enter") send(); });
  show(head2, h("div", { cls: "chat talk" }, tk.msgs.map((m) => {
    if (m.role === "U") return h("div", { cls: "msg you", lang: "ja" }, h("div", { cls: "who" }, "You"), m.text);
    const ja = tkPart(m.text, "JA:") || m.text, en = tkPart(m.text, "EN:"), fix = tkPart(m.text, "FIX:");
    return h("div", { cls: "msg them" }, h("div", { cls: "who" }, "Sakura"), h("div", { lang: "ja", style: "font-size:18px" }, ja), tk.en && en ? h("div", { cls: "en" }, en) : null,
      fix && !fix.startsWith("OK") ? h("div", { cls: "fix", lang: "ja" }, "Maybe better: " + fix) : null, Host.canSpeak() ? iconBtn("speaker", "Hear it", () => Host.speak(ja), "quiet") : null);
  }), tk.wait ? h("div", { cls: "msg them typing", "aria-label": "Sakura is typing" }, h("i"), h("i"), h("i")) : null),
    h("div", { cls: "composer" }, inp, iconBtn("send", "Send", send, "primary")));
  app.lastChild.scrollIntoView({ block: "end" });
  if (!tk.wait) inp.focus();
}

// ---- sync ----
// Phone: sends its copy to the PC, which merges the two. PC: holds the learner's save while this screen is open, and merges what a phone sends.
function syncScreen() {
  setNav("settings"); back = settings;
  if (Host.desktop) return syncHost();
  let last = ""; try { last = localStorage.getItem("yg.syncAddr") || ""; } catch {}
  const addr = field({ placeholder: "PC address, like 192.168.1.23", inputmode: "decimal", "aria-label": "PC address", value: last }),
    code = field({ placeholder: "4-digit code", inputmode: "numeric", maxlength: 4, "aria-label": "Code" }), msg = h("p", { cls: "dim", role: "status" });
  const run = btn([icon("sync"), L("Sync", "どうき", 4)], async () => {
    const a = addr.value.trim(), c = code.value.trim();
    if (!/^[\d.]+$/.test(a) || !/^\d{4}$/.test(c)) { msg.textContent = "Enter the address and the 4-digit code the PC shows."; return sfx("error"); }
    run.disabled = true; msg.textContent = "Syncing…";
    try {
      const r = await fetch(`http://${a}:47820/sync`, { method: "POST", headers: { "Content-Type": "text/plain" }, body: `YOMIGO-SYNC 1 ${c}\n` + Y.saveText(P) });
      const text = await r.text();
      if (r.status === 403) msg.textContent = "Wrong code. Check the code on the PC's Sync screen.";
      else if (!r.ok) msg.textContent = "The PC couldn't use this save: " + text;
      else {
        const p = Y.loadText(text);
        if (!p) throw new Error("bad reply");
        P = p; store.save();
        try { localStorage.setItem("yg.syncAddr", a); } catch {}
        return result(L("Synced", "どうきしました", 4), `${P.name}'s progress is now the same on this phone and the PC.`, true, home);
      }
    } catch { msg.textContent = "Couldn't reach the PC. Check both are on the same Wi-Fi, the PC's Sync screen is open, and Windows allowed YomiGo through its firewall."; }
    sfx("error"); run.disabled = false;
  }, "primary");
  show(head(L("Sync with your PC", "どうき", 4)), h("p", null, "On the PC or TV, open Settings, then Sync. Enter the address and code it shows. Use the same learner name on both."), h("div", { cls: "card" }, addr, code, run, msg));
}
const SAFE_NAME = /^[A-Za-z0-9_-]{1,31}$/;
async function syncHost() {
  back = () => settings();
  const code = 1000 + Math.floor(Math.random() * 9000), note = h("p", { role: "status", cls: "good" });
  let wrong = 0, addr;
  try { addr = await Host.call("sync_open"); }
  catch { return show(head(L("Sync", "どうき", 4)), empty("sync", "Couldn't open the home network", "Check this PC is on Wi-Fi or a cable, then try again.")); }
  const stop = await Host.on("sync", (req) => {
    const nl = req.indexOf("\n"), headLine = nl < 0 ? req : req.slice(0, nl), body = nl < 0 ? "" : req.slice(nl + 1);
    if (!headLine.startsWith("YOMIGO-SYNC 1 ")) return Host.call("sync_reply", { status: 400, body: "Not a YomiGo sync" });
    if (+headLine.slice(14) !== code) {
      Host.call("sync_reply", { status: 403, body: "Wrong code" });
      if (++wrong >= 3) { Host.call("sync_close"); show(head(L("Sync", "どうき", 4)), empty("sync", "Sync stopped", "Three wrong codes, so sync stopped. Open it again for a new code.", btn("Open again", syncScreen, "primary"))); }
      else { note.className = "bad"; note.textContent = "A phone tried a wrong code."; sfx("error"); }
      return;
    }
    const theirs = Y.loadText(body);
    if (!theirs || !SAFE_NAME.test(theirs.name)) return Host.call("sync_reply", { status: 400, body: "That isn't a YomiGo save" });
    let mine;
    if (P && P.name === theirs.name) { mine = Y.merge(P, theirs); store.save(); }
    else { const old = store.load(theirs.name); mine = old ? Y.merge(old, theirs) : theirs; Host.store.write(mine.name, Y.saveText(mine)); }
    Host.call("sync_reply", { status: 200, body: Y.saveText(mine) });
    note.className = "good"; note.textContent = `Synced ${theirs.name} with a phone.`; sfx("done");
  });
  onLeave(() => { stop(); Host.call("sync_close"); });
  show(head(L("Sync with your phone", "どうき", 4), "Keep this screen open until the phone says it's done."),
    h("div", { cls: "card hero" }, h("p", null, "On your phone, open YomiGo, then Settings > Sync with your PC, and enter:"),
      h("div", { cls: "grid2" }, h("div", null, h("div", { cls: "eyebrow" }, "Address"), h("div", { cls: "stat acc" }, addr)), h("div", null, h("div", { cls: "eyebrow" }, "Code"), h("div", { cls: "stat acc" }, code))), note),
    h("p", { cls: "dim small" }, "Both need the same Wi-Fi. If Windows asks, allow YomiGo on private networks. Use the same learner name on both."));
}

// ---- settings ----
const theme = () => { try { return localStorage.getItem("yg.theme") || "system"; } catch { return "system"; } };
function applyTheme(animate) {
  const t = theme();
  if (animate) { root.classList.add("theming"); setTimeout(() => root.classList.remove("theming"), 320); }
  if (t === "system") delete root.dataset.theme; else root.dataset.theme = t;
}
function settings() {
  setNav("settings");
  const area = h("textarea", { rows: 4, "aria-label": "Save text", placeholder: "Paste a YomiGo save here" });
  const stamp = () => { const d = new Date(); return `${d.getFullYear()}${String(d.getMonth() + 1).padStart(2, "0")}${String(d.getDate()).padStart(2, "0")}`; };
  show(head(L("Settings", "せってい", 2)),
    list("Learning", [
      { label: "Pace", sub: ["Gentle: one class a day", "Steady: two a day", "Sprint: three a day"][P.pace - 1], control: segmented(["1", "2", "3"], P.pace - 1, "Classes a day", (i) => { P.pace = i + 1; store.save(); settings(); }) },
      { label: "Labels in Japanese", sub: "Menus switch to Japanese as you learn", control: toggle(!P.en, "Labels in Japanese", (v) => { P.en = v ? 0 : 1; store.save(); setNav("settings"); }) },
      { label: "Genki practice", control: toggle(P.genki, "Genki practice", (v) => { P.genki = v ? 1 : 0; store.save(); }) },
      { label: "Who's learning", value: GENDER[P.gender], on: () => askSpeaker(settings) }]),
    list("Sound and look", [
      { label: "Sounds", control: toggle(!UI.muted(), "Sounds", (v) => { try { localStorage.setItem("yg.mute", v ? "0" : "1"); } catch {} if (v) sfx("done"); }) },
      Host.tv ? null : { label: "Appearance", control: segmented(["Auto", "Light", "Dark"], ["system", "light", "dark"].indexOf(theme()), "Appearance", (i) => { try { localStorage.setItem("yg.theme", ["system", "light", "dark"][i]); } catch {} applyTheme(true); }) },
      { label: "Japanese voice", value: Host.canSpeak() ? "Found" : "None", on: Host.canSpeak() ? null : Host.voiceHelp }]),
    Host.tv && typeof TV !== "undefined" ? TV.settingsRows() : null,
    list("Devices", [
      { label: Host.desktop ? "Sync with your phone" : "Sync with your PC", icon: "sync", tint: "#30b0c7", on: syncScreen },
      Host.desktop ? { label: "Back up my save", icon: "folder", tint: "#ff9f0a", on: () => { store.save(); Host.call("backup", { name: P.name, stamp: stamp() }).then(() => toast("A dated copy of your save is in the backup folder, beside YomiGo.", "good"), (e) => toast("Couldn't back up: " + e, "bad")); } } : null,
      { label: "Switch learner", icon: "person", tint: "#5e5ce6", on: profiles }]),
    list("About", [
      { label: "Check for updates", on: Host.checkUpdate },
      { label: "YomiGo on GitHub", on: () => Host.open("https://github.com/knivesbatz/YomiGO") },
      { label: "Version", value: `${Host.tv ? "TV" : Host.desktop ? "Windows" : Host.android ? "Android" : "Web"} ${Host.version}` }],
      "Speech feedback checks what the recogniser heard. It can't confirm native-like pronunciation."),
    h("div", { cls: "list-h" }, "Move a save between devices"),
    h("div", { cls: "card" }, area, h("div", { cls: "btn-row" }, btn("Show my save", () => { area.value = Y.saveText(P); area.select(); }),
      btn("Load the save above", async () => { const p = Y.loadText(area.value); if (!p) return toast("That isn't a YomiGo save.", "bad"); if (await sheet(`Replace ${p.name}'s progress?`, "The save on this device is overwritten.", [{ label: "Replace", cls: "danger" }]) === 0) { P = p; store.save(); home(); } }))));
}

applyTheme();
// Big Sur everywhere but the TV. On Windows the window draws its own chrome: a toolbar and sidebar that drag it.
function chrome() {
  if (Host.tv) return;
  root.classList.add("mac");
  if (!Host.desktop) return;
  root.classList.add("desk");
  const W = __TAURI__.window.getCurrentWindow();
  // The toolbar and the sidebar's empty space move the window; a double-click zooms it, as on a Mac.
  // There are no window buttons: right-click there for Minimise, Zoom and Close, or use Alt+F4 and the taskbar.
  const bare = (e) => !e.target.closest("button, input, textarea, label, a, [role=button], .seg, .switch, .tab");
  const drag = (e) => { if (e.button || !bare(e)) return; e.preventDefault(); e.detail === 2 ? W.toggleMaximize() : W.startDragging(); };
  const winMenu = (e) => { if (bare(e)) UI.menu(e, [["Minimise", () => W.minimize()], ["Zoom", () => W.toggleMaximize()], "-", ["Close", () => W.close(), "Alt+F4"]]); };
  for (const el of [toolbar, nav]) { el.addEventListener("mousedown", drag); el.addEventListener("contextmenu", winMenu); }
  addEventListener("focus", () => root.classList.remove("inactive")); addEventListener("blur", () => root.classList.add("inactive"));
  // Right-click shows YomiGo's own menus; the browser's own menu would give the web page away
  document.addEventListener("contextmenu", (e) => { if (!e.defaultPrevented && !e.target.closest("input, textarea")) e.preventDefault(); });
}
Host.ready.then(() => { chrome(); if (Host.tv && typeof TV !== "undefined") TV.start(); profiles(); });
