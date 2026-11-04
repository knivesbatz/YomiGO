// Where YomiGo is running: the Android app (YomiBridge), the Windows app (Tauri, in Rust), or a plain browser.
// Screens ask Host, never the platform, so the same UI runs on all three.
"use strict";
const Host = (() => {
  const Bridge = globalThis.YomiBridge || null;
  const T = globalThis.__TAURI__ || null;
  const invoke = T ? T.core.invoke : null;
  const H = { android: !!Bridge, desktop: !!T, tv: false, version: Bridge ? Bridge.version() : "web", info: {} };
  const cache = new Map(); // desktop saves, read once at start, then written through

  H.ready = (async () => {
    if (!T) return;
    H.info = await invoke("host");
    H.version = H.info.version; H.tv = H.info.tv;
    for (const [n, t] of await invoke("saves")) cache.set(n, t);
  })().catch((e) => { console.error(e); });

  // ---- saves: one text save per learner ----
  H.store = {
    list() {
      if (T) return [...cache.keys()];
      try { return JSON.parse(localStorage.getItem("yg.names") || "[]"); } catch { return []; }
    },
    text(n) { if (T) return cache.get(n) || ""; try { return localStorage.getItem("yg.save." + n) || ""; } catch { return ""; } },
    write(n, text) {
      if (T) { cache.set(n, text); return invoke("save", { name: n, text }).catch((e) => UI.toast("Couldn't save: " + e, "bad")); }
      try {
        localStorage.setItem("yg.save." + n, text);
        const l = H.store.list(); if (!l.includes(n)) { l.push(n); localStorage.setItem("yg.names", JSON.stringify(l)); }
      } catch (e) { UI.toast("Couldn't save: " + e.message, "bad"); }
    },
  };

  // ---- speech ----
  H.speak = (t) => {
    if (Bridge) return Bridge.speak(t);
    if (!globalThis.speechSynthesis) return;
    speechSynthesis.cancel();
    const u = new SpeechSynthesisUtterance(t); u.lang = "ja-JP";
    const v = speechSynthesis.getVoices().find((x) => x.lang.startsWith("ja")); if (v) u.voice = v;
    speechSynthesis.speak(u);
  };
  if (globalThis.speechSynthesis) speechSynthesis.getVoices(); // voices load late; ask early
  H.canSpeak = () => !!globalThis.YG_TEST_VOICE || (Bridge ? Bridge.hasVoice() : !!globalThis.speechSynthesis && speechSynthesis.getVoices().some((v) => v.lang.startsWith("ja")));
  const Rec = globalThis.SpeechRecognition || globalThis.webkitSpeechRecognition;
  H.canListen = () => !!Bridge || (!T && !!Rec);
  let onHeard = null;
  globalThis.ygHeard = (text) => { if (onHeard) onHeard(text); };
  H.listen = (cb) => {
    onHeard = cb;
    if (Bridge) return Bridge.listen();
    if (!Rec) return;
    const r = new Rec(); r.lang = "ja-JP"; r.onresult = (e) => cb(e.results[0][0].transcript); r.onerror = () => cb(""); r.start();
  };
  H.voiceHelp = () => {
    if (Bridge) return Bridge.installVoice();
    UI.sheet("Add a Japanese voice", T ? "In Windows Settings, open Time & language, then Speech, and add Japanese voices. Then restart YomiGo. Classes still work without one, and skip listening." : "Listening needs a Japanese voice on this device.", []);
  };

  H.open = (u) => { if (Bridge) Bridge.openUrl(u); else if (T) invoke("open", { target: u }).catch((e) => UI.toast(String(e), "bad")); else window.open(u, "_blank", "noopener"); };
  H.checkUpdate = async () => {
    if (Bridge) return Bridge.checkUpdate();
    if (T) return (await invoke("update")) ? UI.toast("Checking for updates…") : UI.toast("Updates need YomiGoUpdate.exe beside YomiGo.", "bad");
    UI.toast("Updates apply to the apps. This page is always current.");
  };
  H.ocrPick = Bridge && Bridge.ocrPick ? () => Bridge.ocrPick() : null;

  // ---- Windows only ----
  H.call = (cmd, args) => invoke(cmd, args);
  H.on = (ev, fn) => T.event.listen(ev, (e) => fn(e.payload));
  return H;
})();
