// YomiGo shared core for Mobile and the browser extension. Mirrors src/core.c; tests/test_core.mjs holds both to tests/vectors.json.
(function (root) {
  const C = root.YG_CONTENT;
  const romaKeys = C ? Object.keys(C.roma).sort((a, b) => b.length - a.length) : [];
  const subs = C ? C.subs : [];
  const VOWELS = { あかさたなはまやらわがざだばぱぁゃ: "あ", いきしちにひみりぎじぢびぴぃ: "い", うくすつぬふむゆるぐずづぶぷぅゅ: "う", えけせてねへめれげぜでべぺぇ: "え", おこそとのほもよろをごぞどぼぽぉょ: "お" };
  const vowelOf = (k) => { for (const row in VOWELS) if (row.includes(k)) return VOWELS[row]; return ""; };

  function normKana(input) {
    // A Japanese keyboard types full-width letters, spaces and punctuation: they count the same as plain ones
    let s = input.replace(/[！-～]/g, (ch) => String.fromCharCode(ch.charCodeAt(0) - 0xfee0)).replace(/[　・]/g, " ").replace(/、/g, ",").replace(/。/g, ".");
    for (const [from, to] of subs) s = s.split(from).join(to);
    s = s.toLowerCase().replace(/[ ,.!?]/g, "");
    let out = "", i = 0;
    while (i < s.length) {
      if (s.charCodeAt(i) >= 0x80) { out += s[i++]; continue; }
      const k = romaKeys.find((key) => s.startsWith(key, i));
      if (k) { out += C.roma[k]; i += k.length; continue; }
      const c = s[i], nx = s[i + 1];
      if (c === "n") {
        if (nx === "'") i += 2;
        else if (nx === "n" && !"aiueoy".includes(s[i + 2] ?? "\0")) i += 2;
        else i += 1;
        out += "ん";
      } else if (c === "'") i++;
      else if (c === "-") { out += "ー"; i++; }
      else if (c >= "a" && c <= "z" && (nx === c || (c === "t" && nx === "c"))) { out += "っ"; i++; }
      else { out += c; i++; }
    }
    let res = "", last = "";
    for (const ch of out) {
      let c = ch, n = c.codePointAt(0);
      if (n >= 0x30a1 && n <= 0x30f6) c = String.fromCodePoint(n - 0x60);
      if (c === "ー") { c = vowelOf(last); if (!c) continue; }
      if ("。、！？　・".includes(c)) continue;
      res += c; last = c;
    }
    return res;
  }
  function answerOk(input, want, romaji) {
    const a = normKana(input);
    if (!a) return false;
    // A word's final は or へ, as in こんにちは, is said wa or e, so it may be typed that way. A lone kana is still read as written.
    const tail = (s) => s.slice(0, -1) + sound(s.slice(-1)), w = normKana(want);
    return a === w || (!!romaji && a === normKana(romaji)) || (a.length > 1 && w.length > 1 && tail(a) === tail(w));
  }
  const sound = (s) => s.replace(/は/g, "わ").replace(/を/g, "お").replace(/へ/g, "え");
  function chatOk(input, turn) {
    const a = sound(normKana(input));
    return !!a && turn.want.some((w) => sound(normKana(w)) === a);
  }

  // Integer SM-2, the same as srs_grade in src/core.c
  const srsNew = (today) => ({ reps: 0, ivl: 0, lapses: 0, ease: 2500, due: today });
  function srsGrade(c, g, today) {
    let ivl = c.ivl, ease = c.ease;
    const idiv = (a, b) => Math.trunc(a / b);
    if (g === 0) { c.reps = 0; ivl = 0; ease -= 200; c.lapses++; }
    else {
      if (g === 1) { ivl = idiv(ivl * 12, 10); if (ivl < 1) ivl = 1; ease -= 150; }
      else if (g === 2) ivl = c.reps === 0 ? 1 : c.reps === 1 ? 3 : idiv(ivl * ease, 1000);
      else { ivl = c.reps === 0 ? 3 : c.reps === 1 ? 6 : idiv(idiv(ivl * ease, 1000) * 13, 10); ease += 150; }
      c.reps++;
    }
    if (ease < 1300) ease = 1300;
    if (ivl > 3650) ivl = 3650;
    c.ivl = ivl; c.ease = ease; c.due = today + ivl;
  }

  // Save file: the same text format as the Windows builds, so a save moves between devices
  const MY_MAX = 2000;
  const newProfile = (name) => ({ name, xp: 0, streak: 0, best: 0, last: 0, today: 0, placed: 0, day: 0, step: 0, pace: 1, en: 0, genki: 0, gender: 0, reviews: 0, perfects: 0, chats: 0, hw: 0, ach: 0, reads: 0, listens: 0, exams: {}, cards: {}, my: [] });
  const NUMS = ["xp", "streak", "best", "last", "today", "placed", "day", "step", "pace", "en", "genki", "gender", "reviews", "perfects", "chats", "hw", "ach", "reads", "listens"];
  function saveText(p) {
    let t = "YOMIGO-SAVE 1\nname=" + p.name + "\n";
    for (const k of NUMS) t += k + "=" + p[k] + "\n";
    for (const e in p.exams) if (p.exams[e]) t += "exam." + e + "=" + p.exams[e] + "\n";
    for (const id in p.cards) { const c = p.cards[id]; t += `card ${id} ${c.reps} ${c.ivl} ${c.lapses} ${c.ease} ${c.due}\n`; }
    for (const m of p.my) t += `my ${m.reps} ${m.ivl} ${m.lapses} ${m.ease} ${m.due}\t${m.word}\t${m.reading}\t${m.gloss}\t${m.sent}\n`;
    return t;
  }
  const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
  function loadText(text) {
    if (!text.startsWith("YOMIGO-SAVE 1\n")) return null;
    const p = newProfile(""), ids = new Set(C.cards.map((c) => c.id)), examIds = new Set(C.exams.map((e) => e.id));
    for (const raw of text.split("\n").slice(1)) {
      const line = raw.replace(/\r$/, "");
      if (line.startsWith("card ")) {
        const [, id, r, i, l, e, d] = line.split(" ");
        if (ids.has(id)) p.cards[id] = { reps: +r || 0, ivl: clamp(+i || 0, 0, 3650), lapses: +l || 0, ease: clamp(+e || 0, 1300, 5000), due: +d || 0 };
        continue;
      }
      if (line.startsWith("my ")) {
        const [head, word, reading, gloss, sent] = line.split("\t"), [, r, i, l, e, d] = head.split(" ");
        if (word && gloss && p.my.length < MY_MAX) p.my.push({ word, reading: reading || word, gloss, sent: sent || "", reps: +r || 0, ivl: clamp(+i || 0, 0, 3650), lapses: +l || 0, ease: clamp(+e || 0, 1300, 5000), due: +d || 0 });
        continue;
      }
      const eq = line.indexOf("=");
      if (eq < 0) continue;
      const k = line.slice(0, eq), v = line.slice(eq + 1);
      if (k === "name") p.name = v.slice(0, 31);
      else if (k.startsWith("exam.")) { if (examIds.has(k.slice(5))) p.exams[k.slice(5)] = clamp(+v || 0, 0, 100); }
      else if (NUMS.includes(k)) p[k] = +v || 0;
    }
    p.day = clamp(p.day, 0, C.days.length); p.step = p.step > 10 || p.step < 0 ? 0 : p.step; p.pace = clamp(p.pace, 1, 3);
    return p.name ? p : null;
  }

  const PASS = 80;
  const today = () => { const n = new Date(); let y = n.getFullYear(), m = n.getMonth() + 1; const d = n.getDate(); if (m <= 2) { y--; m += 12; }
    return 365 * y + Math.trunc(y / 4) - Math.trunc(y / 100) + Math.trunc(y / 400) + Math.trunc((153 * (m - 3) + 2) / 5) + d; };
  const stageOf = (p) => C.days[Math.min(p.day, C.days.length - 1)].stage;
  const romajiMode = (day) => day < C.days.length && C.days[day].stage <= 1;
  const examGate = (p) => C.exams.findIndex((e) => e.day < p.day && (p.exams[e.id] || 0) < PASS);
  const rankOf = (p) => { let r = 0; C.exams.forEach((e, i) => { if ((p.exams[e.id] || 0) >= PASS) r = i + 1; }); return r; };
  const dueCount = (p, t) => Object.values(p.cards).filter((c) => c.due <= t).length + p.my.filter((m) => m.due <= t).length;
  // Your own words. Tabs and line breaks can't get in, so a field can't break a save line.
  const field = (s, max) => (s || "").replace(/[\t\r\n]+/g, " ").trim().slice(0, max);
  function myAdd(p, word, reading, gloss, sent, t) {
    let w = field(word, 39), r = field(reading, 59);
    const g = field(gloss, 91), s = field(sent, 119);
    if (!r) r = w;
    if (!w) w = r;
    if (!w || !g) return -1;
    if (p.my.some((m) => m.word === w && m.reading === r)) return 0;
    if (p.my.length >= MY_MAX) return -1;
    p.my.push({ word: w, reading: r, gloss: g, sent: s, ...srsNew(t) });
    return 1;
  }
  function myImport(p, text, t) {
    let n = 0;
    for (const line of text.split(/\r?\n/)) {
      if (!line || line[0] === "#") continue;
      const [w, r, g, s] = line.split("\t");
      if (myAdd(p, w, r, g, s, t) === 1) n++;
    }
    return n;
  }
  const myExport = (p) => p.my.map((m) => [m.word, m.reading, m.gloss, m.sent].join("\t")).join("\n");
  function learnDay(p, day, t) { for (const id of C.days[day].new) if (!p.cards[id]) p.cards[id] = srsNew(t); }
  function completeDay(p, t) {
    if (p.last !== t) { p.streak = p.last === t - 1 ? p.streak + 1 : 1; p.today = 0; p.last = t; }
    if (p.streak > p.best) p.best = p.streak;
    p.today++; p.xp += 100; p.day++; p.step = 0;
  }
  function examResult(p, ex, score) {
    const e = C.exams[ex];
    if (score > (p.exams[e.id] || 0)) p.exams[e.id] = score;
    const pass = score >= PASS;
    p.xp += pass ? 200 * e.stars : 20;
    return pass;
  }
  // An update can add cards to classes already taken. They join the reviews, ten a day, rather than being missed.
  function catchUp(p, t) {
    let n = 0;
    for (let d = 0; d < p.day && d < C.days.length; d++) for (const id of C.days[d].new) if (!p.cards[id]) { p.cards[id] = { ...srsNew(t), due: t + Math.floor(n / 10) }; n++; }
    return n;
  }
  function place(p, stage, t) {
    p.day = C.stages[Math.min(stage, C.stages.length - 1)].first; p.step = 0; p.placed = 1;
    for (let d = 0; d < p.day; d++) C.days[d].new.forEach((id, j) => { p.cards[id] = { ...srsNew(t), reps: 2, ivl: 3, due: t + 1 + (j % 5) }; });
    C.exams.forEach((e) => { if (e.day < p.day) p.exams[e.id] = PASS; });
  }

  // Questions
  const rnd = (n) => Math.floor(Math.random() * n);
  const byId = C ? Object.fromEntries(C.cards.map((c) => [c.id, c])) : {};
  function shuffle(a) { for (let i = a.length - 1; i > 0; i--) { const j = rnd(i + 1); [a[i], a[j]] = [a[j], a[i]]; } return a; }
  function choices(card, right, pick) {
    const out = [right];
    // 時 is both じ and とき: its other meaning isn't a wrong answer
    const pool = shuffle(C.cards.filter((o) => o.t === card.t && o.day >= 0 && (o === card || o.f !== card.f)));
    // Wrong answers of the same kind first (a verb gets verbs, as in the C core), then from nearby classes
    const kind = (o) => (o.t !== "w" ? "" : o.en.startsWith("to ") ? "v" : o.f.endsWith("い") ? "a" : "n"), k = kind(card);
    const far = (o) => (Math.abs(o.day - card.day) > 8 ? 1 : 0);
    pool.sort((a, b) => ((kind(a) !== k) - (kind(b) !== k)) * 2 + far(a) - far(b));
    for (const o of pool) { const t = pick(o); if (t && !out.includes(t)) out.push(t); if (out.length === 4) break; }
    return shuffle(out);
  }
  function makeQ(card, pref, rom) {
    const q = { card, kind: "choice", sub: "", prompt: "", tag: "", choices: [], answer: "", want: "", want2: "" };
    if (card.t === "g") { q.prompt = card.s; q.sub = card.tr; q.tag = "Fill in the blank"; q.answer = card.r; q.choices = shuffle([card.r, ...card.w]); return q; }
    const set = (prompt, tag, right, pick) => { q.prompt = prompt; q.tag = tag; q.answer = right; q.choices = choices(card, right, pick); };
    if (card.t === "h" || card.t === "k") {
      if (pref === 0) set(card.f, "How is this read?", card.ro, (o) => o.ro);
      else if (pref === 1) set(card.ro, card.t === "h" ? "Which hiragana?" : "Which katakana?", card.f, (o) => o.f);
      else { q.kind = "type"; q.prompt = card.f; q.tag = "Type the sound"; q.want = card.ro; q.want2 = card.r; }
      return q;
    }
    if (card.t === "j") {
      if (pref === 0) set(card.f, "What does it mean?", card.en, (o) => o.en);
      else if (pref === 1) set(card.en, "Which kanji?", card.f, (o) => o.f);
      else set(card.f, "Choose the reading", card.r, (o) => o.r);
      return q;
    }
    const face = (o) => (rom ? o.ro : o.f);
    if (pref === 0) { set(face(card), "What does it mean?", card.en, (o) => o.en); if (!rom && card.f !== card.r) q.sub = card.r; }
    else if (pref === 1) set(card.en, "Which word?", face(card), face);
    else { q.kind = "type"; q.prompt = card.en; q.tag = rom ? "Type it in romaji or kana" : "Type the reading"; q.want = card.r; q.want2 = card.ro; }
    return q;
  }
  const q3 = (id, rom) => makeQ(byId[id], rnd(3), rom);
  function quizWarmup(p) {
    const rom = romajiMode(p.day), out = [];
    if (p.day > 0) C.days[p.day - 1].new.slice(0, 5).forEach((id) => out.push(q3(id, rom)));
    const old = shuffle(Object.keys(p.cards).filter((id) => byId[id].day < p.day && !(rom && "hk".includes(byId[id].t)))).slice(0, 3);
    old.forEach((id) => out.push(q3(id, rom)));
    return shuffle(out);
  }
  function quizDay(day) {
    const rom = romajiMode(day), ids = shuffle([...C.days[day].new]).slice(0, 20), out = ids.map((id) => q3(id, rom));
    for (let b = day - 1; b >= 0 && out.length < 6; b--) C.days[b].new.slice(0, 4).forEach((id) => out.push(q3(id, rom)));
    return shuffle(out);
  }
  function quizExam(ex) {
    const e = C.exams[ex], stage = C.days[e.day].stage, rom = stage <= 1, want = e.stars >= 4 ? 40 : e.stars >= 3 ? 25 : 15;
    const first = e.stars >= 5 ? 0 : C.stages[stage].first;
    const inStage = C.cards.filter((c) => c.day >= first && c.day <= e.day && !(rom && "hk".includes(c.t)));
    const earlier = C.cards.filter((c) => c.day >= 0 && c.day < first && !(rom && "hk".includes(c.t)));
    return shuffle([...shuffle(inStage).slice(0, want), ...shuffle(earlier).slice(0, 3)]).map((c) => makeQ(c, rnd(3), rom));
  }
  function makeQMy(m, pref) {
    const card = { id: "my", t: "m", f: m.word, r: m.reading, ro: "", en: m.gloss, n: m.sent };
    const q = { card, my: m, kind: "choice", sub: "", prompt: m.word, tag: "Your word: what does it mean?", choices: [], answer: m.gloss, want: "", want2: "" };
    if (pref === 1) { q.kind = "type"; q.prompt = m.gloss; q.tag = "Your word: type the reading"; q.want = m.reading; return q; }
    q.sub = m.sent;
    const wrong = shuffle(C.cards.filter((c) => c.t === "w" && c.day >= 0 && c.en !== m.gloss)).slice(0, 12).map((c) => c.en);
    q.choices = shuffle([m.gloss, ...[...new Set(wrong)].slice(0, 3)]);
    return q;
  }
  // Everything due is shuffled together, so your own words can't be crowded out by the course
  function quizReview(p, t, max) {
    const due = [...Object.keys(p.cards).filter((id) => p.cards[id].due <= t), ...p.my.filter((m) => m.due <= t)];
    return shuffle(due).slice(0, max).map((d) => (typeof d === "string" ? q3(d, romajiMode(byId[d].day) && romajiMode(p.day)) : makeQMy(d, rnd(2))));
  }
  // Genki practice: YomiGo's own cards, grouped by the lesson of the book that covers them
  const quizGenki = (i) => shuffle(C.genki[i].cards.map((id) => q3(id, false))).slice(0, 15);
  // Hear it, then choose what it meant; every third one is dictation. Sentences come from the day's grammar.
  const filled = (c) => c.s.replace("＿", c.r);
  function quizListen(day) {
    const rom = romajiMode(day), out = [];
    for (let b = day; b >= 0 && out.length < 6; b--) for (const id of C.days[b].new) {
      if (out.length >= 8) break;
      const c = byId[id];
      if (c.t === "j" || ("hk".includes(c.t) && rom)) continue;
      const q = makeQ(c, 0, rom);
      q.tag = "What did you hear?"; q.sub = "";
      if (c.t === "g") {
        const wrong = shuffle(C.cards.filter((o) => o.t === "g" && o.day >= 0 && o.tr !== c.tr)).slice(0, 3).map((o) => o.tr);
        q.say = q.prompt = filled(c); q.answer = c.tr; q.choices = shuffle([c.tr, ...wrong]);
      } else if (out.length % 3 === 2) {
        q.kind = "type"; q.tag = "Type what you hear"; q.say = c.r; q.prompt = c.f; q.want = c.r; q.want2 = c.ro; q.choices = [];
      } else { q.say = c.r; q.prompt = "hk".includes(c.t) ? c.f : rom ? c.ro : c.f; }
      out.push(q);
    }
    return shuffle(out);
  }
  // Lines to repeat aloud: the day's conversation, then its grammar sentences, then its words
  function shadowLines(day, max = 4) {
    const d = C.days[day], out = [];
    if (d.chat) for (const t of C.chats[d.chat].turns) if (t.say) out.push({ jp: t.say, ro: t.ro, en: t.en });
    for (const id of d.new) if (byId[id].t === "g") out.push({ jp: filled(byId[id]), ro: "", en: byId[id].tr });
    for (const id of d.new) if (byId[id].t === "w") out.push({ jp: byId[id].f, ro: byId[id].ro, en: byId[id].en });
    return out.slice(0, max);
  }
  // Always starts at romaji
  function placementSection(i) {
    const type = "whkwgj"[i];
    return shuffle(C.cards.filter((c) => c.t === type && c.day >= 0 && (i > 0 || romajiMode(c.day)))).slice(0, 5).map((c) => makeQ(c, 0, i === 0));
  }

  // Full dictionary: lines of word, reading, hiragana, meaning. Loaded only when wanted.
  let dictRows = null, dictIndex = null;
  function dictLoad(text) {
    dictRows = text.split("\n").filter(Boolean).map((l) => { const [word, reading, hira, gloss] = l.split("\t"); return { word, reading, key: hira || reading, gloss }; });
    dictIndex = new Map();
    const put = (k, row) => { if (!k) return; const a = dictIndex.get(k); if (a) a.push(row); else dictIndex.set(k, [row]); };
    for (const r of dictRows) { put(r.word, r); put(r.key, r); if (r.reading !== r.key) put(r.reading, r); } // katakana words are found as written
    return dictRows.length;
  }
  const glossTier = (g, q, tier) => {
    const lower = g.toLowerCase(), isA = (c) => c >= "a" && c <= "z";
    for (let i = lower.indexOf(q); i >= 0; i = lower.indexOf(q, i + 1)) {
      const after = lower[i + q.length], wholeEnd = after === undefined || !isA(after);
      if (tier === 2 && (i === 0 || lower.slice(i - 2, i) === "; ") && wholeEnd) return true;
      if (tier === 3 && (i === 0 || !isA(lower[i - 1])) && wholeEnd) return true;
      if (tier === 4) return true;
    }
    return false;
  };
  function dictSearch(q, max = 9) {
    if (!dictRows || !q) return [];
    const key = normKana(q), asciiQ = /^[\x00-\x7f]+$/.test(q), low = q.toLowerCase(), out = [];
    const add = (test) => { for (const r of dictRows) { if (out.length >= max) break; if (!out.includes(r) && test(r)) out.push(r); } };
    add((r) => r.word === q || (key && r.key === key));
    if (key.charCodeAt(0) >= 0x80 || !asciiQ) add((r) => (r.word && r.word.startsWith(q)) || (key && r.key.startsWith(key)));
    if (asciiQ && q.length >= 2) for (const t of [2, 3, 4]) add((r) => glossTier(r.gloss, low, t));
    return out;
  }
  // Undoes common endings, so a word is found from the form you meet on a page
  const GODAN_I = { い: "う", き: "く", ぎ: "ぐ", し: "す", ち: "つ", に: "ぬ", び: "ぶ", み: "む", り: "る" };
  const GODAN_A = { わ: "う", か: "く", が: "ぐ", さ: "す", た: "つ", な: "ぬ", ば: "ぶ", ま: "む", ら: "る" };
  const POLITE = ["ませんでした", "ましょう", "ました", "ません", "ます", "たいです", "たくない", "たい"];
  const NEG = ["なかった", "ないです", "ない"];
  const TE = [["って", ["う", "つ", "る"]], ["った", ["う", "つ", "る"]], ["んで", ["む", "ぶ", "ぬ"]], ["んだ", ["む", "ぶ", "ぬ"]], ["いて", ["く"]], ["いた", ["く"]], ["いで", ["ぐ"]], ["いだ", ["ぐ"]], ["して", ["す"]], ["した", ["す"]]];
  function deinflect(s) {
    const out = new Set(), last = (t) => t[t.length - 1];
    // The irregular ones come first, so they aren't buried under words that merely sound alike
    for (const e of POLITE) if (s === "し" + e) out.add("する"); else if (s === "き" + e) out.add("くる");
    if (s === "して" || s === "した") out.add("する");
    if (s === "きて" || s === "きた") out.add("くる");
    for (const e of ["って", "った"]) if (s.endsWith(e) && /[行い]$/.test(s.slice(0, -e.length))) out.add(s.slice(0, -e.length) + "く");
    for (const e of POLITE) if (s.endsWith(e) && s.length > e.length) {
      const stem = s.slice(0, -e.length);
      out.add(stem + "る");
      if (GODAN_I[last(stem)]) out.add(stem.slice(0, -1) + GODAN_I[last(stem)]);
    }
    for (const e of NEG) if (s.endsWith(e) && s.length > e.length) {
      const stem = s.slice(0, -e.length);
      out.add(stem + "る");
      if (GODAN_A[last(stem)]) out.add(stem.slice(0, -1) + GODAN_A[last(stem)]);
    }
    for (const [e, rep] of TE) if (s.endsWith(e) && s.length > e.length) for (const r of rep) out.add(s.slice(0, -e.length) + r);
    for (const e of ["て", "た"]) if (s.endsWith(e) && s.length > e.length) out.add(s.slice(0, -e.length) + "る");
    for (const e of ["くない", "かった", "くて", "く"]) if (s.endsWith(e) && s.length > e.length) out.add(s.slice(0, -e.length) + "い");
    return [...out];
  }
  function dictLookup(text) {
    if (!dictIndex) return [];
    const hits = [], seen = new Set();
    for (let len = Math.min(text.length, 12); len > 0 && hits.length < 6; len--) {
      const s = text.slice(0, len);
      for (const form of [s, ...deinflect(s)]) for (const r of dictIndex.get(form) || dictIndex.get(hiraOf(form)) || []) {
        const id = r.word + "|" + r.reading;
        if (!seen.has(id) && hits.length < 6) { seen.add(id); hits.push({ card: { id: "d." + id, f: r.word || r.reading, r: r.reading, ro: "", en: r.gloss }, form: s }); }
      }
    }
    return hits;
  }
  const hiraOf = (s) => [...s].map((c) => { const n = c.codePointAt(0); return n >= 0x30a1 && n <= 0x30f6 ? String.fromCodePoint(n - 0x60) : c; }).join("");

  // Dictionary lookup for the extension: longest word first, with polite-verb endings undone
  const ENDINGS = ["ませんでした", "ましょう", "ました", "ません", "たいです", "たい", "ます", "ない", "て", "た"];
  function lookup(text) {
    const hits = [], seen = new Set();
    const add = (c, form) => { if (!seen.has(c.id)) { seen.add(c.id); hits.push({ card: c, form }); } };
    for (let len = Math.min(text.length, 12); len > 0; len--) {
      const s = text.slice(0, len), n = normKana(s);
      for (const c of C.cards) {
        if (!"wj".includes(c.t)) continue;
        if (c.f === s || normKana(c.r) === n) add(c, s);
      }
      for (const e of ENDINGS) if (s.endsWith(e) && s.length > e.length) {
        const stem = s.slice(0, -e.length), ns = normKana(stem);
        for (const c of C.cards) if (c.t === "w" && (c.f === stem + "ます" || normKana(c.r) === ns + "ます")) add(c, s);
      }
    }
    return hits.slice(0, 6);
  }

  root.YG = { normKana, answerOk, chatOk, srsNew, srsGrade, newProfile, saveText, loadText, today, stageOf, romajiMode, examGate, rankOf, dueCount, learnDay, completeDay, examResult, place, catchUp, makeQ, shuffle, quizWarmup, quizDay, quizExam, quizReview, quizGenki, placementSection, dictLoad, dictSearch, dictLookup, deinflect, myAdd, myImport, myExport, makeQMy, quizListen, shadowLines, lookup, byId, PASS };
})(globalThis);
