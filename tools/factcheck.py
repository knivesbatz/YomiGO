#!/usr/bin/env python3
"""Checks every course word and kanji against the full JMdict. Needs the download, so it runs on demand, not in CI.
usage: python -I tools/factcheck.py path/to/JMdict_e.gz path/to/kanjivg.xml.gz  > docs/FACTCHECK.md"""
import gzip, json, os, re, sys
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.stdout.reconfigure(encoding="utf-8")
C = json.load(open(os.path.join(ROOT, "build", "gen", "content.json"), encoding="utf-8"))

def hira(s):
    return "".join(chr(ord(c) - 0x60) if 0x30A1 <= ord(c) <= 0x30F6 else c for c in s)

forms, readings, gloss = {}, {}, {}
with gzip.open(sys.argv[1]) as f:
    for _, e in ET.iterparse(f):
        if e.tag != "entry": continue
        ks = [k.text for k in e.findall("k_ele/keb")]
        rs = [r.text for r in e.findall("r_ele/reb")]
        gs = [g.text for g in e.findall("sense/gloss")]
        for k in ks: forms.setdefault(k, set()).update(hira(r) for r in rs); gloss.setdefault(k, []).extend(gs)
        for r in rs: readings.setdefault(hira(r), []).extend(gs)
        e.clear()

strokes = {}
with gzip.open(sys.argv[2]) as f:
    for _, e in ET.iterparse(f):
        if e.tag == "kanji":
            kid = e.get("id", "")
            m = re.search(r"kanji_([0-9a-f]{5})$", kid)
            if m: strokes[chr(int(m.group(1), 16))] = sum(1 for p in e.iter("path"))
            e.clear()

STOP = set("a an the to of in on at for with is are be do does did it this that as by or and one (s) etc thing".split())
def words(s):
    return {w for w in re.findall(r"[a-z]+", s.lower()) if w not in STOP}

# The course teaches polite verbs first, and dictionaries list plain forms, so these are checked as plain forms
PLAIN = {"分かります": ("分かる", "わかる"), "分かりません": ("分かる", "わかる"), "食べます": ("食べる", "たべる"), "飲みます": ("飲む", "のむ"),
         "行きます": ("行く", "いく"), "来ます": ("来る", "くる"), "帰ります": ("帰る", "かえる"), "見ます": ("見る", "みる"), "読みます": ("読む", "よむ"),
         "聞きます": ("聞く", "きく"), "書きます": ("書く", "かく"), "話します": ("話す", "はなす"), "買います": ("買う", "かう"), "します": ("する", "する"),
         "あります": ("ある", "ある"), "います": ("いる", "いる"),
         "乗ります": ("乗る", "のる"), "運転します": ("運転", "うんてん"), "着ます": ("着る", "きる"), "履きます": ("履く", "はく"),
         "遊びます": ("遊ぶ", "あそぶ"), "踊ります": ("踊る", "おどる"), "飼います": ("飼う", "かう"),
         "洗います": ("洗う", "あらう"), "探します": ("探す", "さがす"), "笑います": ("笑う", "わらう"), "捨てます": ("捨てる", "すてる"),
         "出かけます": ("出かける", "でかける"), "持ちます": ("持つ", "もつ"), "助けます": ("助ける", "たすける"), "似合います": ("似合う", "にあう"),
         "呼びます": ("呼ぶ", "よぶ"), "着きます": ("着く", "つく"), "始まります": ("始まる", "はじまる"), "見せます": ("見せる", "みせる"),
         "曲がります": ("曲がる", "まがる"), "消します": ("消す", "けす"), "生まれます": ("生まれる", "うまれる"), "頑張ります": ("頑張る", "がんばる"),
         "空きます": ("空く", "すく"),
         "取ります": ("取る", "とる"), "置きます": ("置く", "おく"), "返します": ("返す", "かえす"), "売ります": ("売る", "うる"),
         "習います": ("習う", "ならう"), "答えます": ("答える", "こたえる"), "つけます": ("点ける", "つける"), "切ります": ("切る", "きる"),
         "開きます": ("開く", "あく"), "届きます": ("届く", "とどく"), "困ります": ("困る", "こまる"), "動きます": ("動く", "うごく"),
         "晴れます": ("晴れる", "はれる"), "曇ります": ("曇る", "くもる"), "吹きます": ("吹く", "ふく"), "咲きます": ("咲く", "さく"), "止みます": ("止む", "やむ")}
# Read by a person: each is right, and only worded differently from JMdict, or a kanji's meaning rather than a word's
REVIEWED = {"j.okiru", "j.akarui", "j.kiru", "j.mono", "j.tooru", "j.sumu", "j.kasu", "j.ten2", "j.shi", "j.hatsu", "j.ei", "j.ugoku", "j.atsumeru", "j.ken", "j.tateru", "j.chuu", "v.komarimasu", "v.kirai", "v.hondana", "v.juui", "v.kousaten", "v.naosu", "v.daitai", "v.shikamo", "v.choushi", "v.ririku", "v.gomibako", "v.shitaku", "v.kaeri", "v.kinchou", "v.mitomeru", "v.teishutsu", "v.akusesarii", "v.anohito", "v.shouhin", "v.kensa", "v.kousokudouro", "v.shukuhaku", "v.perapera", "v.kesshite", "v.fuan", "v.zannen", "v.daikirai", "v.chan", "v.koukoku", "v.fuben", "v.katte", "v.okonau", "v.boshuu", "v.kinshi", "v.kamigata", "v.joubu", "v.suugaku", "v.suru", "v.jouzu", "j.tsukuru", "j.tsukau", "j.motsu", "j.hairu", "j.aruku", "j.ooi", "j.iu", "j.shiru", "j.kangaeru", "j.oshieru", "v.shimasu", "v.arimasu", "v.itadakimasu", "v.gochisousama", "v.desu", "v.ichi", "v.heta", "v.mono", "v.manga", "v.suki", "v.imasu", "v.hitotsu", "v.eeto", "v.hontou",
            "j.ichi", "j.iku", "j.kuru", "j.taberu", "j.miru", "j.kaku", "j.hanasu", "j.den", "j.takai",
            # N3, checked by hand in 0.8
            "j.yuu2", "v.totsuzen", "v.omoide", "j.kimeru", "j.kanarazu", "j.ushinau", "j.mamoru", "j.kotowaru", "j.nokoru", "j.shiraberu", "j.tsutaeru",
            # 0.11: 降ります has two readings (降りる, 降る); set phrases that aren't headwords as written; meanings in other words
            "v.orimasu", "v.furimasu", "v.otsukaresama", "v.ohashi", "v.sounanda", "v.majide", "v.daisuki", "v.shusshin", "v.yasai", "v.tashikani", "v.mattaku", "v.untenshimasu", "v.anzen"}
missing, mismatch, meaning, okc = [], [], [], 0
for c in C["cards"]:
    if c["day"] < 0 or c["t"] not in "wj": continue
    f, r = c["f"], hira(c["r"])
    if f in PLAIN: f, r = PLAIN[f]
    if f in forms:
        if r not in forms[f]:
            # kanji cards may hold a stem, such as 大 for 大きい
            # REVIEWED also covers a kanji whose taught reading comes from a compound, such as 由 in 理由
            if c["id"] not in REVIEWED and not (c["t"] == "j" and any(k.startswith(f) and any(x.startswith(r) for x in forms[k]) for k in forms if len(k) <= 4)):
                mismatch.append((c["id"], f, r, sorted(forms[f])[:6]))
                continue
        gl = gloss[f]
    elif r in readings and hira(f) == r:
        gl = readings[r]
    elif c["t"] == "j" and any(k.startswith(f) and any(x.startswith(r) for x in forms[k]) for k in forms if len(k) <= 4):
        okc += 1
        continue
    else:
        if c["id"] in REVIEWED: okc += 1; continue # set phrases, checked by hand
        missing.append((c["id"], f, r))
        continue
    if c["id"] not in REVIEWED and not (words(c["en"]) & words(" ".join(gl))):
        meaning.append((c["id"], f, c["en"], "; ".join(gl[:5])))
    else:
        okc += 1

nostroke = [c["f"] for c in C["cards"] if c["t"] == "j" and c["day"] >= 0 and c["f"] not in strokes]

print("# Course fact-check\n")
print("Generated by `tools/factcheck.py` against JMdict (word, reading, and meaning) and KanjiVG (kanji present).\n")
print("**What this doesn't cover:** grammar explanations, lesson notes, and example sentences, a kanji's listed on and kun readings beyond its main one, and memory hooks. A meaning passes if it shares a word with JMdict's, so a subtly wrong meaning can still pass.\n")
print(f"- Words and kanji checked and agreeing: **{okc}**")
print(f"- Spelling not found in JMdict: **{len(missing)}**")
print(f"- Reading disagrees with JMdict: **{len(mismatch)}**")
print(f"- Meaning shares no word with JMdict, so a person must judge: **{len(meaning)}**")
print(f"- Kanji missing from KanjiVG: **{len(nostroke)}**\n")
def table(title, head, rows):
    print(f"## {title}\n")
    if not rows: print("None.\n"); return
    print("| " + " | ".join(head) + " |"); print("|" + "---|" * len(head))
    for r in rows: print("| " + " | ".join(str(x).replace("|", "/") for x in r) + " |")
    print()
table("Not found", ["Card", "Spelling", "Reading"], missing)
table("Reading disagrees", ["Card", "Spelling", "Course", "JMdict"], mismatch)
table("Meaning to review", ["Card", "Spelling", "Course says", "JMdict says"], meaning)
