#!/usr/bin/env python3
"""Cuts the downloaded JMdict and KanjiVG down to the two small files YomiGo ships in content/dict/.
Run with -I, since it reads downloaded files. usage: python -I tools/extract_data.py JMdict_e.gz kanjivg.xml.gz"""
import gzip, io, json, os, re, sys
import xml.etree.ElementTree as ET

sys.stdout.reconfigure(encoding="utf-8")
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "content", "dict")
os.makedirs(OUT, exist_ok=True)
PRIORITY = {"news1", "ichi1", "spec1", "spec2", "gai1"}  # JMdict's own marks for common words

def clean(s):
    return re.sub(r"[\t\r\n]+", " ", s).strip()

def dictionary(path):
    rows, seen = [], set()
    with gzip.open(path) as f:
        for _, e in ET.iterparse(f):
            if e.tag != "entry": continue
            kes = [k for k in e.findall("k_ele") if any(p.text in PRIORITY for p in k.findall("ke_pri"))]
            res = [r for r in e.findall("r_ele") if any(p.text in PRIORITY for p in r.findall("re_pri"))]
            if kes or res:
                word = kes[0].findtext("keb") if kes else ""
                reading = (res[0] if res else e.find("r_ele")).findtext("reb")
                glosses = []
                for s in e.findall("sense"):
                    if len(glosses) >= 4: break
                    for g in s.findall("gloss")[:2]:
                        if g.text and len(glosses) < 4: glosses.append(clean(g.text))
                key = (word, reading)
                if glosses and key not in seen:
                    seen.add(key)
                    rows.append(f"{word}\t{reading}\t{'; '.join(glosses)[:90]}")
            e.clear()
    rows.sort(key=lambda r: r.split(chr(9))[1])
    return rows

NUM = re.compile(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?")
def flatten(d):
    """SVG path to a list of whole-number points. KanjiVG only uses M, C and S, in both cases."""
    pts, cur, start, last_ctrl, i = [], (0.0, 0.0), None, None, 0
    tokens = re.findall(r"[MmCcSsLlZz]|" + NUM.pattern, d)
    cmd = None
    def take(n):
        nonlocal i
        v = [float(t) for t in tokens[i:i + n]]
        i += n
        return v
    while i < len(tokens):
        if tokens[i].isalpha(): cmd = tokens[i]; i += 1
        rel = cmd.islower(); c = cmd.upper()
        ox, oy = cur if rel else (0, 0)
        if c == "M":
            x, y = take(2); cur = (x + ox, y + oy); pts.append(cur); last_ctrl = None
            cmd = "l" if rel else "L"
        elif c == "L":
            x, y = take(2); cur = (x + ox, y + oy); pts.append(cur); last_ctrl = None
        elif c in "CS":
            if c == "C":
                a = take(6); p1 = (a[0] + ox, a[1] + oy); p2 = (a[2] + ox, a[3] + oy); p3 = (a[4] + ox, a[5] + oy)
            else:
                a = take(4); p1 = (2 * cur[0] - last_ctrl[0], 2 * cur[1] - last_ctrl[1]) if last_ctrl else cur
                p2 = (a[0] + ox, a[1] + oy); p3 = (a[2] + ox, a[3] + oy)
            for t in (0.25, 0.5, 0.75, 1.0):
                u = 1 - t
                pts.append(tuple(u * u * u * cur[k] + 3 * u * u * t * p1[k] + 3 * u * t * t * p2[k] + t * t * t * p3[k] for k in (0, 1)))
            cur, last_ctrl = p3, p2
        else:
            i += 0
    out = []
    for x, y in pts:
        p = (round(x), round(y))
        if not out or out[-1] != p: out.append(p)
    return out

def strokes(path, wanted):
    got = {}
    with gzip.open(path) as f:
        for _, e in ET.iterparse(f):
            if e.tag != "kanji": continue
            m = re.search(r"kanji_([0-9a-f]{5})$", e.get("id", ""))
            if m and chr(int(m.group(1), 16)) in wanted:
                paths = list(e.iter("path"))
                order = lambda p: int(re.search(r"s(\d+)$", p.get("id", "s0")).group(1))
                got[chr(int(m.group(1), 16))] = [flatten(p.get("d")) for p in sorted(paths, key=order)]
            e.clear()
    return got

C = json.load(io.open(os.path.join(ROOT, "build", "gen", "content.json"), encoding="utf-8"))
want = {c["f"] for c in C["cards"] if c["day"] >= 0 and c["t"] in "hkj" and len(c["f"]) == 1}
rows = dictionary(sys.argv[1])
st = strokes(sys.argv[2], want)
missing = sorted(want - set(st))
assert not missing, f"no stroke data for {missing}"
assert all(len(p) >= 2 for g in st.values() for p in g), "a stroke flattened to nothing"
with io.open(os.path.join(OUT, "jmdict_common.tsv"), "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(rows) + "\n")
with io.open(os.path.join(OUT, "strokes.tsv"), "w", encoding="utf-8", newline="\n") as f:
    for ch in sorted(st):
        f.write(ch + "\t" + "|".join(" ".join(f"{x},{y}" for x, y in s) for s in st[ch]) + "\n")
print(f"dictionary {len(rows)} entries, strokes for {len(st)} characters; 五 has {len(st.get('五', []))} strokes, あ has {len(st.get('あ', []))}, 日 has {len(st.get('日', []))}")
