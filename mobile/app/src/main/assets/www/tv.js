// YomiGo on the TV: a home of big tiles, a cursor that moves the way a game console's does, a pad, and an on-screen keyboard.
// Rules followed: the most important thing first and top-left; the cursor goes to what's most in line, then nearest, then
// top-left; every move, choice and back has its own short sound; answers come in about a fifth of a second; no needless questions.
"use strict";
const TV = (() => {
  const { h, icon, btn, sfx, list, segmented, toggle } = UI;
  const get = (k, d) => { try { const v = localStorage.getItem(k); return v === null ? d : JSON.parse(v); } catch { return d; } };
  const put = (k, v) => { try { localStorage.setItem(k, JSON.stringify(v)); } catch {} };
  let prompts = null, osk = null, oskTarget = null;

  // ---- the cursor ----
  const FOCUSABLE = "button:not(:disabled), input:not(:disabled), textarea:not(:disabled), summary, [tabindex='0']";
  const visible = (e) => { const r = e.getBoundingClientRect(); return r.width > 0 && r.height > 0 && getComputedStyle(e).visibility !== "hidden" && !e.closest("nav, .prompts, [hidden]"); };
  function candidates() {
    const d = document.querySelector("dialog[open]");
    if (d) return [...d.querySelectorAll(FOCUSABLE)].filter(visible);
    return [...document.querySelectorAll(FOCUSABLE)].filter(visible);
  }
  function nudge(e, cls) { e.classList.remove("nudge", "edge-x", "edge-y"); void e.offsetWidth; e.classList.add(cls); setTimeout(() => e.classList.remove(cls), 300); }
  function focus(e, quiet) {
    if (!e) return;
    e.focus({ preventScroll: true });
    e.scrollIntoView({ block: "nearest", inline: "nearest", behavior: UI.calm.matches ? "auto" : "smooth" });
    if (!quiet) { sfx("move"); nudge(e, "nudge"); }
    if (e.closest(".tiles")) hubLabel(e);
  }
  // In line first, then nearest, then the one further up and to the left
  function move(dir) {
    const all = candidates(), cur = document.activeElement && all.includes(document.activeElement) ? document.activeElement : null;
    if (!cur) return focus(all.find((e) => e.classList.contains("primary")) || all[0]);
    const a = cur.getBoundingClientRect(), horiz = dir === "left" || dir === "right", sign = dir === "right" || dir === "down" ? 1 : -1;
    let best = null, bestScore = Infinity;
    for (const e of all) {
      if (e === cur) continue;
      const b = e.getBoundingClientRect();
      const gap = horiz ? (sign > 0 ? b.left - a.right : a.left - b.right) : (sign > 0 ? b.top - a.bottom : a.top - b.bottom);
      const centre = horiz ? (sign > 0 ? b.left + b.width / 2 - (a.left + a.width / 2) : a.left + a.width / 2 - (b.left + b.width / 2)) : (sign > 0 ? b.top + b.height / 2 - (a.top + a.height / 2) : a.top + a.height / 2 - (b.top + b.height / 2));
      if (gap < -4 && centre <= 4) continue; // not that way
      const overlap = horiz ? Math.min(a.bottom, b.bottom) - Math.max(a.top, b.top) : Math.min(a.right, b.right) - Math.max(a.left, b.left);
      const off = horiz ? Math.abs(b.top + b.height / 2 - (a.top + a.height / 2)) : Math.abs(b.left + b.width / 2 - (a.left + a.width / 2));
      const score = (overlap > 0 ? 0 : 1e6) + Math.max(gap, 0) + off * 2 + (b.top + b.left) * 1e-4;
      if (score < bestScore) { bestScore = score; best = e; }
    }
    if (best) focus(best); else { sfx("edge"); nudge(cur, horiz ? "edge-x" : "edge-y"); }
  }
  function ok() {
    const e = document.activeElement;
    if (!e || e === document.body || e === document.getElementById("app")) return move("down");
    if (e.tagName === "INPUT" || e.tagName === "TEXTAREA") return openOsk(e);
    sfx("select"); e.click();
  }
  function backPress() {
    if (osk) return closeOsk(true);
    if (goBack()) return;
    sfx("edge");
  }
  // L and R step through the main places, the way shoulder buttons flip pages on a console
  function shoulder(d) {
    if (!P || app.classList.contains("flow")) return;
    const order = [home, cards, exams, quest, library, textReader, talk, settings], cur = TV.place || 0, n = (cur + d + order.length) % order.length;
    TV.place = n; sfx("nav"); order[n]();
  }

  // ---- on-screen keyboard: romaji in, kana out, as the answer boxes already turn romaji into kana ----
  const ROWS = ["1234567890", "qwertyuiop", "asdfghjkl-", "zxcvbnm'.?"];
  function openOsk(input) {
    closeOsk(false);
    oskTarget = input;
    osk = h("div", { cls: "osk", role: "group", "aria-label": "Keyboard" });
    const type = (ch) => { input.value += ch; input.dispatchEvent(new Event("input")); };
    for (const r of ROWS) for (const ch of r) osk.append(h("button", { cls: "key", type: "button", "data-quiet": "1", on: () => type(ch) }, ch));
    osk.append(h("button", { cls: "key wide", type: "button", "data-quiet": "1", on: () => type(" ") }, "Space"),
      h("button", { cls: "key wide", type: "button", "data-quiet": "1", on: () => { input.value = [...input.value].slice(0, -1).join(""); input.dispatchEvent(new Event("input")); } }, "⌫ Delete"),
      h("button", { cls: "key wide go", type: "button", "data-quiet": "1", on: () => { const t = oskTarget; closeOsk(false); t.dispatchEvent(new KeyboardEvent("keydown", { key: "Enter", bubbles: true })); } }, "Done"),
      h("button", { cls: "key", type: "button", "data-quiet": "1", on: () => closeOsk(true) }, "✕"));
    document.body.append(osk); document.documentElement.classList.add("osk-open");
    sfx("sheet"); focus(osk.children[10], true);
    setPrompts([["A", "Type"], ["B", "Close"], ["Start", "Done"]]);
  }
  function closeOsk(refocus) {
    if (!osk) return;
    osk.remove(); osk = null; document.documentElement.classList.remove("osk-open");
    if (refocus && oskTarget && oskTarget.isConnected) { sfx("back"); focus(oskTarget, true); }
    oskTarget = null; defaultPrompts();
  }

  // ---- the button prompts along the bottom ----
  const labels = () => get("yg.padLabels", 0) === 1 ? { A: "✕", B: "○" } : { A: "A", B: "B" };
  function setPrompts(items) { prompts.replaceChildren(...items.map(([k, t]) => h("span", null, h("b", null, labels()[k] || (k === "Start" ? "≡" : k)), t))); }
  const defaultPrompts = () => setPrompts(app.classList.contains("flow") || !P ? [["A", "OK"], ["B", "Back"]] : [["A", "OK"], ["B", "Back"], ["L R", "Places"]]);

  // ---- keys and pad ----
  const PAD = { up: { b: 12 }, down: { b: 13 }, left: { b: 14 }, right: { b: 15 }, ok: { b: 0 }, back: { b: 1 }, l: { b: 4 }, r: { b: 5 }, start: { b: 9 } };
  const ACTIONS = { up: () => move("up"), down: () => move("down"), left: () => move("left"), right: () => move("right"), ok, back: backPress, l: () => shoulder(-1), r: () => shoulder(1),
    start: () => { if (osk) osk.querySelector(".go").click(); } };
  let binding = null; // while rebinding: the action waiting for a press
  function onKey(e) {
    if (binding) return;
    const map = { ArrowUp: "up", ArrowDown: "down", ArrowLeft: "left", ArrowRight: "right", Escape: "back", Backspace: "back", PageUp: "l", PageDown: "r" };
    const t = e.target, typing = (t.tagName === "INPUT" || t.tagName === "TEXTAREA");
    if (typing && (e.key === "ArrowLeft" || e.key === "ArrowRight" || (e.key === "Backspace" && t.value))) return; // a real keyboard edits as usual
    if (osk && e.key.length === 1 && !e.ctrlKey) { oskTarget.value += e.key; oskTarget.dispatchEvent(new Event("input")); return e.preventDefault(); }
    if (e.key === "Enter" && !typing && !(t.tagName === "BUTTON" || t.tagName === "SUMMARY" || t.getAttribute("tabindex") === "0")) { e.preventDefault(); return ok(); }
    if (e.key === "Enter" && (t.tagName === "BUTTON" || t.tagName === "SUMMARY")) sfx("select");
    const a = map[e.key];
    if (a) { e.preventDefault(); ACTIONS[a](); }
  }
  const pressed = (pad, m, dz) => m.b !== undefined ? !!(pad.buttons[m.b] && pad.buttons[m.b].pressed) : (pad.axes[m.a] || 0) * m.s > dz;
  let held = {}, repeatAt = {};
  function poll(t) {
    requestAnimationFrame(poll);
    const pad = [...(navigator.getGamepads ? navigator.getGamepads() : [])].find(Boolean);
    if (!pad) return;
    // The USB N64 pad 0E6D:111D is set up as the N64 launcher does: A, B, L and R, Start, with C-Left and C-Right as L and R too
    const n64 = /0e6d.*111d/i.test(pad.id) ? { ok: { b: 2 }, back: { b: 1 }, l: { b: 4 }, r: { b: 5 }, start: { b: 12 } } : {};
    const map = { ...PAD, ...n64, ...get("yg.pad", {}) }, dz = get("yg.deadzone", 30) / 100 + 0.2;
    // Pads without the standard layout report their d-pad as a hat on axis 9: eight directions from -1 to 1, more than 1 when let go
    const hv = pad.mapping !== "standard" && pad.axes.length > 9 ? pad.axes[9] : 9, hk = hv <= 1.05 ? Math.round((hv + 1) * 3.5) % 8 : -1;
    const hat = { up: [7, 0, 1].includes(hk), right: [1, 2, 3].includes(hk), down: [3, 4, 5].includes(hk), left: [5, 6, 7].includes(hk) };
    const alt = n64.ok ? { l: { b: 0 }, r: { b: 8 } } : {};
    if (binding) {
      for (let b = 0; b < pad.buttons.length; b++) if (pad.buttons[b].pressed && !held["b" + b]) { binding({ b }); held["b" + b] = true; return; }
      for (let a = 0; a < pad.axes.length; a++) if (Math.abs(pad.axes[a]) > 0.6 && !held["a" + a]) { binding({ a, s: Math.sign(pad.axes[a]) }); held["a" + a] = true; return; }
      for (let b = 0; b < pad.buttons.length; b++) if (!pad.buttons[b].pressed) held["b" + b] = false;
      for (let a = 0; a < pad.axes.length; a++) if (Math.abs(pad.axes[a]) < 0.3) held["a" + a] = false;
      return;
    }
    for (const k of Object.keys(ACTIONS)) {
      // the left stick moves too, as well as the d-pad
      const stick = { up: { a: 1, s: -1 }, down: { a: 1, s: 1 }, left: { a: 0, s: -1 }, right: { a: 0, s: 1 } }[k];
      const on = pressed(pad, map[k], dz) || (stick && !get("yg.pad", {})[k] && (pressed(pad, stick, dz) || hat[k])) || (alt[k] && pressed(pad, alt[k], dz));
      if (on && !held[k]) { held[k] = true; repeatAt[k] = t + 380; ACTIONS[k](); }
      else if (on && "updownleftright".includes(k) && t > repeatAt[k]) { repeatAt[k] = t + 110; ACTIONS[k](); }
      else if (!on) held[k] = false;
    }
  }

  // ---- the home: a row of tiles, today's class first and largest ----
  function clock() { const d = new Date(); return d.toLocaleTimeString([], { hour: "numeric", minute: "2-digit" }); }
  function hubLabel(e) { const l = document.querySelector(".hub-label"); if (l) l.textContent = e.dataset.label || ""; }
  function tile(label, sub, ic, tint, on, extra, wide) {
    return h("button", { cls: "tile" + (wide ? " wide" : ""), type: "button", "data-label": label, style: `--tile:${tint}`, "data-quiet": "1", on: () => { TV.place = null; on(); } },
      extra || icon(ic), h("div", null, h("div", { cls: "t", lang: /[぀-ヿ一-鿿]/.test(label) ? "ja" : null }, label), sub ? h("div", { cls: "s" }, sub) : null));
  }
  function hub() {
    setNav("home"); TV.place = 0;
    const t = Y.today(), gate = Y.examGate(P), due = Y.dueCount(P, t);
    if (P.last !== t) P.today = 0;
    const d = C.days[P.day], over = d && P.today >= P.pace && P.last === t;
    const first = gate >= 0 ? tile(C.exams[gate].name, "Required exam · pass with 80%", "exam", "linear-gradient(150deg,#ff375f,#a2133a)", () => startExam(gate), null, true)
      : !d ? tile("Course complete!", "Keep reviewing and immersing", "star", "linear-gradient(150deg,#30d158,#0a7a33)", cards, null, true)
      : tile(d.ja, over ? "Today's class is done" : `${P.step ? "Continue" : "Start"} · Class ${P.day + 1} of ${C.days.length}`, "home", "linear-gradient(150deg,#ff5c8d,#b0234f)", over ? cards : runStep, UI.ring(P.step, 10, `${P.step * 10}%`), true);
    const clk = h("span", { cls: "clock" }, clock()), timer = setInterval(() => { clk.textContent = clock(); }, 10000);
    onLeave(() => clearInterval(timer));
    show(h("div", { cls: "hub-top" }, h("span", { cls: "avatar" }, P.name[0].toUpperCase()), h("div", null, h("div", { cls: "name" }, P.name), h("div", { cls: "chips", style: "margin:4px 0 0" }, h("span", { cls: "chip fire" }, icon("flame"), P.streak), h("span", { cls: "chip star" }, icon("star"), `${P.xp} XP`), h("span", { cls: "chip rank", lang: "ja" }, rankName()), goalBox(true))), clk),
      h("div", { cls: "tiles" }, first,
        tile(L("Flashcards", "カード", 3), due ? `${due} due` : "All caught up", "cards", "linear-gradient(150deg,#ff9f0a,#c25e00)", cards, h("div", { cls: "num" }, due)),
        tile(L("Exams", "しけん", 2), "Gates on the road", "exam", "linear-gradient(150deg,#5e5ce6,#2f2c9e)", exams),
        tile(L("Quest", "クエスト", 3), rankName(), "quest", "linear-gradient(150deg,#30b0c7,#136b7c)", quest),
        tile(L("Library", "本だな", 5), "Dictionary, guide, media", "library", "linear-gradient(150deg,#34c759,#1a7a33)", library),
        tile(L("Text Reader", "テキスト", 3), "Look up copied text", "reader", "linear-gradient(150deg,#64d2ff,#1f7ea8)", textReader),
        tile(L("Talk", "かいわ", 3), "Chat with Sakura", "talk", "linear-gradient(150deg,#bf5af2,#6e2a99)", talk)),
      h("div", { cls: "hub-label", "aria-hidden": "true" }),
      h("div", { cls: "rounds" }, [["settings", "Settings", settings], ["sync", "Sync", syncScreen], ["person", "Switch learner", profiles]].map(([ic, label, go]) =>
        h("button", { cls: "round", type: "button", "aria-label": label, title: label, "data-label": label, "data-quiet": "1", on: go }, icon(ic)))));
  }

  // ---- TV settings: pad, labels, deadzone, screen type ----
  function rebind() {
    const steps = [["ok", "OK"], ["back", "Back"], ["up", "Up"], ["down", "Down"], ["left", "Left"], ["right", "Right"], ["l", "L (previous place)"], ["r", "R (next place)"], ["start", "Start"]];
    const map = {}, title = h("div", { cls: "big acc", style: "text-align:center" }), note = h("p", { cls: "dim", style: "text-align:center" });
    let i = 0;
    setNav(""); back = () => { binding = null; settings(); };
    const ask = () => {
      if (i >= steps.length) { binding = null; put("yg.pad", map); sfx("done"); UI.toast("Controller set up.", "good"); return settings(); }
      title.textContent = steps[i][1]; sfx("move");
      binding = (m) => { map[steps[i][0]] = m; i++; ask(); };
    };
    show(h("h1", null, "Set up your controller"), h("div", { cls: "card prompt" }, h("p", null, "Press the button for:"), title, note),
      btn("Reset to standard", () => { binding = null; put("yg.pad", {}); UI.toast("Back to the standard layout.", "good"); settings(); }));
    note.textContent = [...(navigator.getGamepads ? navigator.getGamepads() : [])].some(Boolean) ? "" : "No controller found. Press a button on it to wake it, or press Esc to go back.";
    ask();
  }
  function settingsRows() {
    return list("TV and controller", [
      { label: "Set up controller", icon: "pad", tint: "#8e8e93", on: rebind },
      { label: "Button labels", control: segmented(["A · B", "✕ · ○"], get("yg.padLabels", 0), "Button labels", (i) => { put("yg.padLabels", i); defaultPrompts(); }) },
      { label: "Stick deadzone", control: segmented(["10%", "30%", "50%"], [10, 30, 50].indexOf(get("yg.deadzone", 30)), "Stick deadzone", (i) => put("yg.deadzone", [10, 30, 50][i])) },
      { label: "CRT or RF screen", sub: "Keeps everything well inside the picture", control: toggle(get("yg.crt", false), "CRT screen", (v) => { put("yg.crt", v); document.documentElement.classList.toggle("crt", v); }) }]);
  }

  function start() {
    document.documentElement.classList.add("tv");
    document.documentElement.classList.toggle("crt", get("yg.crt", false));
    prompts = h("div", { cls: "prompts", "aria-hidden": "true" }); document.body.append(prompts);
    document.addEventListener("keydown", onKey, true);
    // After each new screen: put the cursor on the main action, or the first thing, and show the right prompts
    document.addEventListener("yg:show", () => {
      closeOsk(false); defaultPrompts();
      requestAnimationFrame(() => { const all = candidates(); focus(app.querySelector(".tile, .btn.primary:not(:disabled)") && all.includes(app.querySelector(".tile, .btn.primary:not(:disabled)")) ? app.querySelector(".tile, .btn.primary:not(:disabled)") : all.find((e) => e.tagName !== "INPUT") || all[0], true); });
    });
    requestAnimationFrame(poll);
  }
  return { start, hub, settingsRows, place: 0 };
})();
