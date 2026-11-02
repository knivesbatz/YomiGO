// Runs on web pages. It reads only the text you select, sends nothing anywhere, and stores saved words locally.
(() => {
  const store = chrome.storage.local; // Firefox also provides chrome.*, with callbacks
  let host = null;
  const JP = /[぀-ヿ㐀-鿿]/;

  function close() { if (host) { host.remove(); host = null; } }
  function el(tag, cssText, text) { const e = document.createElement(tag); if (cssText) e.style.cssText = cssText; if (text) e.textContent = text; return e; }

  function sentenceAround(sel) {
    const node = sel.anchorNode; if (!node) return "";
    const t = (node.textContent || "").trim();
    const at = t.indexOf(sel.toString());
    if (at < 0) return t.slice(0, 120);
    let a = at, b = at;
    while (a > 0 && !"。！？\n".includes(t[a - 1])) a--;
    while (b < t.length && !"。！？\n".includes(t[b])) b++;
    return t.slice(a, Math.min(b + 1, t.length)).slice(0, 200);
  }
  function save(card, sentence, button) {
    store.get({ saved: [] }, ({ saved }) => {
      if (!saved.some((s) => s.id === card.id && s.sentence === sentence)) saved.push({ id: card.id, word: card.f, reading: card.r, meaning: card.en, sentence, page: location.hostname, at: Date.now() });
      store.set({ saved }, () => { button.textContent = "Saved"; button.disabled = true; });
    });
  }
  function show(text, rect, sentence) {
    close();
    const hits = YG.lookup(text);
    host = document.createElement("div");
    // Shadow DOM keeps the page's styles out of the popup, and ours out of the page
    const shadow = host.attachShadow({ mode: "closed" });
    const x = Math.max(8, Math.min(rect.left + scrollX, scrollX + innerWidth - 340)), y = rect.bottom + scrollY + 6;
    host.style.cssText = `position:absolute;left:${x}px;top:${y}px;z-index:2147483647`;
    const box = el("div", "width:320px;background:#fdf6f3;color:#34202a;border:1px solid #c2436b;border-radius:10px;padding:10px;font:14px/1.45 system-ui,'Noto Sans JP',sans-serif;box-shadow:0 4px 18px #0004");
    if (!hits.length) box.append(el("div", "color:#7d6670", `"${text.slice(0, 20)}" isn't in YomiGo's course words yet.`));
    for (const { card } of hits) {
      const row = el("div", "padding:4px 0;border-bottom:1px solid #f0dfe3");
      row.append(el("div", "font-size:20px;color:#c2436b", card.f), el("div", "color:#7d6670", card.r !== card.f ? `${card.r} (${card.ro})` : card.ro), el("div", "", card.en));
      const b = el("button", "margin-top:4px;padding:4px 10px;border-radius:8px;border:1px solid #c2436b;background:#fff;color:#34202a;cursor:pointer", "Save with sentence");
      b.addEventListener("click", () => save(card, sentence, b));
      row.append(b);
      box.append(row);
    }
    box.append(el("div", "color:#7d6670;font-size:11px;margin-top:6px", "YomiGo Reader. Esc to close."));
    shadow.append(box);
    document.documentElement.append(host);
  }
  document.addEventListener("mouseup", (e) => {
    if (host && e.composedPath().includes(host)) return;
    const sel = getSelection(), text = sel ? sel.toString().trim() : "";
    if (!text || text.length > 40 || !JP.test(text)) { close(); return; }
    store.get({ enabled: true }, ({ enabled }) => { if (enabled) show(text, sel.getRangeAt(0).getBoundingClientRect(), sentenceAround(sel)); });
  });
  document.addEventListener("keydown", (e) => { if (e.key === "Escape") close(); });
})();
