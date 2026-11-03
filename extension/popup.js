const store = chrome.storage.local; // Firefox also provides chrome.*, with callbacks
const $ = (id) => document.getElementById(id);
// Tabs and newlines would break a row in Anki's import
const cell = (s) => String(s || "").replace(/[\t\r\n]+/g, " ");

function render() {
  store.get({ saved: [], enabled: true }, ({ saved, enabled }) => {
    $("on").checked = enabled;
    $("count").textContent = `${saved.length} saved word(s)`;
    $("list").replaceChildren(...saved.slice(-20).reverse().map((s) => { const li = document.createElement("li"); li.textContent = `${s.word}: ${s.meaning}`; return li; }));
  });
}
$("on").addEventListener("change", (e) => store.set({ enabled: e.target.checked }));
$("anki").addEventListener("click", () => store.get({ saved: [] }, ({ saved }) => {
  const rows = saved.map((s) => [s.word, s.reading, s.meaning, s.sentence, s.page].map(cell).join("\t"));
  const url = URL.createObjectURL(new Blob(["#separator:tab\n#columns:Word\tReading\tMeaning\tSentence\tSource\n" + rows.join("\n") + "\n"], { type: "text/tab-separated-values" }));
  const a = document.createElement("a"); a.href = url; a.download = "yomigo-words.tsv"; a.click();
  setTimeout(() => URL.revokeObjectURL(url), 5000);
}));
// Then in YomiGo: Flashcards > My words > Import
$("copy").addEventListener("click", () => store.get({ saved: [] }, ({ saved }) => {
  const text = saved.map((s) => [s.word, s.reading, s.meaning, s.sentence].map(cell).join("\t")).join("\n");
  navigator.clipboard.writeText(text).then(() => { $("count").textContent = `Copied ${saved.length} word(s). In YomiGo: Flashcards > My words > Import.`; }, () => { $("count").textContent = "Couldn't copy. Use the Anki export instead."; });
}));
$("clear").addEventListener("click", () => { if (confirm("Delete all saved words from this browser?")) store.set({ saved: [] }, render); });
render();
