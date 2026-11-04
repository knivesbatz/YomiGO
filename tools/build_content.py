#!/usr/bin/env python3
"""Validate content/ and generate the C header and JS data every YomiGo app uses."""
import io, json, os, re, sys, unicodedata

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONTENT = os.path.join(ROOT, "content")
GEN = os.path.join(ROOT, "build", "gen")

# romaji, hiragana, katakana, hiragana memory hook, katakana memory hook
BASE = """a あ ア A cross with a curly loop: someone says "ah!" while tying a ribbon.|Looks like a sharp "A" leaning left.
i い イ Two reeds, standing side by side in the breeze: "ee".|A lean-to with one straight pole: "ee!"
u う ウ A person bending over with a small hat: "oo, my back".|A roof over a U shape.
e え エ An exotic bird tilting its head: "eh?"|A steel I-beam for "engineering".
o お オ A golfer with a ball off to the side: "oh!"|A person throwing arms wide: "oh!"
ka か カ A blade cutting with a spark: "ka!"|Same as か without its spark.
ki き キ A key with two teeth.|A key with its two teeth, simplified.
ku く ク A bird's beak: "coo".|A bird's beak with a little head.
ke け ケ A gate and a keg beside it.|A slanted keg on a post.
ko こ コ Two worms, coiled and cozy.|A corner of a box, two strokes.
sa さ サ A cross with a sad little smile underneath.|Two sticks tied across a bar.
shi し シ A long fishing hook: "she" went fishing.|A smile, flat, with eyes above: "she" smiles.
su す ス A loop in a swing: "sue" swings.|A person sliding down: "suuu".
se せ セ A mouth wide open, saying "say!"|A mouth with a bent line.
so そ ソ A zig-zag sewing thread.|A needle and one stitch, upright.
ta た タ "t" and "a" drawn beside each other.|A top with a slash: like a falling "T".
chi ち チ A cheerleader's number 5: "chee!"|A bird on a branch: "cheep".
tsu つ ツ A wave breaking: a "tsu-nami".|Three drops, upright, falling from a tsunami.
te て テ A palm held out: "te" means hand.|A T with a roof.
to と ト A toe with a nail hammered in.|A stick with a toe sticking out.
na な ナ A cross and a knot.|A cross, like a nurse's sign.
ni に ニ A needle (a stroke) beside two knees.|Two lines, like ni, two.
nu ぬ ヌ Noodles with a loop.|A chopstick stirring a noodle.
ne ね ネ A cat curling up its tail: "neko".|A nest on a post.
no の ノ A big "No" sign, one round stroke.|A single slash: no.
ha は ハ A person laughing: "ha!" with a stick.|Two legs: "ha ha" spread wide.
hi ひ ヒ A big grin: "hee hee".|A heel of a boot.
fu ふ フ A person blowing out a candle: "foo".|A hook, blowing to the side.
he へ ヘ A mountain "hey!" seen from far off.|Exactly like へ: a little mountain.
ho ほ ホ A tall gate and a Santa saying "ho ho".|A cross with two arms out: "ho!"
ma ま マ A mama with arms and a looped skirt.|A corner, "ma" pointing down.
mi み ミ A person sitting with a "me" cross.|Three lines of mi-so soup.
mu む ム A cow's face: "moo".|A little triangle-faced cow.
me め メ An eye with a crossed look.|A crossed-out eye.
mo も モ A fishhook with two more bars.|More bars on a post.
ya や ヤ A yak with horns.|A yak's horn from the side.
yu ゆ ユ A fish swimming "you" around.|A U-turn: "you" go back.
yo よ ヨ A yo-yo on a string.|A comb with three teeth.
ra ら ラ A rabbit with one ear up.|A rabbit's top and a slanted back.
ri り リ Two reeds by a river.|Two reeds, straight and clean.
ru る ル A route looping back.|Two legs walking a route.
re れ レ A person kneeling and ready.|A single lean, ready.
ro ろ ロ A road with no loop at its end.|A square road: a box.
wa わ ワ A wasp's body.|A wasp's wing: a curved roof.
wo を ヲ A person bowling a ball, "woah!"|A roof with two bars.
n ん ン A person bowing in a small "n".|A dot and a slope, flatter than ソ."""
DAK = [("ga","か","が","カ","ガ"),("gi","き","ぎ","キ","ギ"),("gu","く","ぐ","ク","グ"),("ge","け","げ","ケ","ゲ"),("go","こ","ご","コ","ゴ"),
("za","さ","ざ","サ","ザ"),("ji","し","じ","シ","ジ"),("zu","す","ず","ス","ズ"),("ze","せ","ぜ","セ","ゼ"),("zo","そ","ぞ","ソ","ゾ"),
("da","た","だ","タ","ダ"),("de","て","で","テ","デ"),("do","と","ど","ト","ド"),
("ba","は","ば","ハ","バ"),("bi","ひ","び","ヒ","ビ"),("bu","ふ","ぶ","フ","ブ"),("be","へ","べ","ヘ","ベ"),("bo","ほ","ぼ","ホ","ボ"),
("pa","は","ぱ","ハ","パ"),("pi","ひ","ぴ","ヒ","ピ"),("pu","ふ","ぷ","フ","プ"),("pe","へ","ぺ","ヘ","ペ"),("po","ほ","ぽ","ホ","ポ")]
YOUON = [("ky","き","キ"),("sh","し","シ"),("ch","ち","チ"),("ny","に","ニ"),("hy","ひ","ヒ"),("my","み","ミ"),("ry","り","リ"),
("gy","ぎ","ギ"),("j","じ","ジ"),("by","び","ビ"),("py","ぴ","ピ")]
ALT = {"si":"し","ti":"ち","tu":"つ","hu":"ふ","zi":"じ","di":"ぢ","du":"づ","sya":"しゃ","syu":"しゅ","syo":"しょ","tya":"ちゃ","tyu":"ちゅ","tyo":"ちょ",
"zya":"じゃ","zyu":"じゅ","zyo":"じょ","jya":"じゃ","jyu":"じゅ","jyo":"じょ","cya":"ちゃ","cyu":"ちゅ","cyo":"ちょ","la":"ら","li":"り","lu":"る","le":"れ","lo":"ろ",
"xa":"ぁ","xi":"ぃ","xu":"ぅ","xe":"ぇ","xo":"ぉ","xtu":"っ","xtsu":"っ","xya":"ゃ","xyu":"ゅ","xyo":"ょ","fa":"ふぁ","fi":"ふぃ","fe":"ふぇ","fo":"ふぉ","va":"ゔぁ","vi":"ゔぃ","vu":"ゔ","ve":"ゔぇ","vo":"ゔぉ","je":"じぇ","she":"しぇ","che":"ちぇ","ti2":"てぃ"}

def die(msg):
    print("CONTENT ERROR:", msg); sys.exit(1)

def kana_rows():
    rows = []
    for line in BASE.split("\n"):
        ro, h, k, rest = line.split(" ", 3)
        mh, mk = rest.split("|")
        rows.append((ro, h, k, mh, mk))
    for ro, bh, h, bk, k in DAK:
        mark = "two dashes" if ro[0] != "p" else "a small circle"
        rows.append((ro, h, k, f"{bh} with {mark}.", f"{bk} with {mark}."))
    for pre, h, k in YOUON:
        for v, sh, sk in (("a","ゃ","ャ"),("u","ゅ","ュ"),("o","ょ","ョ")):
            rows.append((pre+v, h+sh, k+sk, f"{h} + small {sh}, in one beat.", f"{k} + small {sk}, in one beat."))
    return rows

def romaji_table():
    t = {r[0]: r[1] for r in kana_rows()}
    t.update({k: v for k, v in ALT.items() if k != "ti2"})
    del t["n"]  # ん depends on what follows, so the converters handle it themselves
    return t

def hepburn_table():
    """Hiragana to Hepburn, for showing a reading in romaji: the class rows first, other spellings only for kana they don't cover"""
    t = {r[1]: r[0] for r in kana_rows() if r[0] != "n"}
    for ro, kana in ALT.items():
        if kana not in t: t[kana] = "ti" if ro == "ti2" else ro
    return t

VOW = {}
for row, vs in (("あかさたなはまやらわがざだばぱぁゃ","a"),("いきしちにひみりぎじぢびぴぃ","i"),("うくすつぬふむゆるぐずづぶぷぅゅ","u"),("えけせてねへめれげぜでべぺぇ","e"),("おこそとのほもよろをごぞどぼぽぉょ","o")):
    for c in row: VOW[c] = vs
VK = {"a":"あ","i":"い","u":"う","e":"え","o":"お"}

def to_hira(s):
    out = []
    for c in s:
        n = ord(c)
        if 0x30A1 <= n <= 0x30F6: c = chr(n - 0x60)
        if c == "ー" and out: c = VK.get(VOW.get(out[-1], ""), "")
        out.append(c)
    return "".join(out)

def romaji_to_kana(s, table):
    s = s.lower().replace(" ", "")
    out, i = "", 0
    keys = sorted(table, key=len, reverse=True)
    while i < len(s):
        for k in keys:
            if s.startswith(k, i):
                out += table[k]; i += len(k); break
        else:
            c = s[i]
            if c == "n":
                nxt = s[i+1:i+2]
                if nxt == "'": i += 1
                elif nxt == "n" and s[i+2:i+3] not in tuple("aiueoy"): i += 1
                out += "ん"; i += 1
            elif c == s[i+1:i+2] or (c == "t" and s[i+1:i+2] == "c"):
                out += "っ"; i += 1
            elif c == "'":
                i += 1
            elif c == "-":
                out = to_hira(out + "ー"); i += 1
            else:
                out += c; i += 1
    return out

def kana_to_romaji(s):
    """Hepburn spelling of a reply, for the reveal when a beginner can't read kana yet."""
    rows = {r[1]: r[0] for r in kana_rows()}
    s, out, i = to_hira(s).replace("。", ""), "", 0
    while i < len(s):
        two, one = s[i:i+2], s[i]
        if one == "を": out += "o"; i += 1  # only ever a particle, said o
        elif one in "はへ" and i == len(s) - 1: out += "wa" if one == "は" else "e"; i += 1  # said wa and e at the end, as in こんにちは
        elif two in rows: out += rows[two]; i += 2
        elif one == "っ": out += "@"; i += 1
        elif one == "ん":
            # before a vowel or y it needs the apostrophe, or it reads as a different sound
            nx = "o" if s[i+1:i+2] == "を" else "e" if s[i+1:] == "へ" else rows.get(s[i+1:i+3]) or rows.get(s[i+1:i+2]) or ""
            out += "n'" if nx[:1] in tuple("aiueoy") and nx else "n"; i += 1
        elif one in rows: out += rows[one]; i += 1
        else: die(f"can't spell {one} in {s} in romaji")
    while "@" in out:
        at = out.index("@"); nx = out[at + 1]
        out = out[:at] + ("t" if nx == "c" else nx) + out[at + 1:]
    return out

def tsv(name):
    group, rows = None, []
    with io.open(os.path.join(CONTENT, name), encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\r\n")
            if not line: continue
            if line.startswith("# "): continue
            if line.startswith("#"):
                group = line[1:]; continue
            rows.append((group, line.split("\t")))
    return rows

def load_cards():
    table = romaji_table()
    cards, groups = [], {}
    def add(c, group=None):
        c["i"] = len(cards); cards.append(c)
        if group: groups.setdefault(group, []).append(c["id"])
    for ro, h, k, mh, mk in kana_rows():
        if romaji_to_kana(ro, table) != h: die(f"kana table: {ro} -> {romaji_to_kana(ro, table)} != {h}")
        add({"id": "h."+ro, "t": "h", "f": h, "r": h, "ro": ro, "en": ro, "n": mh})
        add({"id": "k."+ro, "t": "k", "f": k, "r": k, "ro": ro, "en": ro, "n": mk})
    for g, r in tsv("words.tsv"):
        if len(r) < 5: die(f"words.tsv: bad row {r}")
        id_, disp, kana, ro, en = r[:5]
        got, want = romaji_to_kana(ro, table), to_hira(kana)
        if len(r) > 5 and r[5] == "p":
            want = want.replace("は", "わ").replace("を", "お").replace("へ", "え")
        # Hepburn writes づ and ぢ as zu and ji, as they're said
        got, want = got.replace("づ", "ず").replace("ぢ", "じ"), want.replace("づ", "ず").replace("ぢ", "じ")
        if got != want: die(f"words.tsv {id_}: romaji {ro} gives {got}, kana says {want}")
        c = {"id": "v."+id_, "t": "w", "f": disp, "r": kana, "ro": ro, "en": en, "n": ""}
        if len(r) > 7 and r[6]: # an example sentence, and its English
            stem = disp[:-1] if len(disp) > 1 else disp
            lead = re.match(r"[^぀-ゟ]+", disp) # the kanji part, which conjugation never changes: 飼 in 飼っています
            if not r[7]: die(f"words.tsv {id_}: the example has no English")
            if not any(x in r[6] for x in (disp, kana, stem, kana[:-1], lead.group(0) if lead else disp)): die(f"words.tsv {id_}: the example doesn't use its word")
            c["s"], c["tr"] = r[6], r[7]
        add(c, g)
    for g, r in tsv("kanji.tsv"):
        id_, kj, kana, ro, en, readings, hook = r
        if romaji_to_kana(ro, table) != kana: die(f"kanji.tsv {id_}: {ro} != {kana}")
        add({"id": id_, "t": "j", "f": kj, "r": kana, "ro": ro, "en": en, "n": f"{readings}\n{hook}"}, g)
    for g, r in tsv("grammar.tsv"):
        id_, pat, ro, mean, expl, sent, ans, wrong, trans = r
        if "＿" not in sent: die(f"grammar {id_}: no blank in sentence")
        if ans in wrong.split(","): die(f"grammar {id_}: answer is listed as wrong")
        add({"id": id_, "t": "g", "f": pat, "r": ans, "ro": ro, "en": mean, "n": expl,
             "s": sent, "w": wrong.split(","), "tr": trans}, g)
    ids = [c["id"] for c in cards]
    if len(ids) != len(set(ids)): die("duplicate card ids: " + str({x for x in ids if ids.count(x) > 1}))
    return cards, groups

def load_chats():
    chats, cur = {}, None
    with io.open(os.path.join(CONTENT, "chats.txt"), encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"): continue
            if line.startswith("@chat "):
                id_, title, ja, who = [x.strip() for x in line[6:].split("|")]
                cur = {"id": id_, "title": title, "ja": ja, "who": who, "turns": []}
                chats[id_] = cur; pending = None
            elif line.startswith("say:"):
                pending = [x.strip() for x in line[4:].split("|")]
            elif line.startswith("want:"):
                w, hint = [x.strip() for x in line[5:].split("|")]
                say = pending or ["", "", ""]
                cur["turns"].append({"say": say[0], "ro": say[1], "en": say[2], "want": w.split(";"), "hint": hint, "wantRo": kana_to_romaji(w.split(";")[0])})
                pending = None
    return chats

# A class teaches at most this many new cards. More, and the rest go to Part 2, Part 3, and so on, cut between groups,
# each with its own practice and quiz. Saves name a class by where it sits in course.txt and which part, so they keep their place.
CLASS_MAX = 25
def split_days(days, exams, stages):
    out, first = [], []
    for i, d in enumerate(days):
        first.append(len(out))
        parts = [[]]
        for ch in d.pop("chunks"):
            if parts[-1] and len(parts[-1]) + len(ch) > CLASS_MAX: parts.append([])
            parts[-1] += ch
        d["new"], d["src"], d["part"] = parts[0], i, 1
        out.append(d)
        for k, more in enumerate(parts[1:], 2):
            out.append({"stage": d["stage"], "title": f"{d['title']}, part {k}", "ja": f"{d['ja']} その{k}", "src": i, "part": k,
                        "notes": [f"Part {k} of {d['title']}: more words and kanji on the same topic, with their own practice and quiz."],
                        "new": more, "chat": "", "read": "", "mat": "", "exam": -1,
                        "hw": "Say three of today's new words in sentences of your own, out loud.",
                        "imm": "Listen and look for today's new words in something Japanese you enjoy."})
        if len(parts) > 1 and d["exam"] >= 0: out[-1]["exam"], d["exam"] = d["exam"], -1  # the exam comes after the last part
    for e in exams: e["day"] = next(j for j, d in enumerate(out) if d["exam"] == exams.index(e))
    for s in stages: s["first"] = first[s["first"]]
    return out, exams, stages

def load_course(cards, groups, chats, reading):
    byid = {c["id"]: c for c in cards}
    stages, days, exams = [], [], []
    with io.open(os.path.join(CONTENT, "course.txt"), encoding="utf-8") as f:
        day = None
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"): continue
            if line.startswith("@stage "):
                n, name, ja, desc = [x.strip() for x in line[7:].split("|")]
                stages.append({"name": name, "ja": ja, "desc": desc, "first": len(days)})
            elif line.startswith("@day "):
                t, ja = [x.strip() for x in line[5:].split("|")]
                day = {"stage": len(stages)-1, "title": t, "ja": ja, "notes": [], "new": [], "chunks": [], "chat": "", "hw": "", "imm": "", "mat": "", "exam": -1, "read": ""}
                days.append(day)
            else:
                k, v = line.split(":", 1); v = v.strip()
                if k == "note": day["notes"].append(v)
                elif k == "new":
                    for tok in v.split():
                        if tok[0] == "@":
                            if tok[1:] not in groups: die(f"course: unknown group {tok}")
                            day["new"] += groups[tok[1:]]; day["chunks"].append(list(groups[tok[1:]]))
                        else:
                            if tok[1:] not in byid: die(f"course: unknown card {tok}")
                            day["new"].append(tok[1:]); day["chunks"].append([tok[1:]])
                elif k == "chat":
                    if v not in chats: die(f"course: unknown chat {v}")
                    day["chat"] = v
                elif k == "read":
                    if v not in reading: die(f"course: unknown reading {v}")
                    day["read"] = v
                elif k == "homework": day["hw"] = v
                elif k == "immerse": day["imm"] = v
                elif k == "material": day["mat"] = v
                elif k == "exam":
                    id_, name, stars = [x.strip() for x in v.split("|")]
                    day["exam"] = len(exams)
                    exams.append({"id": id_, "name": name, "stars": int(stars), "day": len(days)-1})
                else: die(f"course: unknown key {k}")
    days, exams, stages = split_days(days, exams, stages)
    seen = {}
    for i, d in enumerate(days):
        if not d["notes"]: die(f"day {i}: no notes")
        if not d["hw"] or not d["imm"]: die(f"day {i}: homework and immersion are required")
        for c in d["new"]:
            if c in seen: die(f"card {c} taught on two days ({seen[c]} and {i})")
            seen[c] = i
    for g, ids in groups.items():
        for c in ids:
            if c not in seen: die(f"card {c} in group {g} is never taught")
    unused = [k for k in chats if k not in {d["chat"] for d in days}]
    if unused: die(f"chats never used: {unused}")
    unused = [k for k in reading if k not in {d["read"] for d in days}]
    if unused: die(f"readings never used: {unused}")
    # Kana are worth drilling only if the course reaches them
    for c in cards:
        c["day"] = seen.get(c["id"], -1)
    return stages, days, exams

def load_genki(cards, groups):
    byid = {c["id"] for c in cards}
    out = []
    with io.open(os.path.join(CONTENT, "genki.tsv"), encoding="utf-8") as f:
        for line in f:
            line = line.rstrip()
            if not line or line.startswith("#"): continue
            lesson, topics, toks = line.split(chr(9))
            ids = []
            for t in toks.split():
                got = groups[t[1:]] if t[0] == "@" else [t[1:]]
                for i in got:
                    if i not in byid: die(f"genki lesson {lesson}: unknown card {i}")
                    if i not in ids: ids.append(i)
            out.append({"lesson": int(lesson), "topics": topics, "cards": ids})
    return out

def load_guide():
    out = []
    with io.open(os.path.join(CONTENT, "guide.txt"), encoding="utf-8") as f:
        for line in f:
            line = line.rstrip()
            if not line or line.startswith("#"): continue
            if line.startswith("@ "): out.append({"title": line[2:], "body": ""})
            else: out[-1]["body"] += ("\n" if out[-1]["body"] else "") + line
    return out

def load_reading():
    out, cur = {}, None
    with io.open(os.path.join(CONTENT, "reading.txt"), encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"): continue
            if line.startswith("@read "):
                id_, title, ja = [x.strip() for x in line[6:].split("|")]
                cur = {"id": id_, "title": title, "ja": ja, "jp": "", "en": "", "q": []}
                out[id_] = cur
            elif line.startswith("jp:"): cur["jp"] = line[3:].strip().replace("」「", "」\n「")  # each speaker on a line
            elif line.startswith("en:"): cur["en"] = line[3:].strip().replace('" "', '"\n"')
            elif line.startswith("q:"):
                q = [x.strip() for x in line[2:].split("|")]
                if len(q) != 4: die(f"reading {cur['id']}: a question needs one right and two wrong answers")
                cur["q"].append(q)
            else: die(f"reading: unknown line {line}")
    for r in out.values():
        if not r["jp"] or not r["en"] or not r["q"]: die(f"reading {r['id']} is incomplete")
    return out

def check_chats(chats, cards):
    # Every accepted reply must be plain kana, so kanji typed through an IME can be normalised onto it
    for c in chats.values():
        for t in c["turns"]:
            for w in t["want"]:
                bad = [ch for ch in w.rstrip("。") if unicodedata.name(ch, "").startswith("CJK")]
                if bad: die(f"chat {c['id']}: reply {w} has kanji {bad}")
            by_sound = lambda k: k.replace("は", "わ").replace("を", "お").replace("へ", "え")
            if by_sound(romaji_to_kana(t["wantRo"], romaji_table())) != by_sound(to_hira(t["want"][0]).replace("。", "")):
                die(f"chat {c['id']}: revealed romaji {t['wantRo']} doesn't type {t['want'][0]}")

def load_media():
    out = []
    for _, r in tsv("media.tsv"):
        s, era, ty, title, ja, year, task = r
        y = int(year)
        if (era == "old") != (y < 1999) or (era == "new" and y <= 2020):
            die(f"media {title}: {y} breaks the before-1999 / after-2020 rule")
        out.append({"stage": int(s), "era": era, "type": ty, "title": title, "ja": ja, "year": y, "task": task})
    return out


def load_pronouns():
    out = []
    for _, r in tsv("pronouns.tsv"):
        if len(r) != 5 or not r[0] or set(r[0]) - set("mwb"): die(f"pronouns.tsv: bad row {r}")
        if not all("\u3041" <= ch <= "\u309f" for ch in r[2]): die(f"pronouns.tsv: the reading {r[2]} isn't hiragana")
        if r[1] not in r[4]: die(f"pronouns.tsv: the example doesn't use {r[1]}")
        out.append({"who": r[0], "word": r[1], "reading": r[2], "en": r[3], "sent": r[4]})
    return out

def kanji_subs(cards):
    subs = {}
    for c in cards:
        if c["t"] == "w" and c["f"] != c["r"] and any(unicodedata.name(ch, "").startswith("CJK") for ch in c["f"]):
            subs.setdefault(c["f"], to_hira(c["r"]))
    # Stems of the taught verbs and common words, so 食べます and 学校へ both normalise
    subs.update({"食べ": "たべ", "飲み": "のみ", "行き": "いき", "来": "き", "見": "み", "読み": "よみ", "聞き": "きき", "書き": "かき", "話し": "はなし", "買い": "かい",
                 "勉強": "べんきょう", "私": "わたし", "映画": "えいが", "大人": "おとな", "切符": "きっぷ"})
    return sorted(subs.items(), key=lambda kv: -len(kv[0]))

def write_data(guide):
    """The dictionary and stroke data, from content/dict/: see tools/extract_data.py."""
    rows = []
    with io.open(os.path.join(CONTENT, "dict", "jmdict_common.tsv"), encoding="utf-8") as f:
        for line in f:
            word, reading, gloss = line.rstrip("\r\n").split(chr(9))
            h = to_hira(reading)
            rows.append(chr(9).join((word, reading, "" if h == reading else h, gloss)))
    text = "\n".join(rows) + "\n"
    strokes = {}
    with io.open(os.path.join(CONTENT, "dict", "strokes.tsv"), encoding="utf-8") as f:
        for line in f:
            ch, body = line.rstrip("\r\n").split(chr(9))
            strokes[ch] = [[[int(v) for v in p.split(",")] for p in s.split(" ")] for s in body.split("|")]
    www = os.path.join(ROOT, "mobile", "app", "src", "main", "assets", "www")
    with io.open(os.path.join(www, "dict.js"), "w", encoding="utf-8", newline="\n") as f:
        f.write("globalThis.YG_DICT_TEXT=" + json.dumps(text, ensure_ascii=False) + ";\n")
    with io.open(os.path.join(www, "strokes.js"), "w", encoding="utf-8", newline="\n") as f:
        f.write("globalThis.YG_STROKES=" + json.dumps(strokes, ensure_ascii=False, separators=(",", ":")) + ";\n")
    with io.open(os.path.join(ROOT, "extension", "dict.tsv"), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    return len(rows)

def write_js(data):
    text = json.dumps(data, ensure_ascii=False, separators=(",", ":"))
    os.makedirs(GEN, exist_ok=True)
    with io.open(os.path.join(GEN, "content.json"), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    core = io.open(os.path.join(ROOT, "shared", "core.js"), encoding="utf-8").read()
    for d in ("mobile/app/src/main/assets/www", "extension"):
        p = os.path.join(ROOT, d)
        os.makedirs(p, exist_ok=True)
        with io.open(os.path.join(p, "content.js"), "w", encoding="utf-8", newline="\n") as f:
            f.write("globalThis.YG_CONTENT=" + text + ";\n")
        with io.open(os.path.join(p, "core.js"), "w", encoding="utf-8", newline="\n") as f:
            f.write(core)

def main():
    cards, groups = load_cards()
    chats = load_chats()
    reading = load_reading()
    stages, days, exams = load_course(cards, groups, chats, reading)
    check_chats(chats, cards)
    media = load_media()
    table = romaji_table()
    subs = kanji_subs(cards)
    genki = load_genki(cards, groups)
    guide = load_guide()
    pronouns = load_pronouns()
    write_js({"cards": cards, "stages": stages, "days": days, "exams": exams, "chats": chats, "media": media, "roma": table, "hep": hepburn_table(), "subs": subs, "genki": genki, "guide": guide, "reading": reading, "pronouns": pronouns})
    write_data(guide)
    print(f"ok: {len(cards)} cards, {len(days)} days, {len(stages)} stages, {len(exams)} exams, {len(chats)} chats, {len(media)} media")

if __name__ == "__main__":
    main()
