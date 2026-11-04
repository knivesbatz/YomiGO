// YomiGo UI kit: elements, icons, components, sounds and reward effects. Screens live in app.js; nothing here knows about lessons.
// All text goes in with textContent, so nothing from a save file can become markup.
"use strict";
const UI = (() => {
  const calm = matchMedia("(prefers-reduced-motion: reduce)");

  function h(tag, props, ...kids) {
    const e = document.createElement(tag);
    for (const k in props || {}) {
      const v = props[k];
      if (v === undefined || v === null || v === false) continue;
      if (k === "on") e.addEventListener("click", v);
      else if (k === "cls") e.className = v;
      else e.setAttribute(k, v === true ? "" : v);
    }
    for (const c of kids.flat(Infinity)) if (c !== null && c !== undefined && c !== false && c !== "") e.append(c.nodeType ? c : String(c));
    return e;
  }

  // Icons: one stroke style, drawn for YomiGo
  const PATHS = {
    home: "M3.5 10.5 12 3.5l8.5 7V20a1 1 0 0 1-1 1h-4.5v-6h-6v6H4.5a1 1 0 0 1-1-1z",
    cards: "M8 3.5h10a1.5 1.5 0 0 1 1.5 1.5v12A1.5 1.5 0 0 1 18 18.5H8A1.5 1.5 0 0 1 6.5 17V5A1.5 1.5 0 0 1 8 3.5zM3.5 7.5v11a2 2 0 0 0 2 2h10",
    exam: "M2.5 5.5c6 1.6 13 1.6 19 0M4.5 9.5h15M7 6.5V21M17 6.5V21M12 7.2v2.3",
    quest: "M5.5 21V3.5M5.5 4h12l-2.5 4.5L17.5 13h-12",
    library: "M4.5 5.5a2 2 0 0 1 2-2h13v15h-13a2 2 0 0 0-2 2zM4.5 20.5v-15M8.5 7.5h7",
    reader: "M4 6h16M4 11h16M4 16h9M17.5 19.5l3 3M15.5 14a2.5 2.5 0 1 0 0 5 2.5 2.5 0 0 0 0-5z",
    talk: "M4 5.5A1.5 1.5 0 0 1 5.5 4h13A1.5 1.5 0 0 1 20 5.5v9a1.5 1.5 0 0 1-1.5 1.5H10l-4.5 4v-4h0A1.5 1.5 0 0 1 4 14.5zM8.5 10h.01M12 10h.01M15.5 10h.01",
    settings: "M4 7h9M17 7h3M4 17h3M11 17h9M15 5v4M9 15v4",
    chev: "M1.5 1.5 6.5 6.5 1.5 11.5",
    close: "M6 6l12 12M18 6 6 18",
    back: "M15 4.5 7.5 12l7.5 7.5",
    speaker: "M4 9.5h3.5L12 5.5v13l-4.5-4H4zM15.5 9a4 4 0 0 1 0 6M18 6.5a7.5 7.5 0 0 1 0 11",
    mic: "M12 3.5a2.5 2.5 0 0 1 2.5 2.5v6a2.5 2.5 0 0 1-5 0V6A2.5 2.5 0 0 1 12 3.5zM6.5 11.5a5.5 5.5 0 0 0 11 0M12 17v3.5",
    flame: "M12 21c-3.9 0-6.5-2.6-6.5-6 0-3.2 2.4-5 3.5-8 1.6 1.4 2.4 3 2.5 4.5 1.3-1 2-2.6 2-4.5 2.6 2 5 4.8 5 8 0 3.4-2.6 6-6.5 6z",
    star: "M12 3.5l2.6 5.4 5.9.8-4.3 4.1 1 5.8L12 16.8l-5.2 2.8 1-5.8-4.3-4.1 5.9-.8z",
    check: "M5 12.5l4.5 4.5L19 7.5",
    alert: "M12 8v5M12 16.5h.01M12 3.5l9 16H3z",
    sync: "M20 11.5A8 8 0 0 0 6 6.5L4 8.5M4 4v4.5h4.5M4 12.5a8 8 0 0 0 14 5l2-2M20 20v-4.5h-4.5",
    person: "M12 12a4 4 0 1 0 0-8 4 4 0 0 0 0 8zM4.5 20.5a7.5 7.5 0 0 1 15 0",
    send: "M4 12l16-8-6 16-2.5-6.5z",
    plus: "M12 5v14M5 12h14",
    leaf: "M5 19C5 10 10 5 19 5c0 9-5 14-14 14zM5 19l7-7",
    folder: "M3.5 6.5a2 2 0 0 1 2-2h4l2 2.5h7a2 2 0 0 1 2 2v8.5a2 2 0 0 1-2 2h-13a2 2 0 0 1-2-2z",
    book: "M12 6.5c-2-1.5-5-2-8.5-2v14c3.5 0 6.5.5 8.5 2 2-1.5 5-2 8.5-2v-14c-3.5 0-6.5.5-8.5 2zM12 6.5v14",
    pad: "M7.5 8h9a4.5 4.5 0 0 1 4.3 5.8l-1.1 3.7a2 2 0 0 1-3.4.8L14 16h-4l-2.3 2.3a2 2 0 0 1-3.4-.8l-1.1-3.7A4.5 4.5 0 0 1 7.5 8zM8 11v3M6.5 12.5h3M15.5 12h.01M17.5 13.5h.01",
  };
  function icon(name, cls) {
    const s = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    s.setAttribute("viewBox", name === "chev" ? "0 0 8 13" : "0 0 24 24");
    s.setAttribute("class", cls || "ico"); s.setAttribute("aria-hidden", "true");
    s.setAttribute("fill", name === "flame" || name === "star" ? "currentColor" : "none");
    s.setAttribute("stroke", "currentColor"); s.setAttribute("stroke-width", name === "chev" ? "2" : "1.8"); s.setAttribute("stroke-linecap", "round"); s.setAttribute("stroke-linejoin", "round");
    const p = document.createElementNS("http://www.w3.org/2000/svg", "path"); p.setAttribute("d", PATHS[name]); s.append(p);
    return s;
  }

  // ---- sound: short, crisp, tied to what just happened. Made here with Web Audio, so there are no sound files. ----
  let actx = null, combo = 0;
  const muted = () => { try { return localStorage.getItem("yg.mute") === "1"; } catch { return false; } };
  function tone(at, f, vol, len, type = "sine", to) {
    const t = actx.currentTime + at, o = actx.createOscillator(), g = actx.createGain();
    o.type = type; o.frequency.setValueAtTime(f, t); if (to) o.frequency.exponentialRampToValueAtTime(to, t + len);
    g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(vol, t + 0.004); g.gain.exponentialRampToValueAtTime(0.0001, t + len);
    o.connect(g).connect(actx.destination); o.start(t); o.stop(t + len + 0.02);
  }
  const note = (at, f, vol = 0.2, len = 0.45) => { tone(at, f, vol, len); tone(at, f * 2, vol / 3, len / 3); tone(at, f * 2.75, vol / 5, len / 5); };
  const S = [523, 587, 659, 784, 880, 1047, 1175, 1319, 1568, 1760];
  const SFX = {
    tap: () => tone(0, 1900, 0.035, 0.03, "triangle"),
    move: () => tone(0, 1250, 0.05, 0.035, "triangle"),
    select: () => { tone(0, 880, 0.08, 0.06, "triangle"); tone(0.04, 1320, 0.07, 0.08, "triangle"); },
    back: () => { tone(0, 990, 0.07, 0.06, "triangle"); tone(0.045, 660, 0.07, 0.08, "triangle"); },
    edge: () => tone(0, 180, 0.12, 0.07, "sine", 120),
    nav: () => { tone(0, 740, 0.06, 0.07); tone(0.035, 1110, 0.05, 0.09); },
    on: () => { tone(0, 700, 0.07, 0.05, "triangle"); tone(0.04, 1050, 0.07, 0.07, "triangle"); },
    off: () => { tone(0, 1050, 0.06, 0.05, "triangle"); tone(0.04, 700, 0.06, 0.07, "triangle"); },
    sheet: () => tone(0, 420, 0.1, 0.12, "sine", 840),
    error: () => { tone(0, 300, 0.12, 0.09, "square"); tone(0.1, 240, 0.1, 0.12, "square"); },
    ok: () => { note(0, S[combo]); note(0.07, S[combo + 2]); combo = Math.min(combo + 1, 7); },
    bad: () => { note(0, 330, 0.2, 0.3); note(0.11, 262, 0.2, 0.35); },
    done: () => [784, 988, 1175, 1568].forEach((f, i) => note(i * 0.08, f, 0.2, i === 3 ? 0.9 : 0.4)),
    ace: () => { [523, 659, 784, 1047].forEach((f, i) => note(i * 0.07, f)); [1047, 1319, 1568].forEach((f, i) => { note(0.3, f, 0.14, 1.2); note(0.3 + i * 0.1, f * 2, 0.06, 0.3); }); },
    coin: () => { tone(0, 1568, 0.07, 0.06, "square"); tone(0.06, 2093, 0.07, 0.18, "square"); },
  };
  function sfx(kind) {
    if (kind !== "ok") { if (kind !== "tap" && kind !== "move" && kind !== "coin") combo = 0; }
    if (muted()) return;
    try { actx = actx || new AudioContext(); if (actx.state === "suspended") actx.resume(); SFX[kind](); } catch {}
  }
  const buzz = (p) => { try { if (navigator.vibrate) navigator.vibrate(p); } catch {} };
  // Every press clicks, unless the press makes its own sound
  document.addEventListener("pointerdown", (e) => { const b = e.target.closest("button, summary, [role=button]"); if (b && !b.disabled && !b.dataset.quiet) sfx("tap"); }, true);

  // ---- components ----
  const btn = (label, on, cls, attrs) => h("button", { on, cls: "btn " + (cls || ""), type: "button", ...attrs }, label);
  const iconBtn = (name, label, on, cls) => h("button", { on, cls: "btn icon " + (cls || ""), type: "button", "aria-label": label, title: label }, icon(name));
  const progress = (v, max) => { const b = h("div", { cls: "progress", role: "progressbar", "aria-valuenow": v, "aria-valuemin": 0, "aria-valuemax": max }, h("i")); b.firstChild.style.width = (max ? Math.min(100, v * 100 / max) : 0) + "%"; return b; };
  function ring(v, max, label) {
    const NS = "http://www.w3.org/2000/svg", s = document.createElementNS(NS, "svg"), r = 34, len = 2 * Math.PI * r;
    s.setAttribute("viewBox", "0 0 76 76"); s.setAttribute("class", "ring");
    for (const c of ["track", "fill"]) { const e = document.createElementNS(NS, "circle"); e.setAttribute("cx", 38); e.setAttribute("cy", 38); e.setAttribute("r", r); e.setAttribute("class", c); s.append(e); }
    const f = s.lastChild; f.style.strokeDasharray = len; f.style.strokeDashoffset = len;
    requestAnimationFrame(() => requestAnimationFrame(() => { f.style.strokeDashoffset = len * (1 - (max ? Math.min(1, v / max) : 0)); }));
    return h("div", { cls: "ring-wrap", role: "img", "aria-label": label || `${v} of ${max}` }, s, h("span", { "aria-hidden": "true" }, label || `${v}/${max}`));
  }
  // A grouped list. Rows: { label, value, on, icon, sub, control, lang }
  function list(header, rows, footer) {
    const box = h("div", { cls: "list" }, rows.filter(Boolean).map((r) => {
      const inner = [r.icon ? h("span", { cls: "ic", style: r.tint ? `background:${r.tint}` : null }, icon(r.icon)) : null,
        h("span", { cls: "lbl" }, h("span", { lang: r.lang }, r.label), r.sub ? h("div", { cls: "dim small" }, r.sub) : null),
        r.value !== undefined ? h("span", { cls: "val" }, r.value) : null, r.control || null, r.on && !r.control ? icon("chev", "chev") : null];
      const el = r.on && !r.control ? h("button", { cls: "item", type: "button", on: r.on }, inner) : h("div", { cls: "item" }, inner);
      if (r.menu) el.addEventListener("contextmenu", (e) => menu(e, r.menu));
      return el;
    }));
    return [header ? h("div", { cls: "list-h" }, header) : null, box, footer ? h("div", { cls: "list-f" }, footer) : null].filter(Boolean);
  }
  function toggle(on, label, change) {
    const s = h("button", { cls: "switch", type: "button", role: "switch", "aria-checked": String(!!on), "aria-label": label, "data-quiet": "1" });
    s.addEventListener("click", () => { const v = s.getAttribute("aria-checked") !== "true"; s.setAttribute("aria-checked", String(v)); sfx(v ? "on" : "off"); change(v); });
    return s;
  }
  function segmented(options, cur, label, change) {
    const thumb = h("span", { cls: "thumb", "aria-hidden": "true" });
    const box = h("div", { cls: "seg", role: "radiogroup", "aria-label": label }, thumb);
    const place = () => { const b = box.querySelector('[aria-checked="true"]'); if (b) { thumb.style.left = b.offsetLeft + "px"; thumb.style.width = b.offsetWidth + "px"; } };
    options.forEach((o, i) => box.append(h("button", { type: "button", role: "radio", "aria-checked": String(i === cur), "data-quiet": "1", on: () => {
      if (box.querySelector('[aria-checked="true"]') === box.children[i + 1]) return;
      box.querySelectorAll("[role=radio]").forEach((x, k) => x.setAttribute("aria-checked", String(k === i))); place(); sfx("on"); change(i);
    } }, o)));
    requestAnimationFrame(place); new ResizeObserver(place).observe(box);
    return box;
  }
  const empty = (ic, title, text, ...more) => h("div", { cls: "empty" }, icon(ic), h("h3", null, title), text ? h("p", null, text) : null, more);
  const field = (attrs) => h("input", { autocomplete: "off", autocapitalize: "off", spellcheck: "false", ...attrs });

  // Toasts replace alert(): they never block, and screen readers hear them
  function toast(msg, kind) {
    const mac = document.documentElement.classList.contains("mac");
    const t = mac ? h("div", { cls: "toast " + (kind || "") }, h("img", { cls: "app", src: "icon.png", alt: "" }), h("b", null, "YomiGo", h("small", null, "now")), h("span", null, msg))
      : h("div", { cls: "toast " + (kind || "") }, kind ? icon(kind === "good" ? "check" : "alert") : null, msg);
    document.getElementById("toasts").append(t);
    if (kind === "bad") sfx("error");
    setTimeout(() => { t.classList.add("out"); setTimeout(() => t.remove(), 260); }, Math.min(6000, 2200 + msg.length * 40));
  }
  // A sheet replaces confirm(). Resolves with the index of the action picked, or -1. Esc and the backdrop cancel.
  function sheet(title, body, actions) {
    return new Promise((done) => {
      const d = h("dialog", { cls: "sheet", "aria-label": title }, h("h3", null, title), body ? h("p", { cls: "dim" }, body) : null);
      const close = (i) => { if (d.close) d.close(); d.remove(); done(i); };
      const acts = h("div", { cls: "acts" + (actions.length > 1 ? " stack" : "") });
      actions.forEach((a, i) => acts.append(btn(a.label, () => close(i), a.cls === "danger" ? "danger" : "primary")));
      acts.append(actions.length ? btn("Cancel", () => close(-1)) : btn("OK", () => close(-1), "primary"));
      d.append(acts);
      d.addEventListener("cancel", (e) => { e.preventDefault(); close(-1); });
      d.addEventListener("click", (e) => { if (e.target === d) close(-1); });
      // Big Sur's own WebKit has no <dialog>, so there it's shown as a plain layer
      document.body.append(d); if (d.showModal) d.showModal(); else { d.setAttribute("open", ""); d.classList.add("fallback"); } sfx("sheet");
      (d.querySelector(".btn.primary, .btn") || d).focus();
    });
  }

  // ---- reward effects: petals from a right answer, more the longer the run; a cheer from three in a row; petal rain for a perfect score ----
  let streak = 0, comboEl = null;
  function fly(x, y, path, char, color, ms) {
    const s = h("span", { cls: "petal", "aria-hidden": "true" }, char);
    s.style.cssText = `left:${x}px;top:${y}px;color:${color};font-size:${14 + Math.random() * 12}px`;
    document.body.append(s);
    s.animate(path, { duration: ms, easing: "cubic-bezier(.2,.8,.2,1)" }).onfinish = () => s.remove();
  }
  function floatText(x, y, text) {
    const s = h("div", { cls: "xp-float", "aria-hidden": "true" }, text);
    s.style.left = x + "px"; s.style.top = y + "px"; document.body.append(s);
    s.animate([{ transform: "translate(-50%,0) scale(.6)", opacity: 0 }, { transform: "translate(-50%,-18px) scale(1.15)", opacity: 1, offset: 0.25 }, { transform: "translate(-50%,-56px) scale(1)", opacity: 0 }],
      { duration: calm.matches ? 600 : 1000, easing: "ease-out" }).onfinish = () => s.remove();
  }
  function showCombo() {
    if (streak < 2) { if (comboEl) { comboEl.remove(); comboEl = null; } return; }
    if (!comboEl) { comboEl = h("div", { cls: "combo", "aria-hidden": "true" }); document.body.append(comboEl); }
    comboEl.textContent = `${streak} combo`; comboEl.classList.remove("bump"); void comboEl.offsetWidth; comboEl.classList.add("bump");
  }
  function cheerRight(anchor, xp) {
    streak++; sfx("ok"); buzz(12);
    requestAnimationFrame(() => {
      const r = (anchor && anchor.isConnected ? anchor : document.body).getBoundingClientRect(), cx = r.left + Math.min(r.width / 2, 200), cy = r.top + Math.min(r.height / 2, 40), n = 10 + 2 * Math.min(streak, 10);
      if (xp) floatText(cx, cy - 10, `+${xp} XP`);
      if (!calm.matches) for (let i = 0; i < n; i++) {
        const dx = (Math.random() - 0.5) * 320, up = 80 + Math.random() * 160, spin = Math.random() * 540;
        fly(cx, cy, [{ transform: "translate(0,0) scale(.4)", opacity: 1 }, { transform: `translate(${dx * 0.6}px,${-up}px) rotate(${spin / 2}deg) scale(1)`, opacity: 1, offset: 0.45 },
          { transform: `translate(${dx}px,${120 - up / 2}px) rotate(${spin}deg)`, opacity: 0 }], i % 3 ? "✦" : "✿", ["#f2b6c4", "#d6336c", "#ffc24a"][i % 3], 1000);
      }
      showCombo();
      if (streak >= 3 && (streak < 6 || streak % 5 === 0)) {
        const p = h("div", { cls: "cheer", "aria-hidden": "true" }, streak >= 20 ? "てんさい！" : streak >= 10 ? "さいこう！" : streak >= 5 ? "すごい！" : "いいね！", h("small", null, `${streak} in a row`));
        document.body.append(p);
        p.animate([{ transform: "translate(-50%,-40px) scale(.6)", opacity: 0 }, { transform: "translate(-50%,0) scale(1.1)", opacity: 1, offset: 0.2 },
          { transform: "translate(-50%,0) scale(1)", opacity: 1, offset: 0.8 }, { transform: "translate(-50%,-10px) scale(1)", opacity: 0 }], { duration: 1300 }).onfinish = () => p.remove();
      }
    });
  }
  function cheerMiss() { streak = 0; showCombo(); sfx("bad"); buzz([18, 50, 18]); }
  function endRun() { streak = 0; showCombo(); }
  function petalRain() {
    buzz([20, 60, 20, 60, 40]);
    if (!calm.matches) for (let i = 0; i < 44; i++) setTimeout(() => fly(Math.random() * innerWidth, -30, [{ transform: "translate(0,0)" }, { transform: `translate(${(Math.random() - 0.5) * 140}px,${innerHeight + 60}px) rotate(${Math.random() * 720}deg)` }], "✿", "#f2b6c4", 2400 + Math.random() * 800), i * 45);
  }
  // Numbers that count up, for XP on a result screen
  function countUp(el, to, ms = 900) {
    if (calm.matches || to <= 0) { el.textContent = to; return; }
    const t0 = performance.now(); let last = -1;
    const step = (t) => { const k = Math.min(1, (t - t0) / ms), v = Math.round(to * (1 - Math.pow(1 - k, 3))); if (v !== last) { el.textContent = v; if (v % 4 === 0 && v) sfx("coin"); last = v; } if (k < 1) requestAnimationFrame(step); };
    requestAnimationFrame(step);
  }

  // A context menu, as right-click shows on macOS. items: [label, action, shortcut?] or "-" for a separator.
  function menu(e, items) {
    e.preventDefault();
    document.querySelectorAll(".menu").forEach((m) => m.remove());
    const m = h("div", { cls: "menu", role: "menu" }, items.map((it) => it === "-" ? h("hr") : h("button", { type: "button", role: "menuitem", "data-quiet": "1", on: () => { m.remove(); it[1](); } }, it[0], it[2] ? h("kbd", null, it[2]) : null)));
    document.body.append(m);
    const r = m.getBoundingClientRect();
    m.style.left = Math.min(e.clientX, innerWidth - r.width - 6) + "px"; m.style.top = Math.min(e.clientY, innerHeight - r.height - 6) + "px";
    const off = (ev) => { if (!m.contains(ev.target) || ev.key === "Escape") { m.remove(); removeEventListener("pointerdown", off, true); removeEventListener("keydown", off, true); } };
    addEventListener("pointerdown", off, true); addEventListener("keydown", off, true);
    m.querySelector("button").focus();
  }
  return { menu, h, icon, btn, iconBtn, progress, ring, list, toggle, segmented, empty, field, toast, sheet, sfx, buzz, muted, cheerRight, cheerMiss, endRun, petalRain, countUp, calm, floatText };
})();
