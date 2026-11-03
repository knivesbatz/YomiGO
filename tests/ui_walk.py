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
# Listening, on a pretend voice since a test PC may have none: the whole course again, through those screens
for exe in ("YomiGo.exe", "YomiGoHTPC.exe"):
    root = tempfile.mkdtemp()
    script = "w2;ow2;ow2;tEars;w1;ow2;dw1;ow2;dw1;ow2;G;" if exe == "YomiGo.exe" else "w2;ow2;ow2;ow1;sw2;dw1;ow2;dw1;ow2;G;"
    r = subprocess.run([os.path.join(ROOT, "dist", exe), "--root", root, "--fake-voice", "1", "--script", script], timeout=600)
    saves = [f for f in os.listdir(root) if f.endswith(".sav")]
    vals = dict(l.split("=", 1) for l in open(os.path.join(root, saves[0]), encoding="utf-8").read().splitlines() if "=" in l) if saves else {}
    ok = r.returncode == 0 and int(vals.get("day", 0)) == DAYS and int(vals.get("listens", 0)) >= DAYS - 1
    print("listening walk", exe, "listens", vals.get("listens"), "OK" if ok else "FAILED")
    fail += not ok
    shutil.rmtree(root, ignore_errors=True)
# The real Windows speech call, in silence. It needs some voice installed, and CI may not have one.
r = subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--speak-test", "1"], timeout=60)
print("speech call, silent", "OK" if r.returncode == 0 else "no voice or failed: not counted", r.returncode)
# Text Reader: Japanese copied by another program appears, a click looks a word up, and Add keeps the line as the sentence
from PIL import Image, ImageDraw, ImageFont
root = tempfile.mkdtemp()
esc = lambda s: "".join("\\u%04x" % ord(c) for c in s)
line = "今日は猫を見た"
# the rail's Text button is the sixth: from Start class, left twice reaches Settings, then up once
script = "w2;ow2;ow2;tText;w1;ow2;dw1;ow2;dw1;ow2;lluw1;ow2;ow2;K" + esc(line) + ";w2;"
# then click 猫, the fourth character, and the first result's Add button
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", script.replace(";ow2;ow2;K", ";ow2;K") + "ow2;M172,113;w1;M590,183;w1;"], timeout=60)
text = open(os.path.join(root, "Text.sav"), encoding="utf-8").read() if os.path.exists(os.path.join(root, "Text.sav")) else ""
ok = "\t猫\tねこ\t" in text and ("\t" + line) in text
print("text reader: a pasted line, a clicked word, and Add keep the whole line as its sentence:", "OK" if ok else "FAILED")
fail += not ok
shutil.rmtree(root, ignore_errors=True)
# A screenshot through the OCR helper, in English because the build PC has no Japanese OCR. Its lines need Japanese to show, so only the reader's own output is checked.
root = tempfile.mkdtemp()
im = Image.new("RGB", (640, 120), "white"); ImageDraw.Draw(im).text((20, 30), "OCR check 42", fill="black", font=ImageFont.truetype("C:/Windows/Fonts/arial.ttf", 40))
img = os.path.join(root, "shot.bmp"); im.save(img)
r = subprocess.run([os.path.join(ROOT, "dist", "YomiGoOCR.exe"), img, os.path.join(root, "out.txt"), "en"], timeout=120)
got = open(os.path.join(root, "out.txt"), encoding="utf-8").read().strip() if r.returncode == 0 else ""
print("screenshot reader", "OK" if got == "OCR check 42" else "FAILED", repr(got), r.returncode)
fail += got != "OCR check 42"
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", "w2;ow2;ow2;tOcr;w1;ow2;dw1;ow2;dw1;ow2;lluw1;ow2;I" + img.replace("\\", "/") + ";rrw1;ow1;W;pocr;"], timeout=120)
print("screenshot through YomiGo:", "OK" if os.path.exists(os.path.join(root, "ocr_out.txt")) and "OCR check 42" in open(os.path.join(root, "ocr_out.txt"), encoding="utf-8").read() else "FAILED")
fail += not (os.path.exists(os.path.join(root, "ocr_out.txt")) and "OCR check 42" in open(os.path.join(root, "ocr_out.txt"), encoding="utf-8").read())
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
