#!/usr/bin/env python3
"""Plays the whole course through the real screens of both Windows builds, with no window, then checks the save."""
import json, os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DAYS = len(json.load(open(os.path.join(ROOT, "build", "gen", "content.json"), encoding="utf-8"))["days"])
START = {"YomiGo.exe": "w2;ow2;ow2;tWalker;ow2;odow2;G;", "YomiGoHTPC.exe": None}
fail = 0
for exe in ("YomiGo.exe", "YomiGoHTPC.exe"):
    root = tempfile.mkdtemp()
    if exe == "YomiGo.exe":
        script = "w2;ow2;ow2;tWalker;w1;ow2;dw1;ow2;dw1;ow2;G;"
    else:
        # the TV build types its name on the on-screen keyboard, then presses Start
        script = "w2;ow2;ow2;ow1;sw2;dw1;ow2;dw1;ow2;G;"
    r = subprocess.run([os.path.join(ROOT, "dist", exe), "--root", root, "--script", script], timeout=600)
    saves = [f for f in os.listdir(root) if f.endswith(".sav")]
    text = open(os.path.join(root, saves[0]), encoding="utf-8").read() if saves else ""
    vals = dict(l.split("=", 1) for l in text.splitlines() if "=" in l)
    cards = sum(1 for l in text.splitlines() if l.startswith("card "))
    ok = r.returncode == 0 and saves and int(vals.get("day", 0)) == DAYS and cards > 400 and int(vals.get("xp", 0)) > 5000 and "exam.N5" in vals and "exam.N4" in vals and int(vals.get("reads", 0)) >= 10
    print(exe, "exit", r.returncode, "day", vals.get("day"), "cards", cards, "xp", vals.get("xp"), "chats", vals.get("chats"), "hw", vals.get("hw"), "reads", vals.get("reads"), "OK" if ok else "FAILED")
    fail += not ok
    shutil.rmtree(root, ignore_errors=True)
# Your own words on the TV: find 猫 in the dictionary, add it, and it must be saved
root = tempfile.mkdtemp()
subprocess.run([os.path.join(ROOT, "dist", "YomiGoHTPC.exe"), "--root", root, "--script", "w2;ow2;ow2;tMine;w1;sw2;dw1;ow2;dw1;ow2;rrrrw8;ow2;RRRw2;tneko;w1;sw2;xw2;"], timeout=60)
text = open(os.path.join(root, "Mine.sav"), encoding="utf-8").read() if os.path.exists(os.path.join(root, "Mine.sav")) else ""
ok = "\t猫\tねこ\t" in text
print("adding from the dictionary", "OK" if ok else "FAILED")
fail += not ok
shutil.rmtree(root, ignore_errors=True)
# Copy and paste, using a stand-in clipboard so the real one is left alone: "Ab" pasted after itself makes "AbAb"
root = tempfile.mkdtemp()
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", "w2;ow2;ow2;tAb;w1;C;w1;V;w1;ow2;"], timeout=60)
ok = os.path.exists(os.path.join(root, "AbAb.sav"))
print("copy and paste", "OK" if ok else "FAILED", os.listdir(root))
fail += not ok
shutil.rmtree(root, ignore_errors=True)
# Text a Japanese keyboard finishes: type every word of day 1 in real kana. Finishing moves the class on and earns 1 XP a word.
esc = lambda s: "".join("\\u%04x" % ord(c) for c in s)
kana = ["おはようございます", "こんにちは", "こんばんは", "さようなら", "おやすみなさい"]
root = tempfile.mkdtemp()
C = json.load(open(os.path.join(ROOT, "build", "gen", "content.json"), encoding="utf-8"))
pages = len(C["days"][0]["notes"]) + len(C["days"][0]["new"])
script = "w2;ow2;ow2;tKana;w1;ow2;dw1;ow2;dw1;ow2;ow2;ow2;" + "ow1;" * pages + "".join(f"t{esc(k)};w1;ow1;" * 3 for k in kana)
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", script], timeout=60)
vals = dict(l.split("=", 1) for l in open(os.path.join(root, "Kana.sav"), encoding="utf-8").read().splitlines() if "=" in l)
ok = vals.get("step") == "3" and vals.get("xp") == "5"
print("typing practice in real kana", "OK" if ok else "FAILED", "step", vals.get("step"), "xp", vals.get("xp"))
fail += not ok
shutil.rmtree(root, ignore_errors=True)
sys.exit(fail != 0)
